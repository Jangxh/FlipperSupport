// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "FlipperForceSystemTypes.h"
#include "FlipperForceApplicator.generated.h"

/**
 * 力施加模块
 *
 * 职责：
 * - 将最终力优先施加到对应支撑臂刚体
 * - 使用 AddForceAtLocation 在接触点施加力
 * - 若支撑臂刚体缺失，则 fallback 到 RootPrimitive
 */
UCLASS()
class P_FLIPPERSUPPORT_API UFlipperForceApplicator : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * @param InRootPrimitive       车体主刚体，作为 fallback
	 * @param InFlipperPrimitives   四个支撑臂刚体，按 EFlipperIndex 索引
	 */
	void Initialize(UPrimitiveComponent* InRootPrimitive,
	                const TArray<UPrimitiveComponent*>& InFlipperPrimitives);

	void ApplyForces(const TArray<FFinalForce>& FinalForces);

private:
	UPROPERTY()
	TObjectPtr<UPrimitiveComponent> RootPrimitive = nullptr;

	UPROPERTY()
	TArray<TObjectPtr<UPrimitiveComponent>> FlipperPrimitives;
};
