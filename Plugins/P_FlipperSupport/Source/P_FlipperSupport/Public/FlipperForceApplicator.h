// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FlipperForceSystemTypes.h"
#include "FlipperForceApplicator.generated.h"

/**
 * 力施加模块
 * 
 * 职责：
 * - 将最终力施加到车体主刚体
 * - 使用 AddForceAtLocation 在接触点施加力
 * - 不直接施加到支撑臂局部刚体
 * 
 * 实现要点：
 * - 对每个 FFinalForce，若 bShouldApply 为真，则施加力
 * - 调用 RootPrimitive->AddForceAtLocation(Force, ApplicationPoint)
 * - 所有力施加到车体主刚体，由物理引擎传递到支撑臂
 */
UCLASS()
class P_FLIPPERSUPPORT_API UFlipperForceApplicator : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * 初始化力施加器
	 * @param InRootPrimitive 车体主刚体引用，接收所有计算出的力
	 */
	void Initialize(UPrimitiveComponent* InRootPrimitive);

	/**
	 * 施加力到车体主刚体
	 * @param FinalForces 最终力数组，包含所有需要施加的力
	 */
	void ApplyForces(const TArray<FFinalForce>& FinalForces);

private:
	// ===== 外部引用 =====

	/** 车体主刚体引用，接收所有计算出的力 */
	UPROPERTY()
	UPrimitiveComponent* RootPrimitive = nullptr;
};
