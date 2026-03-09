// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FlipperForceSystemTypes.h"
#include "FlipperAntiSlipSolver.generated.h"

/**
 * 防滑力求解模块
 * 
 * 职责：
 * - 计算抗滑切向力需求
 * - 基于接触点切向速度计算防滑力
 * - 仅响应滑动趋势，不引入推进力
 * 
 * 实现要点：
 * - 防滑力方向：与切向速度相反
 * - 防滑力大小：F_antislip = Gain * |V_tangential|
 * - 应用速度死区以避免在极低滑动速度时产生过大的力
 * - 限制到 [0, MaxAntiSlipForce]
 * - 确保力完全位于接触平面内
 */
UCLASS()
class P_FLIPPERSUPPORT_API UFlipperAntiSlipSolver : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * 计算防滑力
	 * @param Patch 接触片数据，包含接触几何和动力学信息
	 * @param VehicleState 车辆状态快照
	 * @return 防滑力矢量（世界空间，UE 力单位：kg·cm/s²）
	 */
	FVector ComputeForce(const FContactPatchData& Patch, const FFlipperVehicleState& VehicleState);

	/**
	 * 设置最大防滑力
	 * @param MaxForce 最大防滑力（UE 力单位：kg·cm/s²）
	 */
	void SetMaxForce(float MaxForce);

	/**
	 * 设置防滑力增益
	 * @param Gain 防滑力增益（kg/s）
	 */
	void SetGain(float Gain);

	/**
	 * 设置速度死区
	 * @param Deadband 速度死区 (cm/s)
	 */
	void SetVelocityDeadband(float Deadband);

private:
	// ===== 配置参数 =====

	/** 最大防滑力（UE 力单位：kg·cm/s²） */
	float MaxAntiSlipForce = 3000.0f;

	/** 防滑力增益（kg/s），F_antislip = Gain * |V_tangential| */
	float AntiSlipForceGain = 100.0f;

	/** 防滑力速度死区 (cm/s)，切向速度低于此值时不施加防滑力 */
	float AntiSlipVelocityDeadband = 0.1f;
};
