// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
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
 * 不负责：
 * - 稳定性审查（已上移至 ExecutePipeline）
 * - 传入零力则直接输出 bIsValid=false 的空 candidate，无额外判断
 */
UCLASS()
class P_FLIPPERSUPPORT_API UFlipperForceComposer : public UObject
{
	GENERATED_BODY()

public:
	void ComposeForces(const TArray<FContactPatchData>& ContactPatches,
	                  const TArray<FVector>& SupportForces,
	                  const TArray<FVector>& AntiSlipForces,
	                  const TArray<FVector>& TractionAssistForces,
	                  TArray<FCandidateForce>& OutCandidateForces);

	void SetFrictionCoefficient(float Mu);

	/**
	 * 设置障碍物爬坡摩擦增益
	 * @param Multiplier 增益系数 [0, inf]，有效μ = FrictionCoefficient * (1 + Multiplier * 障碍陡峭度)
	 */
	void SetObstacleClimbFrictionMultiplier(float Multiplier);

private:
	/**
	 * 应用摩擦锥约束，限制切向力不超过 EffectiveMu * 法向力
	 * @param Force 候选力（原地修改）
	 * @param EffectiveMu 有效摩擦系数（已根据接触面陡峭程度调整）
	 */
	void ApplyFrictionConeConstraint(FCandidateForce& Force, float EffectiveMu);

	float FrictionCoefficient = 0.8f;

	/** 障碍物爬坡摩擦增益，陡峭障碍时提升有效摩擦系数 */
	float ObstacleClimbFrictionMultiplier = 1.5f;
};
