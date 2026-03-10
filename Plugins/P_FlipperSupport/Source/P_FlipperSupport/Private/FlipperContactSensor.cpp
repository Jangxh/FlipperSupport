// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlipperContactSensor.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"

void UFlipperContactSensor::Initialize(UPrimitiveComponent* InRootPrimitive,
                                       const TArray<UPrimitiveComponent*>& InFlipperPrimitives)
{
	RootPrimitive = InRootPrimitive;
	FlipperPrimitives = InFlipperPrimitives;

	PreviousPatches.SetNum(4);
	ContactHints.SetNum(4);

	for (int32 i = 0; i < 4; ++i)
	{
		PreviousPatches[i].FlipperIndex = static_cast<EFlipperIndex>(i);
		PreviousPatches[i].bIsValid = false;
		PreviousPatches[i].Confidence = 0.0f;
		PreviousPatches[i].bIsStable = false;
		PreviousPatches[i].bHasLastValidContact = false;
		PreviousPatches[i].TimeSinceLastValidContact = 999.0f;
		ContactHints[i].bHasHistory = false;
	}

	UE_LOG(LogTemp, Log, TEXT("FlipperContactSensor: Initialized with %d flipper primitives"), FlipperPrimitives.Num());
}

void UFlipperContactSensor::DetectContacts(float DeltaTime, TArray<FContactPatchData>& OutContactPatches)
{
	OutContactPatches.SetNum(4);

	for (int32 i = 0; i < 4; ++i)
	{
		FContactPatchData& CurrentPatch = OutContactPatches[i];
		CurrentPatch.FlipperIndex = static_cast<EFlipperIndex>(i);

		if (i < PreviousPatches.Num())
		{
			CurrentPatch.Confidence = PreviousPatches[i].Confidence;
			CurrentPatch.PreviousForce = PreviousPatches[i].PreviousForce;
			CurrentPatch.bHasLastValidContact = PreviousPatches[i].bHasLastValidContact;
			CurrentPatch.LastValidContactPoint = PreviousPatches[i].LastValidContactPoint;
			CurrentPatch.LastValidContactNormal = PreviousPatches[i].LastValidContactNormal;
			CurrentPatch.TimeSinceLastValidContact = PreviousPatches[i].TimeSinceLastValidContact;
		}

		FHitResult HitResult;
		const bool bDetected = DetectSingleFlipperContact(i, HitResult);

		if (bDetected)
		{
			CurrentPatch.bIsValid = true;
			CurrentPatch.ContactPoint = HitResult.ImpactPoint;
			CurrentPatch.ContactNormal = HitResult.ImpactNormal;

			const float ActualGap = HitResult.bStartPenetrating
				? -HitResult.PenetrationDepth
				: HitResult.Distance;

			CurrentPatch.ContactMetric = FMath::Max(0.0f, TargetContactGap - ActualGap);

			if (CurrentPatch.ContactMetric < ContactMetricMinThreshold)
			{
				CurrentPatch.bIsValid = false;
				CurrentPatch.ContactMetric = 0.0f;
				CurrentPatch.TimeSinceLastValidContact += DeltaTime;
			}
			else
			{
				CurrentPatch.bHasLastValidContact = true;
				CurrentPatch.LastValidContactPoint = HitResult.ImpactPoint;
				CurrentPatch.LastValidContactNormal = HitResult.ImpactNormal;
				CurrentPatch.TimeSinceLastValidContact = 0.0f;
			}
		}
		else
		{
			CurrentPatch.bIsValid = false;
			CurrentPatch.ContactMetric = 0.0f;
			CurrentPatch.TimeSinceLastValidContact += DeltaTime;
			
			if (CurrentPatch.bHasLastValidContact && FlipperPrimitives.IsValidIndex(i) && FlipperPrimitives[i])
			{
				const FVector FlipperTipPos = FlipperPrimitives[i]->GetComponentLocation();
				const float Drift = FVector::Dist(CurrentPatch.LastValidContactPoint, FlipperTipPos);
				// 若接触点距臂端超过探测距离的2倍，说明臂已经离开了，作废历史
				if (Drift > DetectionSweepDistance * 2.0f)
				{
					CurrentPatch.bHasLastValidContact = false;
					CurrentPatch.TimeSinceLastValidContact = 999.0f;
				}
			}
		}

		UpdateConfidence(CurrentPatch, CurrentPatch.bIsValid, DeltaTime);

		if (CurrentPatch.bIsValid)
		{
			ComputeContactDynamics(CurrentPatch);
		}
		else
		{
			CurrentPatch.ContactVelocity = FVector::ZeroVector;
			CurrentPatch.TangentialVelocity = FVector::ZeroVector;
		}

		if (RootPrimitive && RootPrimitive->GetWorld())
		{
			CurrentPatch.LastUpdateTime = RootPrimitive->GetWorld()->GetTimeSeconds();
		}
	}

	PreviousPatches = OutContactPatches;
}

