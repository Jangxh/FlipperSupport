// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FlipperForceSystemTypes.h"
#include "FlipperForceSafetyFilter.generated.h"

/**
 * 安全过滤模块
 * 
 * 职责：
 * - 对候选力应用安全裁剪和平滑
 * - 接触平面裁剪：移除指向地面内部的力分量
 * - 底盘接触降额：底盘接触时降低牵引辅助力
 * - 俯仰/横滚力矩限幅：限制总扭矩以防止不真实旋转
 * - 方向和大小平滑：平滑力方向变化和限制 jerk
 * - 力衰减：无接触时衰减缓存力
 * 
 * 实现要点：
 * - 按顺序应用所有过滤步骤
 * - 保持力方向，仅缩放大小（力矩限幅时）
 * - 从 ContactPatchData.PreviousForce 获取历史用于平滑
 * - 力衰减仅用于更新缓存，不施加力
 */
UCLASS()
class P_FLIPPERSUPPORT_API UFlipperForceSafetyFilter : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * 初始化安全过滤器
	 * @param InRootPrimitive 车体主刚体引用，用于获取质心位置
	 */
	void Initialize(UPrimitiveComponent* InRootPrimitive);

	/**
	 * 过滤候选力，应用所有安全约束和平滑
	 * @param CandidateForces 候选力数组（输入）
	 * @param ContactPatches 接触片数据数组（用于获取历史和几何信息）
	 * @param VehicleState 车辆状态（用于底盘接触检查和质心位置）
	 * @param DeltaTime 帧时间间隔（秒）
	 * @param OutFinalForces 输出最终力数组
	 */
	void FilterForces(const TArray<FCandidateForce>& CandidateForces,
	                  const TArray<FContactPatchData>& ContactPatches,
	                  const FFlipperVehicleState& VehicleState,
	                  float DeltaTime,
	                  TArray<FFinalForce>& OutFinalForces);

	/**
	 * 无接触时力衰减（用于 EarlyExit）
	 * 对无有效接触的支撑臂应用指数衰减，仅用于更新缓存，不施加力
	 * @param ContactPatches 接触片数据数组
	 * @param DeltaTime 帧时间间隔（秒）
	 * @param OutFinalForces 输出最终力数组（bShouldApply 将为 false）
	 */
	void DecayForces(const TArray<FContactPatchData>& ContactPatches,
	                 float DeltaTime,
	                 TArray<FFinalForce>& OutFinalForces);

	/**
	 * 设置底盘接触降额因子
	 * @param Factor 降额因子 [0, 1]，底盘接触时牵引辅助力的缩放比例
	 */
	void SetChassisContactReductionFactor(float Factor);

	/**
	 * 设置力矩限制
	 * @param MaxPitchTorque 最大俯仰扭矩（UE 扭矩单位：kg·cm²/s²）
	 * @param MaxRollTorque 最大横滚扭矩（UE 扭矩单位：kg·cm²/s²）
	 */
	void SetMomentLimits(float MaxPitchTorque, float MaxRollTorque);

	/**
	 * 设置平滑参数
	 * @param DirectionSmoothingRate 力方向平滑速率 (1/s)
	 * @param JerkLimit 力 Jerk 限制（UE 力单位/秒：kg·cm/s³）
	 */
	void SetSmoothingParameters(float DirectionSmoothingRate, float JerkLimit);

	/**
	 * 设置力衰减速率
	 * @param DecayRate 力衰减速率 (1/s)，无接触时缓存力的指数衰减速率
	 */
	void SetForceDecayRate(float DecayRate);

private:
	/**
	 * 接触平面裁剪
	 * 检查支撑力是否指向地面内部，若是则归零
	 * 投影切向力到接触平面
	 * @param Forces 候选力数组（输入输出）
	 * @param Patches 接触片数据数组
	 */
	void ApplyContactPlaneClipping(TArray<FCandidateForce>& Forces,
	                                const TArray<FContactPatchData>& Patches);

	/**
	 * 底盘接触降额
	 * 检查底盘接触状态，对 TractionAssistForce 应用降额因子
	 * 不影响 SupportForce 和 AntiSlipForce
	 * @param Forces 候选力数组（输入输出）
	 * @param VehicleState 车辆状态
	 */
	void ApplyChassisContactReduction(TArray<FCandidateForce>& Forces,
	                                   const FFlipperVehicleState& VehicleState);

	/**
	 * 俯仰/横滚力矩限幅
	 * 计算所有力关于质心的总扭矩，提取俯仰和横滚分量
	 * 若超限，统一缩放所有力，保持力方向
	 * @param Forces 候选力数组（输入输出）
	 * @param VehicleState 车辆状态（用于质心位置和坐标系）
	 */
	void ApplyMomentLimiting(TArray<FCandidateForce>& Forces,
	                         const FFlipperVehicleState& VehicleState);

	/**
	 * 方向和大小平滑
	 * 从 ContactPatchData.PreviousForce 获取历史
	 * 平滑力方向变化，限制力大小变化率（jerk 限制）
	 * @param Forces 候选力数组（输入输出）
	 * @param Patches 接触片数据数组（用于获取 PreviousForce）
	 * @param DeltaTime 帧时间间隔（秒）
	 */
	void ApplySmoothingAndJerkLimiting(TArray<FCandidateForce>& Forces,
	                                    const TArray<FContactPatchData>& Patches,
	                                    float DeltaTime);

	// ===== 外部引用 =====

	/** 车体主刚体引用，用于获取质心位置 */
	UPROPERTY()
	UPrimitiveComponent* RootPrimitive = nullptr;

	// ===== 配置参数 =====

	/** 底盘接触降额因子 [0, 1]，底盘接触时牵引辅助力的缩放比例 */
	float ChassisContactReductionFactor = 0.5f;

	/** 最大俯仰扭矩（UE 扭矩单位：kg·cm²/s²），限制所有支撑臂力产生的总俯仰扭矩 */
	float MaxPitchTorque = 10000.0f;

	/** 最大横滚扭矩（UE 扭矩单位：kg·cm²/s²），限制所有支撑臂力产生的总横滚扭矩 */
	float MaxRollTorque = 8000.0f;

	/** 力方向平滑速率 (1/s)，控制力方向变化的平滑程度 */
	float ForceDirectionSmoothingRate = 5.0f;

	/** 力 Jerk 限制（UE 力单位/秒：kg·cm/s³），限制力大小的变化率，防止突变 */
	float ForceJerkLimit = 50000.0f;

	/** 力衰减速率 (1/s)，无接触时缓存力的指数衰减速率 */
	float ForceDecayRate = 3.0f;
};
