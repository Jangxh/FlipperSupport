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

private:
	void ApplyFrictionConeConstraint(FCandidateForce& Force);

	float FrictionCoefficient = 0.8f;
};
