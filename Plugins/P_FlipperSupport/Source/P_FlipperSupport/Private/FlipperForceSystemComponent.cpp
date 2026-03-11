// FlipperForceSystemComponent.cpp
#include "FlipperForceSystemComponent.h"
#include "DrawDebugHelpers.h"

UFlipperForceSystemComponent::UFlipperForceSystemComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UFlipperForceSystemComponent::BeginPlay()
{
    Super::BeginPlay();
    InitVehicleRefs();
}

// ─────────────────────────────────────────────────────────────
//  初始化：找到真正在模拟物理的刚体
// ─────────────────────────────────────────────────────────────
void UFlipperForceSystemComponent::InitVehicleRefs()
{
    AActor* Owner = GetOwner();
    if (!Owner) return;

    VehicleMove = Owner->FindComponentByClass<UChaosWheeledVehicleMovementComponent>();

    // 优先使用根组件，但要确认它是真正在模拟物理的PrimitiveComponent
    UPrimitiveComponent* Root = Cast<UPrimitiveComponent>(Owner->GetRootComponent());
    if (Root && Root->IsSimulatingPhysics())
    {
        VehicleBody = Root;
    }
    else
    {
        // 根不行，找第一个正在模拟物理的PrimitiveComponent作为候选
        TArray<UPrimitiveComponent*> Primitives;
        Owner->GetComponents<UPrimitiveComponent>(Primitives);
        for (UPrimitiveComponent* Comp : Primitives)
        {
            if (Comp && Comp->IsSimulatingPhysics())
            {
                VehicleBody = Comp;
                break;
            }
        }
    }

    if (!VehicleBody)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("FlipperForceSystemComponent: 未找到模拟物理的VehicleBody，施力将无效。Owner: %s"),
            *Owner->GetName());
    }

    if (!VehicleMove)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("FlipperForceSystemComponent: 未找到ChaosWheeledVehicleMovementComponent。Owner: %s"),
            *Owner->GetName());
    }
}

// ─────────────────────────────────────────────────────────────
//  Tick
// ─────────────────────────────────────────────────────────────
void UFlipperForceSystemComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (TrenchState != EFlipperAssistState::Idle)
    {
        TickTrench(DeltaTime);
    }
}

// ─────────────────────────────────────────────────────────────
//  越壕沟：接口
// ─────────────────────────────────────────────────────────────
void UFlipperForceSystemComponent::BeginTrenchCrossing()
{
    if (TrenchState != EFlipperAssistState::Idle) return;

    TrenchState         = EFlipperAssistState::RampingIn;
    TrenchBlendTime     = 0.f;
    TrenchModeAlpha     = 0.f;
    TrenchSmoothedDemand = 0.f;
}

void UFlipperForceSystemComponent::EndTrenchCrossing()
{
    if (TrenchState == EFlipperAssistState::Idle) return;

    // 冻结退出瞬间的快照，RampOut期间不再实时计算
    TrenchCachedExitModeAlpha  = TrenchModeAlpha;
    TrenchCachedExitDemand     = TrenchSmoothedDemand;
    TrenchCachedExitSpeedScale = ComputeSpeedScale();

    TrenchState     = EFlipperAssistState::RampingOut;
    TrenchBlendTime = 0.f;
    // ModeAlpha保持当前值，从此处平滑衰减到0（防止End在RampIn中途时跳变）
}

// ─────────────────────────────────────────────────────────────
//  越壕沟：Tick状态机
// ─────────────────────────────────────────────────────────────
void UFlipperForceSystemComponent::TickTrench(float DeltaTime)
{
    // 除零保护
    const float SafeBlendIn  = FMath::Max(TrenchConfig.BlendInDuration,  KINDA_SMALL_NUMBER);
    const float SafeBlendOut = FMath::Max(TrenchConfig.BlendOutDuration, KINDA_SMALL_NUMBER);

    switch (TrenchState)
    {
    // ── RampingIn：时间线性升起，实时读取需求 ──
    case EFlipperAssistState::RampingIn:
    {
        TrenchBlendTime += DeltaTime;
        TrenchModeAlpha  = FMath::Clamp(TrenchBlendTime / SafeBlendIn, 0.f, 1.f);

        // 平滑需求系数
        float RawDemand     = ComputeRawDemandScale();
        TrenchSmoothedDemand = FMath::FInterpTo(TrenchSmoothedDemand, RawDemand,
                                                DeltaTime, TrenchConfig.DemandInterpSpeed);

        ApplyTrenchForce(TrenchModeAlpha, TrenchSmoothedDemand, ComputeSpeedScale());

        if (TrenchBlendTime >= SafeBlendIn)
        {
            TrenchModeAlpha = 1.f;
            TrenchState     = EFlipperAssistState::Active;
        }
        break;
    }

    // ── Active：ModeAlpha固定1，需求和速度自由变化 ──
    case EFlipperAssistState::Active:
    {
        float RawDemand     = ComputeRawDemandScale();
        TrenchSmoothedDemand = FMath::FInterpTo(TrenchSmoothedDemand, RawDemand,
                                                DeltaTime, TrenchConfig.DemandInterpSpeed);

        ApplyTrenchForce(1.f, TrenchSmoothedDemand, ComputeSpeedScale());
        break;
    }

    // ── RampingOut：从End快照值线性衰减，需求和速度冻结 ──
    case EFlipperAssistState::RampingOut:
    {
        TrenchBlendTime  += DeltaTime;
        float OutProgress  = FMath::Clamp(TrenchBlendTime / SafeBlendOut, 0.f, 1.f);

        // 从ExitModeAlpha衰减到0，而不是强制从1开始（防止中途End时的跳变）
        TrenchModeAlpha = TrenchCachedExitModeAlpha * (1.f - OutProgress);

        // 用冻结的需求和速度，不受实时物理状态干扰
        ApplyTrenchForce(TrenchModeAlpha,
                         TrenchCachedExitDemand,
                         TrenchCachedExitSpeedScale);

        if (TrenchBlendTime >= SafeBlendOut)
        {
            TrenchModeAlpha  = 0.f;
            TrenchState      = EFlipperAssistState::Idle;
        }
        break;
    }

    default: break;
    }
}

