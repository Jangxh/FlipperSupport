// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "FlipperForceSystemTypes.h"
#include "FlipperContactSensor.generated.h"

class UPrimitiveComponent;

/**
 * 接触点跨帧历史，用于稳定化过滤
 * 当本帧命中点与上一帧差距同时超过点位和法线容忍范围时，丢弃本帧
 */
struct FFlipperContactHint
{
	FVector LastContactPoint = FVector::ZeroVector;
	FVector LastContactNormal = FVector::UpVector;
	bool bHasHistory = false;
};

/**
 * 支撑臂接触探测模块
 *
 * 相比原版的改动：
 * - 探测方式：LineTrace → ComponentSweepMulti（使用真实物理碰撞形状）
 * - 扫掠方向：单一 -UpVector → 三方向（-Up、Forward、前下45°）
 * - ContactMetric：DetectionRadius/Distance 伪量 → 压缩量(cm)
 * - 接触动力学：基于 RootPrimitive → 基于对应 FlipperPrimitive
 * - 新增接触点稳定化，过滤帧间接触点跳变噪声
 */
UCLASS()
class P_FLIPPERSUPPORT_API UFlipperContactSensor : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UPrimitiveComponent* InRootPrimitive,
	                const TArray<UPrimitiveComponent*>& InFlipperPrimitives);

	void DetectContacts(float DeltaTime, TArray<FContactPatchData>& OutContactPatches);

	/** 设置 Sweep 每方向位移（保留旧接口名以兼容调用方） */
	void SetDetectionRadius(float Radius);

	void SetTargetContactGap(float Gap);

	/** MaxThreshold 已废弃，保留参数仅为兼容旧调用 */
	void SetContactMetricThresholds(float MinThreshold, float MaxThreshold);

	void SetConfidenceRates(float IncreaseRate, float DecreaseRate);
	void SetStableConfidenceThreshold(float Threshold);

	/** 设置接触点帧间漂移容差和法线一致性容差 */
	void SetContactDriftTolerances(float PointDrift, float NormalDot);

private:
	/** 多方向 ComponentSweep 探测单个支撑臂 */
	bool DetectSingleFlipperContact(int32 FlipperIndex, FHitResult& OutHit);

	/** 单方向 ComponentSweep 辅助函数 */
	bool SweepInDirection(UPrimitiveComponent* FlipperPrimitive,
	                     const FVector& Direction,
	                     float Distance,
	                     const FComponentQueryParams& QueryParams,
	                     FHitResult& OutHit) const;

	void UpdateConfidence(FContactPatchData& Patch, bool bDetected, float DeltaTime);
	void ComputeContactDynamics(FContactPatchData& Patch);

private:
	// ===== 外部引用 =====

	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> RootPrimitive = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPrimitiveComponent>> FlipperPrimitives;

	// ===== 历史数据 =====

	UPROPERTY(Transient)
	TArray<FContactPatchData> PreviousPatches;

	/** 每个支撑臂的接触点历史，用于跳变过滤 */
	TArray<FFlipperContactHint> ContactHints;

	// ===== 配置参数 =====

	/** Sweep 每方向位移 (cm) */
	float DetectionSweepDistance = 8.0f;

	/** 目标接触间隙 (cm)，必须 < DetectionSweepDistance */
	float TargetContactGap = 3.0f;

	/** 接触度量最小阈值 */
	float ContactMetricMinThreshold = 0.0f;

	/** 接触点最大允许漂移 (cm) */
	float ContactPointDriftTolerance = 7.0f;

	/** 法线 dot 下限（~32°） */
	float ContactNormalDotTolerance = 0.85f;

	float ConfidenceIncreaseRate = 6.0f;
	float ConfidenceDecreaseRate = 2.0f;
	float StableConfidenceThreshold = 0.45f;
};
