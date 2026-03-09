// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlipperForceSystemComponent.h"

#include "C_TankMovementComponent.h"
#include "FlipperContactSensor.h"
#include "VehicleStateProvider.h"
#include "FlipperSupportSolver.h"
#include "FlipperAntiSlipSolver.h"
#include "FlipperTractionAssistSolver.h"
#include "FlipperForceComposer.h"
#include "FlipperForceSafetyFilter.h"
#include "FlipperForceApplicator.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "DrawDebugHelpers.h"

UFlipperForceSystemComponent::UFlipperForceSystemComponent()
{
	// 设置组件每帧 Tick
	PrimaryComponentTick.bCanEverTick = true;
	
	// 默认启用系统
	bEnableSystem = true;
	
	// 默认禁用调试可视化
	bEnableDebugVisualization = false;
	
	// 预分配运行时数据数组
	ContactPatches.SetNum(4);
	CandidateForces.Reserve(4);
	FinalForces.Reserve(4);
}

void UFlipperForceSystemComponent::BeginPlay()
{
	Super::BeginPlay();

	// 解析并缓存组件引用
	ResolveAndCacheComponents();

	// 验证所有必需的外部引用
	if (!ValidateReferences())
	{
		UE_LOG(LogTemp, Error, TEXT("FlipperForceSystem: 引用校验失败，禁用系统"));
		bEnableSystem = false;
		return;
	}

	// 创建所有子模块实例
	ContactSensor = NewObject<UFlipperContactSensor>(this);
	StateProvider = NewObject<UVehicleStateProvider>(this);
	SupportSolver = NewObject<UFlipperSupportSolver>(this);
	AntiSlipSolver = NewObject<UFlipperAntiSlipSolver>(this);
	TractionAssistSolver = NewObject<UFlipperTractionAssistSolver>(this);
	ForceComposer = NewObject<UFlipperForceComposer>(this);
	SafetyFilter = NewObject<UFlipperForceSafetyFilter>(this);
	ForceApplicator = NewObject<UFlipperForceApplicator>(this);

	// 初始化所有子模块
	InitializeSubmodules();

	UE_LOG(LogTemp, Log, TEXT("FlipperForceSystem: 系统初始化成功"));
}

void UFlipperForceSystemComponent::TickComponent(float DeltaTime, ELevelTick TickType, 
                                                 FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 检查系统是否启用
	if (!bEnableSystem)
	{
		return;
	}

	// BeginPlay 中已完成强校验，这里仅做轻量防御检查
	if (!RootPrimitive || !TankMovementComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("FlipperForceSystem: 运行时关键引用丢失，禁用系统"));
		bEnableSystem = false;
		return;
	}

	// 执行主管道
	ExecutePipeline(DeltaTime);

	// 调试可视化
	if (bEnableDebugVisualization)
	{
		DrawDebugVisualization();
	}
}

void UFlipperForceSystemComponent::InitializeSubmodules()
{
	// 初始化 ContactSensor
	ContactSensor->Initialize(RootPrimitive, FlipperPrimitives);
	ContactSensor->SetDetectionRadius(Config.ContactDetectionRadius);
	ContactSensor->SetContactMetricThresholds(Config.ContactMetricMinThreshold, Config.ContactMetricMaxThreshold);
	ContactSensor->SetConfidenceRates(Config.ConfidenceIncreaseRate, Config.ConfidenceDecreaseRate);
	ContactSensor->SetStableConfidenceThreshold(Config.StableConfidenceThreshold);

	// 初始化 StateProvider
	StateProvider->Initialize(RootPrimitive, TankMovementComponent);

	// 配置 SupportSolver
	SupportSolver->SetMaxForce(Config.MaxSupportForce);
	SupportSolver->SetStiffness(Config.SupportForceStiffness);
	SupportSolver->SetDamping(Config.SupportForceDamping);

	// 配置 AntiSlipSolver
	AntiSlipSolver->SetMaxForce(Config.MaxAntiSlipForce);
	AntiSlipSolver->SetGain(Config.AntiSlipForceGain);
	AntiSlipSolver->SetVelocityDeadband(Config.AntiSlipVelocityDeadband);

	// 配置 TractionAssistSolver
	TractionAssistSolver->SetMaxForce(Config.MaxTractionAssistForce);
	TractionAssistSolver->SetThrottleGain(Config.TractionAssistThrottleGain);
	TractionAssistSolver->SetConditionThresholds(
		Config.WheelContactRatioThreshold,
		Config.YawInputThreshold,
		Config.ThrottleInputThreshold
	);

	// 配置 ForceComposer
	ForceComposer->SetFrictionCoefficient(Config.FrictionCoefficient);

	// 初始化 SafetyFilter
	SafetyFilter->Initialize(RootPrimitive);
	SafetyFilter->SetChassisContactReductionFactor(Config.ChassisContactReductionFactor);
	SafetyFilter->SetMomentLimits(Config.MaxPitchTorque, Config.MaxRollTorque);
	SafetyFilter->SetSmoothingParameters(Config.ForceDirectionSmoothingRate, Config.ForceJerkLimit);
	SafetyFilter->SetForceDecayRate(Config.ForceDecayRate);

	// 初始化 ForceApplicator
	ForceApplicator->Initialize(RootPrimitive);
}