void UFlipperContactSensor::SetDetectionRadius(float Radius)
{
	DetectionSweepDistance = FMath::Max(0.1f, Radius);
	TargetContactGap = FMath::Min(TargetContactGap, DetectionSweepDistance * 0.8f);
}

void UFlipperContactSensor::SetTargetContactGap(float Gap)
{
	TargetContactGap = FMath::Clamp(Gap, 0.1f, DetectionSweepDistance * 0.8f);
}

void UFlipperContactSensor::SetContactMetricThresholds(float MinThreshold, float /*MaxThreshold*/)
{
	ContactMetricMinThreshold = FMath::Max(0.0f, MinThreshold);
}

void UFlipperContactSensor::SetConfidenceRates(float IncreaseRate, float DecreaseRate)
{
	ConfidenceIncreaseRate = FMath::Max(0.0f, IncreaseRate);
	ConfidenceDecreaseRate = FMath::Max(0.0f, DecreaseRate);
}

void UFlipperContactSensor::SetStableConfidenceThreshold(float Threshold)
{
	StableConfidenceThreshold = FMath::Clamp(Threshold, 0.0f, 1.0f);
}

void UFlipperContactSensor::SetContactDriftTolerances(float PointDrift, float NormalDot)
{
	ContactPointDriftTolerance = FMath::Max(1.0f, PointDrift);
	ContactNormalDotTolerance = FMath::Clamp(NormalDot, 0.0f, 1.0f);
}

bool UFlipperContactSensor::SweepInDirection(UPrimitiveComponent* FlipperPrimitive,
                                             const FVector& Direction,
                                             float Distance,
                                             const FComponentQueryParams& QueryParams,
                                             FHitResult& OutHit) const
{
	if (!FlipperPrimitive)
	{
		return false;
	}

	UWorld* World = FlipperPrimitive->GetWorld();
	if (!World)
	{
		return false;
	}

	const FVector Start = FlipperPrimitive->GetComponentLocation();
	const FVector End = Start + Direction * Distance;
	const FQuat Rotation = FlipperPrimitive->GetComponentQuat();

	TArray<FHitResult> Hits;
	bool bHit = World->ComponentSweepMulti(Hits, FlipperPrimitive, Start, End, Rotation, QueryParams);
	if (!bHit || Hits.Num() == 0)
	{
		return false;
	}

	int32 BestIdx = 0;
	float BestScore = -FLT_MAX;
	for (int32 k = 0; k < Hits.Num(); ++k)
	{
		const float Score = Hits[k].bStartPenetrating ? Hits[k].PenetrationDepth : -Hits[k].Distance;
		if (Score > BestScore)
		{
			BestScore = Score;
			BestIdx = k;
		}
	}

	OutHit = Hits[BestIdx];
	return true;
}

