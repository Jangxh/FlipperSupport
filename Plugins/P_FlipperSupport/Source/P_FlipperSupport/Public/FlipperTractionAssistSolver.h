// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FlipperForceSystemTypes.h"
#include "FlipperTractionAssistSolver.generated.h"

/**
 * 牵引辅助力求解模块
 * 
 * 职责：
 * - 计算推进辅助切向力需求
 * - 在特定条件下提供有限的纵向辅助力
 * - 仅在轮地接触不足时启用，不使用闭环控制
 * 
 * 实现要点：
 * - 条件检查：轮地接触比例低、偏航输入小、油门输入大、接触稳定
 * - 牵引方向：车辆纵向方向投影到接触平面
 * - 牵引力大小：由油门、轮地接触缺失程度、接触稳定性共同调制
 * - 限制到 [0, MaxTractionAssistForce]
 * - 禁止使用目标速度误差或闭环控制
 */
UCLASS()
class P_FLIPPERSUPPORT_API UFlipperTractionAssistSolver : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * 计算牵引辅助力
	 * @param Patch 接触片数据，包含接触几何和动力学信息
	 * @param VehicleState 车辆状态快照
	 * @return 牵引辅助力矢量（世界空间，UE 力单位：kg·cm/s²）
	 */
	FVector ComputeForce(const FContactPatchData& Patch, const FFlipperVehicleState& VehicleState);

	/**
	 * 设置最大牵引辅助力
	 * @param MaxForce 最大牵引辅助力（UE 力单位：kg·cm/s²）
	 */
	void SetMaxForce(float MaxForce);

	/**
	 * 设置油门增益
	 * @param Gain 油门增益（UE 力单位：kg·cm/s²）
	 */
	void SetThrottleGain(float Gain);

	/**
	 * 设置条件阈值
	 * @param WheelContactThreshold 轮地接触比例阈值 [0, 1]
	 * @param YawThreshold 偏航输入阈值 [0, 1]
	 * @param ThrottleThreshold 油门输入阈值 [0, 1]
	 */
	void SetConditionThresholds(float WheelContactThreshold, float YawThreshold, float ThrottleThreshold);

	/**
	 * 设置越障爬坡辅助参数
	 * @param UpwardBias 爬坡上向偏置系数 [0, 1]，控制陡峭障碍时的向上力分量权重
	 * @param MinConfidence 最低置信度阈值 [0, 1]，低于此值不提供牵引辅助
	 */
	void SetClimbAssistParams(float UpwardBias, float MinConfidence);

private:
	/**
	 * 检查是否满足牵引辅助条件
	 * @param Patch 接触片数据
	 * @param VehicleState 车辆状态快照
	 * @return 是否应该提供牵引辅助
	 */
	bool ShouldProvideTractionAssist(const FContactPatchData& Patch, const FFlipperVehicleState& VehicleState) const;

	// ===== 配置参数 =====

	/** 最大牵引辅助力（UE 力单位：kg·cm/s²） */
	float MaxTractionAssistForce = 2000.0f;

	/** 牵引辅助油门增益（UE 力单位：kg·cm/s²），F_traction = Gain * |Throttle| * ContactDeficit * Confidence */
	float TractionAssistThrottleGain = 1000.0f;

	/** 轮地接触比例阈值 [0, 1]，低于此值时启用牵引辅助 */
	float WheelContactRatioThreshold = 0.5f;

	/** 偏航输入阈值 [0, 1]，偏航输入低于此值时启用牵引辅助 */
	float YawInputThreshold = 0.3f;

	/** 油门输入阈值 [0, 1]，油门输入高于此值时启用牵引辅助 */
	float ThrottleInputThreshold = 0.1f;

	/**
	 * 越障爬坡上向偏置系数 [0, 1]
	 * 0=仅接触平面投影（垂直障碍时力为零），1=垂直障碍时切换为上向力
	 */
	float ClimbAssistUpwardBias = 1.0f;

	/**
	 * 牵引辅助最低置信度阈值 [0, 1]
	 * 替代 bIsStable 条件，允许 PreStable 阶段提供牵引辅助
	 */
	float MinTractionConfidenceThreshold = 0.1f;
};
