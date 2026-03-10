// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlipperForceComposer.h"

void UFlipperForceComposer::ComposeForces(const TArray<FContactPatchData>& ContactPatches,
                                          const TArray<FVector>& SupportForces,
                                          const TArray<FVector>& AntiSlipForces,
                                          const TArray<FVector>& TractionAssistForces,
                                          TArray<FCandidateForce>& OutCandidateForces)
{
	OutCandidateForces.Reset();

	const int32 NumPatches = ContactPatches.Num();
	if (SupportForces.Num() != NumPatches ||
		AntiSlipForces.Num() != NumPatches ||
		TractionAssistForces.Num() != NumPatches)
	{
		UE_LOG(LogTemp, Warning, TEXT("FlipperForceComposer: 输入数组大小不一致，跳过力组合"));
		return;
	}

	for (int32 i = 0; i < NumPatches; ++i)
	{
		const FContactPatchData& Patch = ContactPatches[i];

		FCandidateForce CandidateForce;
		CandidateForce.FlipperIndex = Patch.FlipperIndex;
		CandidateForce.ApplicationPoint = Patch.bIsValid ? Patch.ContactPoint : Patch.LastValidContactPoint;

		const FVector& InSupport = SupportForces[i];
		const FVector& InAntiSlip = AntiSlipForces[i];
		const FVector& InTraction = TractionAssistForces[i];
		const bool bHasAnyForce = !InSupport.IsNearlyZero() || !InAntiSlip.IsNearlyZero() || !InTraction.IsNearlyZero();

		if (!bHasAnyForce)
		{
			CandidateForce.bIsValid = false;
			CandidateForce.SupportForce = FVector::ZeroVector;
			CandidateForce.AntiSlipForce = FVector::ZeroVector;
			CandidateForce.TractionAssistForce = FVector::ZeroVector;
			CandidateForce.TotalForce = FVector::ZeroVector;
			OutCandidateForces.Add(CandidateForce);
			continue;
		}

		CandidateForce.SupportForce = InSupport;
		CandidateForce.AntiSlipForce = InAntiSlip;
		CandidateForce.TractionAssistForce = InTraction;

		// 根据接触面陡峭程度计算有效摩擦系数
		// GroundnessMetric = 1：平地（法线朝上），使用基础摩擦系数
		// GroundnessMetric = 0：垂直障碍（法线水平），应用爬坡摩擦增益
		// 物理依据：支撑臂对垂直障碍施力时，驱动力来自电机扭矩（非纯接触摩擦），允许更高切向力
		const float GroundnessMetric = FMath::Max(0.0f, FVector::DotProduct(Patch.ContactNormal, FVector::UpVector));
		const float ObstacleSteepness = 1.0f - GroundnessMetric;
		const float EffectiveMu = FrictionCoefficient * (1.0f + ObstacleClimbFrictionMultiplier * ObstacleSteepness);

		ApplyFrictionConeConstraint(CandidateForce, EffectiveMu);
		CandidateForce.bIsValid = true;

		OutCandidateForces.Add(CandidateForce);
	}
}

void UFlipperForceComposer::SetFrictionCoefficient(float Mu)
{
	FrictionCoefficient = FMath::Max(0.0f, Mu);
}

void UFlipperForceComposer::SetObstacleClimbFrictionMultiplier(float Multiplier)
{
	ObstacleClimbFrictionMultiplier = FMath::Max(0.0f, Multiplier);
}

void UFlipperForceComposer::ApplyFrictionConeConstraint(FCandidateForce& Force, float EffectiveMu)
{
	const float Fn = Force.SupportForce.Size();
	FVector Ft = Force.AntiSlipForce + Force.TractionAssistForce;
	const float FtMag = Ft.Size();
	const float MaxTangentialForce = EffectiveMu * Fn;

	if (FtMag > MaxTangentialForce && FtMag > KINDA_SMALL_NUMBER)
	{
		const float ScaleFactor = MaxTangentialForce / FtMag;
		Ft *= ScaleFactor;
		Force.AntiSlipForce *= ScaleFactor;
		Force.TractionAssistForce *= ScaleFactor;
	}

	Force.TotalForce = Force.SupportForce + Ft;
}