void UFlipperForceSystemComponent::ResolveAndCacheComponents()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		UE_LOG(LogTemp, Error, TEXT("FlipperForceSystem: Owner 为空"));
		return;
	}

	// 解析 RootPrimitive
	if (RootPrimitiveName.IsNone())
	{
		// 如果未指定名称，使用 Actor 的 RootComponent
		RootPrimitive = Cast<UPrimitiveComponent>(Owner->GetRootComponent());
		if (RootPrimitive)
		{
			UE_LOG(LogTemp, Log, TEXT("FlipperForceSystem: 自动使用 RootComponent 作为 RootPrimitive"));
		}
	}
	else
	{
		// 根据名称查找
		RootPrimitive = FindPrimitiveByName(RootPrimitiveName);
		if (RootPrimitive)
		{
			UE_LOG(LogTemp, Log, TEXT("FlipperForceSystem: 找到 RootPrimitive: %s"), *RootPrimitiveName.ToString());
		}
	}

	// 解析 TankMovementComponent
	if (TankMovementComponentName.IsNone())
	{
		// 如果未指定名称，查找第一个 UC_TankMovementComponent
		TankMovementComponent = Owner->FindComponentByClass<UC_TankMovementComponent>();
		if (TankMovementComponent)
		{
			UE_LOG(LogTemp, Log, TEXT("FlipperForceSystem: 自动找到 TankMovementComponent"));
		}
	}
	else
	{
		// 根据名称查找
		TArray<UC_TankMovementComponent*> MovementComponents;
		Owner->GetComponents<UC_TankMovementComponent>(MovementComponents);
		for (UC_TankMovementComponent* Comp : MovementComponents)
		{
			if (Comp && Comp->GetFName() == TankMovementComponentName)
			{
				TankMovementComponent = Comp;
				UE_LOG(LogTemp, Log, TEXT("FlipperForceSystem: 找到 TankMovementComponent: %s"), 
				       *TankMovementComponentName.ToString());
				break;
			}
		}
	}

	// 解析四个支撑臂组件
	FlipperPrimitives.Empty();
	FlipperPrimitives.SetNum(4);

	FlipperPrimitives[0] = FindPrimitiveByName(FrontLeftFlipperName);
	FlipperPrimitives[1] = FindPrimitiveByName(FrontRightFlipperName);
	FlipperPrimitives[2] = FindPrimitiveByName(RearLeftFlipperName);
	FlipperPrimitives[3] = FindPrimitiveByName(RearRightFlipperName);

	// 记录支撑臂查找结果
	const TArray<FName> FlipperNames = {
		FrontLeftFlipperName, FrontRightFlipperName, 
		RearLeftFlipperName, RearRightFlipperName
	};
	const TArray<FString> FlipperLabels = {
		TEXT("前左"), TEXT("前右"), TEXT("后左"), TEXT("后右")
	};

	for (int32 i = 0; i < 4; ++i)
	{
		if (FlipperPrimitives[i])
		{
			UE_LOG(LogTemp, Log, TEXT("FlipperForceSystem: 找到%s支撑臂: %s"), 
			       *FlipperLabels[i], *FlipperNames[i].ToString());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("FlipperForceSystem: 未找到%s支撑臂: %s"), 
			       *FlipperLabels[i], *FlipperNames[i].ToString());
		}
	}
}

