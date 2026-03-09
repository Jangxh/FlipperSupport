// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlipperForceSafetyFilter.h"

void UFlipperForceSafetyFilter::Initialize(UPrimitiveComponent* InRootPrimitive)
{
	RootPrimitive = InRootPrimitive;

	if (!RootPrimitive)
	{
		UE_LOG(LogTemp, Warning, TEXT("FlipperForceSafetyFilter: RootPrimitive 引用无效"));
	}
}

void UFlipperForceSafetyFilter::FilterForces(const TArray<FCandidateForce>& CandidateForces,
                                              const TArray<FContactPatchData>& ContactPatches,
                                              const FFlipperVehicleState& VehicleState,
                                              float DeltaTime,
                                              TArray<FFinalForce>& OutFinalForces)
{
	// 清空输出数组
	OutFinalForces.Reset();

	// 验证输入数组大小一致性
	if (CandidateForces.Num() != ContactPatches.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("FlipperForceSafetyFilter: 输入数组大小不一致，跳过力过滤"));
		return;
	}

	// 复制候选力数组用于逐步过滤
	TArray<FCandidateForce> FilteredForces = CandidateForces;

	// 步骤 1: 接触平面裁剪
	ApplyContactPlaneClipping(FilteredForces, ContactPatches);

	// 步骤 2: 底盘接触降额
	ApplyChassisContactReduction(FilteredForces, VehicleState);

	// 步骤 3: 俯仰/横滚力矩限幅
	ApplyMomentLimiting(FilteredForces, VehicleState);

	// 步骤 4: 方向和大小平滑
	ApplySmoothingAndJerkLimiting(FilteredForces, ContactPatches, DeltaTime);

	// 步骤 5: 生成最终力数组
	for (const FCandidateForce& Force : FilteredForces)
	{
		FFinalForce FinalForce;
		FinalForce.FlipperIndex = Force.FlipperIndex;
		FinalForce.Force = Force.TotalForce;
		FinalForce.ApplicationPoint = Force.ApplicationPoint;
		FinalForce.bShouldApply = Force.bIsValid;

		OutFinalForces.Add(FinalForce);
	}
}

void UFlipperForceSafetyFilter::DecayForces(const TArray<FContactPatchData>& ContactPatches,
                                             float DeltaTime,
                                             TArray<FFinalForce>& OutFinalForces)
{
	// 清空输出数组
	OutFinalForces.Reset();

	// 对每个接触片应用力衰减
	for (const FContactPatchData& Patch : ContactPatches)
	{
		FFinalForce FinalForce;
		FinalForce.FlipperIndex = Patch.FlipperIndex;
		FinalForce.ApplicationPoint = Patch.ContactPoint;
		FinalForce.bShouldApply = false; // 衰减时不施加力

		// 若接触片无效或不稳定，应用指数衰减
		if (!Patch.bIsValid || !Patch.bIsStable)
		{
			// 指数衰减：Force *= exp(-DecayRate * DeltaTime)
			const float DecayFactor = FMath::Exp(-ForceDecayRate * DeltaTime);
			FinalForce.Force = Patch.PreviousForce * DecayFactor;

			// 当力大小小于阈值时归零
			const float ForceThreshold = 0.1f; // 力大小阈值（UE 力单位）
			if (FinalForce.Force.Size() < ForceThreshold)
			{
				FinalForce.Force = FVector::ZeroVector;
			}
		}
		else
		{
			// 有效且稳定的接触片保持上一帧的力
			FinalForce.Force = Patch.PreviousForce;
		}

		OutFinalForces.Add(FinalForce);
	}
}

void UFlipperForceSafetyFilter::SetChassisContactReductionFactor(float Factor)
{
	ChassisContactReductionFactor = FMath::Clamp(Factor, 0.0f, 1.0f);
}

void UFlipperForceSafetyFilter::SetMomentLimits(float InMaxPitchTorque, float InMaxRollTorque)
{
	this->MaxPitchTorque = FMath::Max(0.0f, InMaxPitchTorque);
	this->MaxRollTorque = FMath::Max(0.0f, InMaxRollTorque);
}

