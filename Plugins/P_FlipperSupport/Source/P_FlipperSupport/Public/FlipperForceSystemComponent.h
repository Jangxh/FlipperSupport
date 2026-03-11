// FlipperForceSystemComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "FlipperForceSystemTypes.h"
#include "FlipperForceSystemComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class P_FLIPPERSUPPORT_API UFlipperForceSystemComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UFlipperForceSystemComponent();

    /**
     * 壕沟配置
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FlipperForce|Trench")
    FTrenchCrossingConfig TrenchConfig;

    // ── 调试开关 ──
    UPROPERTY(EditDefaultsOnly, Category="FlipperForce|Debug")
    bool bDrawDebug = false;

    // ── 越壕沟接口 ──
    UFUNCTION(BlueprintCallable, Category="FlipperForce")
    void BeginTrenchCrossing();

    UFUNCTION(BlueprintCallable, Category="FlipperForce")
    void EndTrenchCrossing();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

private:
    // ── 车辆引用 ──
    UPROPERTY() 
    UPrimitiveComponent* VehicleBody = nullptr;
    UPROPERTY() 
    UChaosWheeledVehicleMovementComponent* VehicleMove = nullptr;

    // ── 越壕沟运行时状态 ──
    EFlipperAssistState TrenchState    = EFlipperAssistState::Idle;
    float TrenchBlendTime              = 0.f;   // 当前阶段已经过的时间
    float TrenchModeAlpha              = 0.f;   // 当前模式整体强度 0~1
    float TrenchSmoothedDemand         = 0.f;   // 平滑后的需求系数

    // RampOut冻结快照
    float TrenchCachedExitModeAlpha    = 1.f;
    float TrenchCachedExitDemand       = 0.f;
    float TrenchCachedExitSpeedScale   = 1.f;

    // ── 内部函数 ──
    void InitVehicleRefs();

    // 越壕沟
    void  TickTrench(float DeltaTime);
    void  ApplyTrenchForce(float InModeAlpha, float InDemandScale, float InSpeedScale) const;
    float ComputeRawDemandScale() const;  // 实时接地缺失需求
    float ComputeSpeedScale() const;      // 前向速度缩放
    float ComputeGroundRatio() const;     // 接地比
    float ComputeThrottleSign() const;
};