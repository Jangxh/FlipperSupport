// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlipperTractionAssistSolver.h"

FVector UFlipperTractionAssistSolver::ComputeForce(const FContactPatchData& Patch, const FFlipperVehicleState& VehicleState)
{
	// 若接触片无效，返回零力
	if (!Patch.bIsValid)
	{
		return FVector::ZeroVector;
	}

	// 检查是否满足牵引辅助条件
	if (!ShouldProvideTractionAssist(Patch, VehicleState))
	{
		return FVector::ZeroVector;
	}

	// 计算车辆纵向方向（考虑油门符号）
	// 油门为正时前进，为负时后退
	const FVector LongitudinalDirection = VehicleState.ForwardVector * FMath::Sign(VehicleState.ThrottleInput);

	// 投影纵向方向到接触平面
	// 接触平面由接触法线定义，投影公式：V_projected = V - (V · N) * N
	const FVector& ContactNormal = Patch.ContactNormal;
	const FVector ProjectedDirection = (LongitudinalDirection - FVector::DotProduct(LongitudinalDirection, ContactNormal) * ContactNormal).GetSafeNormal();

	// 若投影后方向无效（例如纵向方向与法线平行），返回零力
	if (ProjectedDirection.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	// 计算牵引力大小：由油门、轮地接触缺失程度、接触稳定性共同调制
	// ContactDeficit = 1.0 - WheelGroundContactRatio，表示轮地接触缺失的程度
	// 轮地接触越少，支撑臂需要提供的牵引辅助越多
	const float ContactDeficit = 1.0f - VehicleState.WheelGroundContactRatio;

	// F_traction = ThrottleGain * |Throttle| * ContactDeficit * Confidence
	// - ThrottleGain: 油门增益，控制牵引力的基础强度
	// - |Throttle|: 油门输入的绝对值，油门越大，牵引力越大
	// - ContactDeficit: 轮地接触缺失程度，轮地接触越少，牵引力越大
	// - Confidence: 接触稳定性，接触越稳定，牵引力越可靠
	float TractionForceMagnitude = TractionAssistThrottleGain * FMath::Abs(VehicleState.ThrottleInput) * ContactDeficit * Patch.Confidence;

	// 限制到 MaxTractionAssistForce
	TractionForceMagnitude = FMath::Min(TractionForceMagnitude, MaxTractionAssistForce);

	// 返回牵引辅助力矢量
	// 牵引辅助力沿投影后的纵向方向，完全位于接触平面内
	const FVector TractionAssistForce = ProjectedDirection * TractionForceMagnitude;

	// 验证力完全位于接触平面内（调试用，可选）
	// 理论上 TractionAssistForce 应该与 ContactNormal 垂直，即 Dot(TractionAssistForce, ContactNormal) ≈ 0
#if !UE_BUILD_SHIPPING
	const float NormalComponent = FVector::DotProduct(TractionAssistForce, ContactNormal);
	if (FMath::Abs(NormalComponent) > 0.01f * TractionForceMagnitude)
	{
		UE_LOG(LogTemp, Warning, TEXT("FlipperTractionAssistSolver: TractionAssistForce has non-negligible normal component: %.2f"), NormalComponent);
	}
#endif

	return TractionAssistForce;
}

bool UFlipperTractionAssistSolver::ShouldProvideTractionAssist(const FContactPatchData& Patch, const FFlipperVehicleState& VehicleState) const
{
	// 条件 1: 轮地接触比例低于阈值
	// 只有在轮地接触不足时才需要支撑臂提供牵引辅助
	if (VehicleState.WheelGroundContactRatio >= WheelContactRatioThreshold)
	{
		return false;
	}

	// 条件 2: 偏航输入小于阈值
	// 在转向时不提供牵引辅助，避免干扰转向操作
	if (FMath::Abs(VehicleState.YawInput) >= YawInputThreshold)
	{
		return false;
	}

	// 条件 3: 油门输入大于阈值
	// 只有在驾驶员有明确的前进或后退意图时才提供牵引辅助
	if (FMath::Abs(VehicleState.ThrottleInput) <= ThrottleInputThreshold)
	{
		return false;
	}

	// 条件 4: 接触片稳定
	// 只有在接触稳定时才提供牵引辅助，避免在接触不稳定时产生不可预测的力
	if (!Patch.bIsStable)
	{
		return false;
	}

	// 所有条件满足，应该提供牵引辅助
	return true;
}

void UFlipperTractionAssistSolver::SetMaxForce(float MaxForce)
{
	// 设置最大牵引辅助力，确保为非负值
	MaxTractionAssistForce = FMath::Max(0.0f, MaxForce);
}

void UFlipperTractionAssistSolver::SetThrottleGain(float Gain)
{
	// 设置油门增益，确保为非负值
	TractionAssistThrottleGain = FMath::Max(0.0f, Gain);
}

void UFlipperTractionAssistSolver::SetConditionThresholds(float WheelContactThreshold, float YawThreshold, float ThrottleThreshold)
{
	// 设置轮地接触比例阈值，限制在 [0, 1] 范围内
	WheelContactRatioThreshold = FMath::Clamp(WheelContactThreshold, 0.0f, 1.0f);

	// 设置偏航输入阈值，限制在 [0, 1] 范围内
	YawInputThreshold = FMath::Clamp(YawThreshold, 0.0f, 1.0f);

	// 设置油门输入阈值，限制在 [0, 1] 范围内
	ThrottleInputThreshold = FMath::Clamp(ThrottleThreshold, 0.0f, 1.0f);
}
