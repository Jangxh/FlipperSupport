// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlipperSupportSolver.h"

FVector UFlipperSupportSolver::ComputeForce(const FContactPatchData& Patch, const FFlipperVehicleState& VehicleState)
{
	// 若接触片无效，返回零力
	if (!Patch.bIsValid)
	{
		return FVector::ZeroVector;
	}

	// 获取接触法线（单位矢量，指向远离地面）
	const FVector& ContactNormal = Patch.ContactNormal;

	// 计算法向相对速度：Vn = Dot(ContactVelocity, N)
	// 正值表示接触点正在远离地面，负值表示正在接近地面
	const float NormalVelocity = FVector::DotProduct(Patch.ContactVelocity, ContactNormal);

	// 计算支撑力大小：F_support = Stiffness * ContactMetric - Damping * Vn
	// ContactMetric 是非负接触强度度量，值越大表示接触参与程度越强
	// 刚度项提供基于接触强度的支撑力
	// 阻尼项用于减少接触切换时的振荡，当接触点接近地面时增加支撑力，远离时减少支撑力
	float SupportForceMagnitude = SupportForceStiffness * Patch.ContactMetric - SupportForceDamping * NormalVelocity;

	// 限制支撑力到 [0, MaxSupportForce]
	// 支撑力只能向远离地面方向推动（正法线方向），不能拉向地面
	SupportForceMagnitude = FMath::Clamp(SupportForceMagnitude, 0.0f, MaxSupportForce);

	// 返回沿法线方向的力矢量
	// 支撑力方向始终沿接触法线，指向远离地面
	return ContactNormal * SupportForceMagnitude;
}

void UFlipperSupportSolver::SetMaxForce(float MaxForce)
{
	// 设置最大支撑力，确保为非负值
	MaxSupportForce = FMath::Max(0.0f, MaxForce);
}

void UFlipperSupportSolver::SetStiffness(float Stiffness)
{
	// 设置支撑力刚度系数，确保为非负值
	SupportForceStiffness = FMath::Max(0.0f, Stiffness);
}

void UFlipperSupportSolver::SetDamping(float Damping)
{
	// 设置支撑力阻尼系数，确保为非负值
	SupportForceDamping = FMath::Max(0.0f, Damping);
}
