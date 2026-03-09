// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlipperAntiSlipSolver.h"

FVector UFlipperAntiSlipSolver::ComputeForce(const FContactPatchData& Patch, const FFlipperVehicleState& VehicleState)
{
	// 若接触片无效，返回零力
	if (!Patch.bIsValid)
	{
		return FVector::ZeroVector;
	}

	// 获取切向速度（接触平面内，cm/s）
	// 切向速度已在 ContactSensor 中计算：V_tangential = V_contact - (V_contact · N) * N
	const FVector& TangentialVelocity = Patch.TangentialVelocity;

	// 计算切向速度大小
	const float TangentialSpeed = TangentialVelocity.Size();

	// 应用速度死区检查
	// 若切向速度低于死区阈值，不施加防滑力，避免在极低滑动速度时产生过大的力
	if (TangentialSpeed < AntiSlipVelocityDeadband)
	{
		return FVector::ZeroVector;
	}

	// 计算防滑力方向（与切向速度相反）
	// 使用 GetSafeNormal() 确保在速度接近零时不会产生无效的单位矢量
	const FVector AntiSlipDirection = -TangentialVelocity.GetSafeNormal();

	// 计算防滑力大小：F_antislip = Gain * |V_tangential|
	// 防滑力与切向速度大小成比例，速度越大，防滑力越大
	float AntiSlipForceMagnitude = AntiSlipForceGain * TangentialSpeed;

	// 限制到 MaxAntiSlipForce
	AntiSlipForceMagnitude = FMath::Min(AntiSlipForceMagnitude, MaxAntiSlipForce);

	// 返回防滑力矢量
	// 防滑力完全位于接触平面内，方向与切向速度相反，用于抵消滑动趋势
	const FVector AntiSlipForce = AntiSlipDirection * AntiSlipForceMagnitude;

	// 验证力完全位于接触平面内（调试用，可选）
	// 理论上 AntiSlipForce 应该与 ContactNormal 垂直，即 Dot(AntiSlipForce, ContactNormal) ≈ 0
	// 由于 TangentialVelocity 已经是投影到接触平面的结果，这里的验证应该总是通过
#if !UE_BUILD_SHIPPING
	const float NormalComponent = FVector::DotProduct(AntiSlipForce, Patch.ContactNormal);
	if (FMath::Abs(NormalComponent) > 0.01f * AntiSlipForceMagnitude)
	{
		UE_LOG(LogTemp, Warning, TEXT("FlipperAntiSlipSolver: AntiSlipForce has non-negligible normal component: %.2f"), NormalComponent);
	}
#endif

	return AntiSlipForce;
}

void UFlipperAntiSlipSolver::SetMaxForce(float MaxForce)
{
	// 设置最大防滑力，确保为非负值
	MaxAntiSlipForce = FMath::Max(0.0f, MaxForce);
}

void UFlipperAntiSlipSolver::SetGain(float Gain)
{
	// 设置防滑力增益，确保为非负值
	AntiSlipForceGain = FMath::Max(0.0f, Gain);
}

void UFlipperAntiSlipSolver::SetVelocityDeadband(float Deadband)
{
	// 设置速度死区，确保为非负值
	AntiSlipVelocityDeadband = FMath::Max(0.0f, Deadband);
}
