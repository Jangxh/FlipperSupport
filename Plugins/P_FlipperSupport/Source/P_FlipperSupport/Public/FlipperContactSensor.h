// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FlipperForceSystemTypes.h"
#include "FlipperContactSensor.generated.h"

class UPrimitiveComponent;

/**
 * 支撑臂接触探测模块
 * 
 * 职责：
 * - 主动探测四个支撑臂的接触状态
 * - 生成接触片数据（FContactPatchData）
 * - 维护跨帧接触身份、置信度和力历史
 * - 计算接触动力学（速度、切向速度）
 * 
 * 实现要点：
 * - 使用物理查询（Sweep/LineTrace）而非依赖 OnComponentHit 事件
 * - 置信度平滑增减以过滤接触闪烁
 * - 跨帧追踪同一支撑臂的接触历史
 */
UCLASS()
class P_FLIPPERSUPPORT_API UFlipperContactSensor : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * 初始化接触传感器
	 * @param InRootPrimitive 车体主刚体，用于读取速度和角速度
	 * @param InFlipperPrimitives 四个支撑臂的刚体组件数组
	 */
	void Initialize(UPrimitiveComponent* InRootPrimitive, const TArray<UPrimitiveComponent*>& InFlipperPrimitives);

	/**
	 * 每帧探测接触
	 * @param DeltaTime 帧间隔时间（秒）
	 * @param OutContactPatches 输出接触片数据数组（4个元素）
	 */
	void DetectContacts(float DeltaTime, TArray<FContactPatchData>& OutContactPatches);

	/**
	 * 设置接触探测半径
	 * @param Radius 探测半径（cm）
	 */
	void SetDetectionRadius(float Radius);

	/**
	 * 设置接触度量阈值
	 * @param MinThreshold 最小阈值，低于此值视为无效接触
	 * @param MaxThreshold 最大阈值，高于此值视为完全接触
	 */
	void SetContactMetricThresholds(float MinThreshold, float MaxThreshold);

	/**
	 * 设置置信度变化速率
	 * @param IncreaseRate 增加速率（1/s）
	 * @param DecreaseRate 减少速率（1/s）
	 */
	void SetConfidenceRates(float IncreaseRate, float DecreaseRate);

	/**
	 * 设置稳定置信度阈值
	 * @param Threshold 阈值 [0, 1]，置信度超过此值时接触片视为稳定
	 */
	void SetStableConfidenceThreshold(float Threshold);

private:
	/**
	 * 探测单个支撑臂的接触
	 * @param FlipperIndex 支撑臂索引
	 * @param OutHit 输出碰撞结果
	 * @return 是否检测到有效接触
	 */
	bool DetectSingleFlipperContact(int32 FlipperIndex, FHitResult& OutHit);

	/**
	 * 更新接触片的置信度
	 * @param Patch 接触片数据（输入输出）
	 * @param bDetected 本帧是否检测到接触
	 * @param DeltaTime 帧间隔时间（秒）
	 */
	void UpdateConfidence(FContactPatchData& Patch, bool bDetected, float DeltaTime);

	/**
	 * 计算接触动力学
	 * 计算接触点速度和切向速度分量
	 * @param Patch 接触片数据（输入输出）
	 */
	void ComputeContactDynamics(FContactPatchData& Patch);

	// ===== 外部引用 =====

	/** 车体主刚体，用于读取速度和角速度 */
	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> RootPrimitive = nullptr;

	/** 四个支撑臂的刚体组件数组 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPrimitiveComponent>> FlipperPrimitives;

	// ===== 历史数据 =====

	/** 上一帧的接触片数据，用于跨帧追踪身份、置信度和力历史 */
	UPROPERTY(Transient)
	TArray<FContactPatchData> PreviousPatches;

	// ===== 配置参数 =====

	/** 接触探测半径（cm） */
	float DetectionRadius = 50.0f;

	/** 接触度量最小阈值 */
	float ContactMetricMinThreshold = 0.0f;

	/** 接触度量最大阈值 */
	float ContactMetricMaxThreshold = 10.0f;

	/** 置信度增加速率（1/s） */
	float ConfidenceIncreaseRate = 2.0f;

	/** 置信度减少速率（1/s） */
	float ConfidenceDecreaseRate = 4.0f;

	/** 稳定置信度阈值 [0, 1] */
	float StableConfidenceThreshold = 0.7f;
};