UPrimitiveComponent* UFlipperForceSystemComponent::FindPrimitiveByName(const FName& CompName) const
{
	if (CompName.IsNone())
	{
		return nullptr;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	// 查找所有 PrimitiveComponent
	TArray<UPrimitiveComponent*> Primitives;
	Owner->GetComponents<UPrimitiveComponent>(Primitives);

	for (UPrimitiveComponent* Comp : Primitives)
	{
		if (Comp && Comp->GetFName() == CompName)
		{
			return Comp;
		}
	}

	return nullptr;
}

bool UFlipperForceSystemComponent::ValidateReferences()
{
	// 检查 RootPrimitive 引用
	if (!RootPrimitive)
	{
		UE_LOG(LogTemp, Error, TEXT("FlipperForceSystem: RootPrimitive 引用无效"));
		return false;
	}

	// 检查 TankMovementComponent 引用
	if (!TankMovementComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("FlipperForceSystem: TankMovementComponent 引用无效"));
		return false;
	}

	// 检查 FlipperPrimitives 数组大小
	if (FlipperPrimitives.Num() != 4)
	{
		UE_LOG(LogTemp, Error, TEXT("FlipperForceSystem: FlipperPrimitives 数组大小不为 4（当前: %d）"), 
		       FlipperPrimitives.Num());
		return false;
	}

	// 检查每个 FlipperPrimitive 元素
	for (int32 i = 0; i < FlipperPrimitives.Num(); ++i)
	{
		if (!FlipperPrimitives[i])
		{
			UE_LOG(LogTemp, Error, TEXT("FlipperForceSystem: FlipperPrimitives[%d] 引用无效"), i);
			return false;
		}
	}

	// 所有引用有效
	return true;
}

void UFlipperForceSystemComponent::ExecutePipeline(float DeltaTime)
{
	// 步骤 1: 探测接触
	ContactSensor->DetectContacts(DeltaTime, ContactPatches);

	// 步骤 2: 构建车辆状态
	StateProvider->BuildState(VehicleState);

	// 步骤 3: 提前退出检查
	if (ShouldEarlyExit())
	{
		// 仅衰减缓存力，不施加任何力
		SafetyFilter->DecayForces(ContactPatches, DeltaTime, FinalForces);
		
		// 更新 PreviousForce 历史
		for (int32 i = 0; i < ContactPatches.Num() && i < FinalForces.Num(); ++i)
		{
			ContactPatches[i].PreviousForce = FinalForces[i].Force;
		}
		
		return; // 本帧不调用 ForceApplicator
	}

	// 步骤 4: 对每个有效且稳定的接触片计算三类力
	TArray<FVector> SupportForces;
	TArray<FVector> AntiSlipForces;
	TArray<FVector> TractionAssistForces;

	SupportForces.Reserve(ContactPatches.Num());
	AntiSlipForces.Reserve(ContactPatches.Num());
	TractionAssistForces.Reserve(ContactPatches.Num());

	for (const FContactPatchData& Patch : ContactPatches)
	{
		if (Patch.bIsValid && Patch.bIsStable)
		{
			// 计算三类力
			SupportForces.Add(SupportSolver->ComputeForce(Patch, VehicleState));
			AntiSlipForces.Add(AntiSlipSolver->ComputeForce(Patch, VehicleState));
			TractionAssistForces.Add(TractionAssistSolver->ComputeForce(Patch, VehicleState));
		}
		else
		{
			// 无效或不稳定的接触片，力为零
			SupportForces.Add(FVector::ZeroVector);
			AntiSlipForces.Add(FVector::ZeroVector);
			TractionAssistForces.Add(FVector::ZeroVector);
		}
	}

	// 步骤 5: 组合力并应用摩擦锥约束
	ForceComposer->ComposeForces(ContactPatches, SupportForces, 
	                             AntiSlipForces, TractionAssistForces, 
	                             CandidateForces);

	// 步骤 6: 安全过滤
	SafetyFilter->FilterForces(CandidateForces, ContactPatches, 
	                          VehicleState, DeltaTime, FinalForces);

	// 步骤 7: 更新 PreviousForce 历史
	for (int32 i = 0; i < ContactPatches.Num() && i < FinalForces.Num(); ++i)
	{
		ContactPatches[i].PreviousForce = FinalForces[i].Force;
	}

	// 步骤 8: 施加力
	ForceApplicator->ApplyForces(FinalForces);
}