// ─────────────────────────────────────────────────────────────
//  越壕沟：施力
// ─────────────────────────────────────────────────────────────
void UFlipperForceSystemComponent::ApplyTrenchForce(
    float InModeAlpha, float InDemandScale, float InSpeedScale) const
{
    if (!VehicleBody) return;

    float ThrottleSign = ComputeThrottleSign();
    if (ThrottleSign < KINDA_SMALL_NUMBER)
        return;
    
    float FinalScale = InModeAlpha * InDemandScale * InSpeedScale;
    if (FinalScale < KINDA_SMALL_NUMBER) return;

    float BaseMagnitude = VehicleBody->GetMass() * TrenchConfig.BaseForcePerMass;
    FVector Forward     = VehicleBody->GetForwardVector();
   
    float  DirSign   = FMath::Sign(ThrottleSign);
    FVector ForceDir    = (Forward * DirSign + FVector::UpVector * TrenchConfig.LiftRatio).GetSafeNormal();
    FVector Force       = ForceDir * BaseMagnitude * FinalScale;

    // 硬上限：防止参数叠加异常
    Force = Force.GetClampedToMaxSize(TrenchConfig.MaxAssistForce);

    VehicleBody->AddForce(Force, NAME_None, false);

#if WITH_EDITOR
    if (bDrawDebug)
    {
        FVector Loc = VehicleBody->GetComponentLocation();
        DrawDebugDirectionalArrow(GetWorld(),
            Loc, Loc + ForceDir * FinalScale * 400.f,
            50.f, FColor::Cyan, false, -1.f, 0, 3.f);
        DrawDebugString(GetWorld(), Loc + FVector(0.f, 0.f, 120.f),
            FString::Printf(TEXT("[Trench] Mode:%.2f  Demand:%.2f  Speed:%.2f  Scale:%.2f"),
                InModeAlpha, InDemandScale, InSpeedScale, FinalScale),
            nullptr, FColor::White, 0.f);
    }
#endif
}

// ─────────────────────────────────────────────────────────────
//  工具函数
// ─────────────────────────────────────────────────────────────
float UFlipperForceSystemComponent::ComputeRawDemandScale() const
{
    float GroundRatio = ComputeGroundRatio();
    // 接地比超过阈值 → 需求为0；完全离地 → 需求为1
    return FMath::Clamp(
        (TrenchConfig.GroundRatioThreshold - GroundRatio) / 
         FMath::Max(TrenchConfig.GroundRatioThreshold, KINDA_SMALL_NUMBER),
        0.f, 1.f
    );
}

float UFlipperForceSystemComponent::ComputeSpeedScale() const
{
    if (!VehicleBody) return 1.f;

    FVector Vel        = VehicleBody->GetPhysicsLinearVelocity();
    float ForwardSpeed = FMath::Abs(
        FVector::DotProduct(Vel, VehicleBody->GetForwardVector())
    );

    return FMath::GetMappedRangeValueClamped(
        TrenchConfig.ForwardSpeedRange,
        TrenchConfig.SpeedScaleRange,
        ForwardSpeed
    );
}

float UFlipperForceSystemComponent::ComputeGroundRatio() const
{
    if (!VehicleMove) return 1.f;

    int32 Total = VehicleMove->GetNumWheels();
    if (Total <= 0) return 1.f;

    int32 Grounded = 0;
    for (int32 i = 0; i < Total; i++)
    {
        FWheelStatus WheelStatus = VehicleMove->GetWheelState(i);
        if (WheelStatus.bInContact)
        {
            Grounded++;
        }
    }
    return static_cast<float>(Grounded) / static_cast<float>(Total);
}

float UFlipperForceSystemComponent::ComputeThrottleSign() const
{
    if (!VehicleMove) return 0.f;
    
    float Throttle = VehicleMove->GetThrottleInput();  // -1~1

    // 死区过滤：极小输入视为无输入，不施力
    if (FMath::Abs(Throttle) < 0.05f) return 0.f;

    return FMath::Clamp(Throttle, -1.f, 1.f);
}