void UFlipperForceSafetyFilter::SetSmoothingParameters(float DirectionSmoothingRate, float JerkLimit)
{
	ForceDirectionSmoothingRate = FMath::Max(0.0f, DirectionSmoothingRate);
	ForceJerkLimit = FMath::Max(0.0f, JerkLimit);
}

void UFlipperForceSafetyFilter::SetForceDecayRate(float DecayRate)
{
	ForceDecayRate = FMath::Max(0.0f, DecayRate);
}

void UFlipperForceSafetyFilter::ApplyContactPlaneClipping(TArray<FCandidateForce>& Forces,
                                                           const TArray<FContactPatchData>& Patches)
{
	// 验证数组大小一致性
	if (Forces.Num() != Patches.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("FlipperForceSafetyFilter: ApplyContactPlaneClipping 输入数组大小不一致"));
		return;
	}

	// 对每个力进行接触平面裁剪
	for (int32 i = 0; i < Forces.Num(); ++i)
	{
		FCandidateForce& Force = Forces[i];
		const FContactPatchData& Patch = Patches[i];

		// 仅处理有效的力
		if (!Force.bIsValid)
		{
			continue;
		}

		const FVector& ContactNormal = Patch.ContactNormal;

		// 1. 检查支撑力是否指向地面内部
		// 支撑力应沿法线方向（远离地面），若指向地面内部则归零
		const float SupportDotNormal = FVector::DotProduct(Force.SupportForce, ContactNormal);
		if (SupportDotNormal < 0.0f)
		{
			// 支撑力指向地面内部，归零
			Force.SupportForce = FVector::ZeroVector;
		}

		// 2. 投影切向力到接触平面
		// 切向力 = AntiSlipForce + TractionAssistForce
		// 投影公式：Ft_clipped = Ft - (Ft · N) * N
		
		// 投影防滑力到接触平面
		const float AntiSlipDotNormal = FVector::DotProduct(Force.AntiSlipForce, ContactNormal);
		Force.AntiSlipForce = Force.AntiSlipForce - AntiSlipDotNormal * ContactNormal;

		// 投影牵引辅助力到接触平面
		const float TractionDotNormal = FVector::DotProduct(Force.TractionAssistForce, ContactNormal);
		Force.TractionAssistForce = Force.TractionAssistForce - TractionDotNormal * ContactNormal;

		// 3. 重新计算总力
		Force.TotalForce = Force.SupportForce + Force.AntiSlipForce + Force.TractionAssistForce;
	}
}

void UFlipperForceSafetyFilter::ApplyChassisContactReduction(TArray<FCandidateForce>& Forces,
                                                              const FFlipperVehicleState& VehicleState)
{
	// 仅在底盘接触时应用降额
	if (!VehicleState.bChassisContact)
	{
		return;
	}

	// 对每个力的牵引辅助分量应用降额因子
	for (FCandidateForce& Force : Forces)
	{
		// 仅处理有效的力
		if (!Force.bIsValid)
		{
			continue;
		}

		// 对牵引辅助力应用降额因子
		// 不影响支撑力和防滑力
		Force.TractionAssistForce *= ChassisContactReductionFactor;

		// 重新计算总力
		Force.TotalForce = Force.SupportForce + Force.AntiSlipForce + Force.TractionAssistForce;
	}
}

