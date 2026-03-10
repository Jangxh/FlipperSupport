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

	const FVector& ContactNormal = Patch.ContactNormal;

	// 计算接触面的"地面程度"：法线与世界上方的点积
	// GroundnessMetric = 1：完全平地（法线朝上）
	// GroundnessMetric = 0：垂直障碍物（法线水平）
	const float GroundnessMetric = FMath::Max(0.0f, FVector::DotProduct(ContactNormal, FVector::UpVector));

	// 计算期望运动方向，根据障碍陡峭程度混入向上分量：
	// - 平地时：UpwardBias = 0，DesiredDirection ≈ ForwardVector → 投影后为前向牵引
	// - 垂直障碍时：UpwardBias = ClimbAssistUpwardBias，DesiredDirection = (Forward+Up).norm
	//   → 投影到垂直接触平面后 ≈ UpVector（即向上的抬升力）
	// 这修复了垂直障碍时纯前向投影 = 零向量的根本缺陷
	const float UpwardBias = ClimbAssistUpwardBias * (1.0f - GroundnessMetric);
	const FVector DesiredDirection = (LongitudinalDirection + FVector::UpVector * UpwardBias).GetSafeNormal();

	// 投影期望方向到接触平面
	// 投影公式：V_projected = V - (V · N) * N
	const FVector ProjectedDirection = (DesiredDirection - FVector::DotProduct(DesiredDirection, ContactNormal) * ContactNormal).GetSafeNormal();

	// 若投影后方向无效（极端情况），返回零力
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

	// 条件 4: 接触置信度高于最低阈值
	// 使用置信度阈值替代 bIsStable，允许接触建立初期（PreStable 阶段）也提供牵引辅助。
	// 置信度已通过力大小公式（Confidence 因子）自然调制，因此低置信度时力也较小，不会产生突变。
	if (Patch.Confidence < MinTractionConfidenceThreshold)
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

void UFlipperTractionAssistSolver::SetClimbAssistParams(float UpwardBias, float MinConfidence)
{
	// 设置越障爬坡偏置系数，限制在 [0, 1] 范围内
	ClimbAssistUpwardBias = FMath::Clamp(UpwardBias, 0.0f, 1.0f);

	// 设置最低置信度阈值，限制在 [0, 1] 范围内
	MinTractionConfidenceThreshold = FMath::Clamp(MinConfidence, 0.0f, 1.0f);
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
