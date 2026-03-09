// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlipperForceApplicator.h"
#include "Components/PrimitiveComponent.h"

void UFlipperForceApplicator::Initialize(UPrimitiveComponent* InRootPrimitive)
{
	// 存储车体主刚体引用
	RootPrimitive = InRootPrimitive;

	// 验证引用有效性
	if (!RootPrimitive)
	{
		UE_LOG(LogTemp, Error, TEXT("FlipperForceApplicator::Initialize - RootPrimitive is null"));
	}
}

void UFlipperForceApplicator::ApplyForces(const TArray<FFinalForce>& FinalForces)
{
	// 防御性检查：确保 RootPrimitive 有效
	if (!RootPrimitive)
	{
		UE_LOG(LogTemp, Warning, TEXT("FlipperForceApplicator::ApplyForces - RootPrimitive is null, cannot apply forces"));
		return;
	}

	// 遍历所有最终力
	for (const FFinalForce& FinalForce : FinalForces)
	{
		// 仅施加标记为应该施加的力
		if (FinalForce.bShouldApply)
		{
			// 施加力到车体主刚体的指定位置
			// AddForceAtLocation 参数：
			// - Force: 力矢量（世界空间，UE 力单位：kg·cm/s²）
			// - Location: 施力点位置（世界空间，cm）
			// - BoneName: 骨骼名称（可选，默认 NAME_None 表示施加到整个刚体）
			RootPrimitive->AddForceAtLocation(FinalForce.Force, FinalForce.ApplicationPoint);
		}
	}
}
