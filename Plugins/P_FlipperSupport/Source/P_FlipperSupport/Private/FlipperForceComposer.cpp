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

		ApplyFrictionConeConstraint(CandidateForce);
		CandidateForce.bIsValid = true;

		OutCandidateForces.Add(CandidateForce);
	}
}

void UFlipperForceComposer::SetFrictionCoefficient(float Mu)
{
	FrictionCoefficient = FMath::Max(0.0f, Mu);
}

void UFlipperForceComposer::ApplyFrictionConeConstraint(FCandidateForce& Force)
{
	const float Fn = Force.SupportForce.Size();
	FVector Ft = Force.AntiSlipForce + Force.TractionAssistForce;
	const float FtMag = Ft.Size();
	const float MaxTangentialForce = FrictionCoefficient * Fn;

	if (FtMag > MaxTangentialForce && FtMag > KINDA_SMALL_NUMBER)
	{
		const float ScaleFactor = MaxTangentialForce / FtMag;
		Ft *= ScaleFactor;
		Force.AntiSlipForce *= ScaleFactor;
		Force.TractionAssistForce *= ScaleFactor;
	}

	Force.TotalForce = Force.SupportForce + Ft;
}