void UFlipperForceSafetyFilter::ApplyMomentLimiting(TArray<FCandidateForce>& Forces,
                                                     const FFlipperVehicleState& VehicleState)
{
	// 计算所有力关于质心的总扭矩
	FVector TotalTorque = FVector::ZeroVector;

	for (const FCandidateForce& Force : Forces)
	{
		// 仅处理有效的力
		if (!Force.bIsValid)
		{
			continue;
		}

		// 计算力臂：r = ApplicationPoint - CenterOfMass
		const FVector LeverArm = Force.ApplicationPoint - VehicleState.CenterOfMass;

		// 计算扭矩：Torque = r × F
		const FVector Torque = FVector::CrossProduct(LeverArm, Force.TotalForce);

		TotalTorque += Torque;
	}

	// 提取俯仰和横滚分量
	// 俯仰扭矩：绕车辆右向轴（RightVector）
	const float PitchTorque = FVector::DotProduct(TotalTorque, VehicleState.RightVector);
	const float AbsPitchTorque = FMath::Abs(PitchTorque);

	// 横滚扭矩：绕车辆前向轴（ForwardVector）
	const float RollTorque = FVector::DotProduct(TotalTorque, VehicleState.ForwardVector);
	const float AbsRollTorque = FMath::Abs(RollTorque);

	// 计算缩放因子
	float ScaleFactor = 1.0f;

	// 检查俯仰扭矩是否超限
	if (AbsPitchTorque > MaxPitchTorque && AbsPitchTorque > KINDA_SMALL_NUMBER)
	{
		const float PitchScale = MaxPitchTorque / AbsPitchTorque;
		ScaleFactor = FMath::Min(ScaleFactor, PitchScale);
	}

	// 检查横滚扭矩是否超限
	if (AbsRollTorque > MaxRollTorque && AbsRollTorque > KINDA_SMALL_NUMBER)
	{
		const float RollScale = MaxRollTorque / AbsRollTorque;
		ScaleFactor = FMath::Min(ScaleFactor, RollScale);
	}

	// 若需要缩放，统一缩放所有力
	if (ScaleFactor < 1.0f)
	{
		for (FCandidateForce& Force : Forces)
		{
			if (!Force.bIsValid)
			{
				continue;
			}

			// 缩放所有力分量，保持方向
			Force.SupportForce *= ScaleFactor;
			Force.AntiSlipForce *= ScaleFactor;
			Force.TractionAssistForce *= ScaleFactor;
			Force.TotalForce *= ScaleFactor;
		}
	}
}

void UFlipperForceSafetyFilter::ApplySmoothingAndJerkLimiting(TArray<FCandidateForce>& Forces,
                                                               const TArray<FContactPatchData>& Patches,
                                                               float DeltaTime)
{
	// 验证数组大小一致性
	if (Forces.Num() != Patches.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("FlipperForceSafetyFilter: ApplySmoothingAndJerkLimiting 输入数组大小不一致"));
		return;
	}

	// 避免除零
	if (DeltaTime <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	// 对每个力进行平滑和 jerk 限制
	for (int32 i = 0; i < Forces.Num(); ++i)
	{
		FCandidateForce& Force = Forces[i];
		const FContactPatchData& Patch = Patches[i];

		// 仅处理有效的力
		if (!Force.bIsValid)
		{
			continue;
		}

		// 获取上一帧的力
		const FVector& PreviousForce = Patch.PreviousForce;
		const FVector& CurrentForce = Force.TotalForce;

		// 1. 方向平滑
		FVector SmoothedDirection;
		const float CurrentMag = CurrentForce.Size();
		const float PreviousMag = PreviousForce.Size();

		if (CurrentMag > KINDA_SMALL_NUMBER && PreviousMag > KINDA_SMALL_NUMBER)
		{
			// 两个力都非零，进行方向插值
			const FVector CurrentDir = CurrentForce / CurrentMag;
			const FVector PreviousDir = PreviousForce / PreviousMag;

			// 使用 Lerp 进行方向平滑
			// Alpha = SmoothingRate * DeltaTime，限制在 [0, 1]
			const float Alpha = FMath::Clamp(ForceDirectionSmoothingRate * DeltaTime, 0.0f, 1.0f);
			SmoothedDirection = FMath::Lerp(PreviousDir, CurrentDir, Alpha).GetSafeNormal();
		}
		else if (CurrentMag > KINDA_SMALL_NUMBER)
		{
			// 仅当前力非零，使用当前方向
			SmoothedDirection = CurrentForce / CurrentMag;
		}
		else
		{
			// 当前力为零，使用零矢量
			Force.TotalForce = FVector::ZeroVector;
			continue;
		}

		// 2. Jerk 限制（限制力大小变化率）
		const float MagDelta = CurrentMag - PreviousMag;
		const float MaxMagDelta = ForceJerkLimit * DeltaTime;
		const float ClampedMagDelta = FMath::Clamp(MagDelta, -MaxMagDelta, MaxMagDelta);
		const float FinalMag = PreviousMag + ClampedMagDelta;

		// 3. 计算最终平滑后的力
		Force.TotalForce = SmoothedDirection * FMath::Max(0.0f, FinalMag);
	}
}
