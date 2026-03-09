// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FlipperForceSystemTypes.h"
#include "FlipperForceComposer.generated.h"

/**
 * 力组合模块
 * 
 * 职责：
 * - 组合三类力（支撑力、防滑力、牵引辅助力）
 * - 应用摩擦锥约束 |Ft| <= μ * Fn
 * - 生成候选力数组（FCandidateForce）
 * 
 * 实现要点：
 * - 对每个有效且稳定的接触片独立处理
 * - 计算法向力大小 Fn = |SupportForce|
 * - 计算切向力 Ft = AntiSlipForce + TractionAssistForce
 * - 检查摩擦锥约束，若违反则缩放切向力
 * - 保持力方向，仅缩放大小
 * - 当前版本未实现跨接触片的全局力分配或配平
 */
UCLASS()
class P_FLIPPERSUPPORT_API UFlipperForceComposer : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * 组合力并应用摩擦锥约束
	 * @param ContactPatches 接触片数据数组
	 * @param SupportForces 支撑力数组（与 ContactPatches 对应）
	 * @param AntiSlipForces 防滑力数组（与 ContactPatches 对应）
	 * @param TractionAssistForces 牵引辅助力数组（与 ContactPatches 对应）
	 * @param OutCandidateForces 输出候选力数组
	 */
	void ComposeForces(const TArray<FContactPatchData>& ContactPatches,
	                   const TArray<FVector>& SupportForces,
	                   const TArray<FVector>& AntiSlipForces,
	                   const TArray<FVector>& TractionAssistForces,
	                   TArray<FCandidateForce>& OutCandidateForces);

	/**
	 * 设置摩擦系数
	 * @param Mu 摩擦系数 μ，用于摩擦锥约束 |Ft| <= μ * Fn
	 */
	void SetFrictionCoefficient(float Mu);

private:
	/**
	 * 应用摩擦锥约束到单个候选力
	 * 若切向力超过摩擦锥限制，缩放切向力以满足约束
	 * @param Force 候选力数据（输入输出）
	 */
	void ApplyFrictionConeConstraint(FCandidateForce& Force);

	// ===== 配置参数 =====

	/** 摩擦系数 μ，用于摩擦锥约束 |Ft| <= μ * Fn */
	float FrictionCoefficient = 0.8f;
};
