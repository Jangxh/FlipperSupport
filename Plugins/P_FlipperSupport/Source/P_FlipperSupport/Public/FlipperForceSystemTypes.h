#pragma once

#include "CoreMinimal.h"
#include "FlipperForceSystemTypes.generated.h"

USTRUCT(BlueprintType)
struct FTrenchCrossingConfig
{
    GENERATED_BODY()

    // 接地比低于此值才产生辅助需求（0~1）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Threshold",
        meta=(ClampMin="0.0", ClampMax="1.0"))
    float GroundRatioThreshold = 0.5f;

    // 基础力系数：Force = VehicleMass × BaseForcePerMass × FinalScale
    // 物理意义等同于"最强辅助时的目标额外加速度标尺"
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Force",
        meta=(ClampMin="0.0"))
    float BaseForcePerMass = 150.f;

    // 垂直分量比例，0=纯前推，越大越抬头
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Force",
        meta=(ClampMin="0.0", ClampMax="1.0"))
    float LiftRatio = 0.1f;

    // 最终力的绝对上限（牛顿），防止参数叠出异常
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Force",
        meta=(ClampMin="0.0"))
    float MaxAssistForce = 2000000.f;

    // 模式淡入时长（秒）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Blend",
        meta=(ClampMin="0.0"))
    float BlendInDuration = 0.25f;

    // 模式淡出时长（秒）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Blend",
        meta=(ClampMin="0.0"))
    float BlendOutDuration = 0.4f;

    // DemandScale的插值速度，平滑接地比离散跳变
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Blend",
        meta=(ClampMin="0.0"))
    float DemandInterpSpeed = 8.f;

    // 前向速度范围（cm/s）：低速端到高速端
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Speed")
    FVector2D ForwardSpeedRange = FVector2D(0.f, 500.f);

    // 对应的力缩放范围：低速增强，高速正常
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Speed")
    FVector2D SpeedScaleRange = FVector2D(1.5f, 1.0f);
};


USTRUCT(BlueprintType)
struct FVerticalObstacleConfig
{
    GENERATED_BODY()
};


USTRUCT(BlueprintType)
struct FStairClimbingConfig
{
    GENERATED_BODY()

   
};

/**
 * 支撑臂状态
 */
UENUM()
enum class EFlipperAssistState : uint8
{
    Idle,
    RampingIn,
    Active,
    RampingOut
};