bool UFlipperContactSensor::DetectSingleFlipperContact(int32 FlipperIndex, FHitResult& OutHit)
{
	if (!FlipperPrimitives.IsValidIndex(FlipperIndex) || !FlipperPrimitives[FlipperIndex])
	{
		return false;
	}

	UPrimitiveComponent* FlipperPrimitive = FlipperPrimitives[FlipperIndex];

	FComponentQueryParams QueryParams;
	QueryParams.AddIgnoredActor(FlipperPrimitive->GetOwner());
	QueryParams.bFindInitialOverlaps = true;
	QueryParams.bTraceComplex = false;
	QueryParams.bReturnPhysicalMaterial = true;

	const FVector Up = FlipperPrimitive->GetUpVector();
	const FVector Forward = FlipperPrimitive->GetForwardVector();
	const FVector Directions[3] =
	{
		-Up,
		Forward,
		(-Up + Forward).GetSafeNormal()
	};

	FHitResult BestHit;
	float BestScore = -FLT_MAX;
	bool bAnyHit = false;

	for (const FVector& Direction : Directions)
	{
		FHitResult Hit;
		if (SweepInDirection(FlipperPrimitive, Direction, DetectionSweepDistance, QueryParams, Hit))
		{
			const float Score = Hit.bStartPenetrating ? Hit.PenetrationDepth : -Hit.Distance;
			if (Score > BestScore)
			{
				BestScore = Score;
				BestHit = Hit;
				bAnyHit = true;
			}
		}
	}

	if (!bAnyHit)
	{
		ContactHints[FlipperIndex].bHasHistory = false;
		return false;
	}

	FFlipperContactHint& Hint = ContactHints[FlipperIndex];
	if (Hint.bHasHistory)
	{
		const float PointDrift = FVector::Dist(BestHit.ImpactPoint, Hint.LastContactPoint);
		const float NormalDot = FVector::DotProduct(BestHit.ImpactNormal, Hint.LastContactNormal);
		if (PointDrift > ContactPointDriftTolerance && NormalDot < ContactNormalDotTolerance)
		{
			return false;
		}
	}

	Hint.LastContactPoint = BestHit.ImpactPoint;
	Hint.LastContactNormal = BestHit.ImpactNormal;
	Hint.bHasHistory = true;

	OutHit = BestHit;
	return true;
}

void UFlipperContactSensor::UpdateConfidence(FContactPatchData& Patch, bool bDetected, float DeltaTime)
{
	if (bDetected)
	{
		Patch.Confidence += ConfidenceIncreaseRate * DeltaTime;
		Patch.Confidence = FMath::Clamp(Patch.Confidence, 0.0f, 1.0f);
	}
	else
	{
		Patch.Confidence -= ConfidenceDecreaseRate * DeltaTime;
		Patch.Confidence = FMath::Clamp(Patch.Confidence, 0.0f, 1.0f);
	}

	Patch.bIsStable = (Patch.Confidence >= StableConfidenceThreshold);
}

void UFlipperContactSensor::ComputeContactDynamics(FContactPatchData& Patch)
{
	const int32 Index = static_cast<int32>(Patch.FlipperIndex);
	UPrimitiveComponent* SourcePrimitive =
		(FlipperPrimitives.IsValidIndex(Index) && FlipperPrimitives[Index])
		? FlipperPrimitives[Index].Get()
		: RootPrimitive.Get();

	if (!SourcePrimitive)
	{
		Patch.ContactVelocity = FVector::ZeroVector;
		Patch.TangentialVelocity = FVector::ZeroVector;
		return;
	}

	const FVector LinearVelocity = SourcePrimitive->GetPhysicsLinearVelocity();
	const FVector AngularVelocity = SourcePrimitive->GetPhysicsAngularVelocityInRadians();
	const FVector CenterOfMass = SourcePrimitive->GetCenterOfMass();
	const FVector LeverArm = Patch.ContactPoint - CenterOfMass;

	Patch.ContactVelocity = LinearVelocity + FVector::CrossProduct(AngularVelocity, LeverArm);
	const float NormalComponent = FVector::DotProduct(Patch.ContactVelocity, Patch.ContactNormal);
	Patch.TangentialVelocity = Patch.ContactVelocity - NormalComponent * Patch.ContactNormal;
}