bool UFlipperForceSystemComponent::ShouldEarlyExit()
{
	// 检查是否存在至少一个有效且稳定的接触片
	for (const FContactPatchData& Patch : ContactPatches)
	{
		if (Patch.bIsValid && Patch.bIsStable)
		{
			return false; // 至少有一个有效且稳定的接触，不提前退出
		}
	}
	
	// 无有效接触，应该提前退出
	return true;
}

void UFlipperForceSystemComponent::DrawDebugVisualization()
{
	if (!GetWorld())
	{
		return;
	}

	// 力矢量缩放因子，用于将力大小转换为可视化箭头长度
	const float ForceScale = 0.01f;

	// 遍历所有接触片
	for (int32 i = 0; i < ContactPatches.Num(); ++i)
	{
		const FContactPatchData& Patch = ContactPatches[i];

		// 绘制接触点（颜色编码：绿色=稳定，黄色=不稳定，红色=无效）
		FColor PointColor;
		if (!Patch.bIsValid)
		{
			// 无效接触：红色
			PointColor = FColor::Red;
		}
		else if (Patch.bIsStable)
		{
			// 有效且稳定：绿色
			PointColor = FColor::Green;
		}
		else
		{
			// 有效但不稳定：黄色
			PointColor = FColor::Yellow;
		}

		// 绘制接触点球体
		DrawDebugSphere(GetWorld(), Patch.ContactPoint, 10.0f, 8, PointColor, false, -1.0f, 0, 1.0f);

		// 仅对有效接触绘制法线和置信度
		if (Patch.bIsValid)
		{
			// 绘制接触法线（蓝色箭头）
			DrawDebugDirectionalArrow(
				GetWorld(),
				Patch.ContactPoint,
				Patch.ContactPoint + Patch.ContactNormal * 50.0f,
				5.0f,
				FColor::Blue,
				false,
				-1.0f,
				0,
				1.5f
			);

			// 绘制置信度文本
			DrawDebugString(
				GetWorld(),
				Patch.ContactPoint + FVector(0, 0, 20),
				FString::Printf(TEXT("Conf: %.2f"), Patch.Confidence),
				nullptr,
				FColor::White,
				0.0f,
				true
			);
		}

		// 绘制候选力的三类子力（完整调试模式）
		if (i < CandidateForces.Num() && CandidateForces[i].bIsValid)
		{
			const FCandidateForce& Candidate = CandidateForces[i];

			// 绘制支撑力（绿色箭头）
			if (!Candidate.SupportForce.IsNearlyZero())
			{
				DrawDebugDirectionalArrow(
					GetWorld(),
					Candidate.ApplicationPoint,
					Candidate.ApplicationPoint + Candidate.SupportForce * ForceScale,
					5.0f,
					FColor::Green,
					false,
					-1.0f,
					0,
					1.5f
				);
			}

			// 绘制防滑力（黄色箭头）
			if (!Candidate.AntiSlipForce.IsNearlyZero())
			{
				DrawDebugDirectionalArrow(
					GetWorld(),
					Candidate.ApplicationPoint,
					Candidate.ApplicationPoint + Candidate.AntiSlipForce * ForceScale,
					5.0f,
					FColor::Yellow,
					false,
					-1.0f,
					0,
					1.5f
				);
			}

			// 绘制牵引辅助力（橙色箭头）
			if (!Candidate.TractionAssistForce.IsNearlyZero())
			{
				DrawDebugDirectionalArrow(
					GetWorld(),
					Candidate.ApplicationPoint,
					Candidate.ApplicationPoint + Candidate.TractionAssistForce * ForceScale,
					5.0f,
					FColor::Orange,
					false,
					-1.0f,
					0,
					1.5f
				);
			}
		}

		// 绘制最终总力（红色粗箭头）
		if (i < FinalForces.Num() && FinalForces[i].bShouldApply)
		{
			const FFinalForce& Force = FinalForces[i];

			if (!Force.Force.IsNearlyZero())
			{
				DrawDebugDirectionalArrow(
					GetWorld(),
					Force.ApplicationPoint,
					Force.ApplicationPoint + Force.Force * ForceScale,
					10.0f,
					FColor::Red,
					false,
					-1.0f,
					0,
					2.5f
				);
			}
		}
	}
}
