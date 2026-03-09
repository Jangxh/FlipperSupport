// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FlipperForceSystemTypes.h"
#include "FlipperSupportSolver.generated.h"

/**
 * 支撑力求解模块
 * 
 * 职责：
 * - 计算法向支撑力需求
 * - 基于接触度量和法向相对速度计算支撑力大小
 * - 提供有限支撑，不是完整悬挂替代
 * 
 * 实现要点：
 * - 支撑力方向：沿接触法线 N
 * - 支撑力大小：F_support = Stiffness * ContactMetric - Damping * Vn
 * - 限制到 [0, MaxSupportForce]
 * - 阻尼项用于减少接触切换时的振荡
 */
UCLASS()
class P_FLIPPERSUPPORT_API UFlipperSupportSolver : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * 计算支撑力
	 * @param Patch 接触片数据，包含接触几何和动力学信息
	 * @param VehicleState 车辆状态快照
	 * @return 支撑力矢量（世界空间，UE 力单位：kg·cm/s²）
	 */
	FVector ComputeForce(const FContactPatchData& Patch, const FFlipperVehicleState& VehicleState);

	/**
	 * 设置最大支撑力
	 * @param MaxForce 最大支撑力（UE 力单位：kg·cm/s²）
	 */
	void SetMaxForce(float MaxForce);

	/**
	 * 设置支撑力刚度系数
	 * @param Stiffness 刚度系数（kg/s²）
	 */
	void SetStiffness(float Stiffness);

	/**
	 * 设置支撑力阻尼系数
	 * @param Damping 阻尼系数（kg/s）
	 */
	void SetDamping(float Damping);

private:
	// ===== 配置参数 =====

	/** 最大支撑力（UE 力单位：kg·cm/s²） */
	float MaxSupportForce = 5000.0f;

	/** 支撑力刚度系数（kg/s²） */
	float SupportForceStiffness = 500.0f;

	/** 支撑力阻尼系数（kg/s） */
	float SupportForceDamping = 50.0f;
};
