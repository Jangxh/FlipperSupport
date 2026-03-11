// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlipperForceApplicator.h"
#include "Components/PrimitiveComponent.h"

void UFlipperForceApplicator::Initialize(UPrimitiveComponent* InRootPrimitive,
                                         const TArray<UPrimitiveComponent*>& InFlipperPrimitives)
{
	RootPrimitive = InRootPrimitive;
	FlipperPrimitives = InFlipperPrimitives;

	if (!RootPrimitive)
	{
		UE_LOG(LogTemp, Error, TEXT("FlipperForceApplicator::Initialize - RootPrimitive is null"));
	}
}

void UFlipperForceApplicator::ApplyForces(const TArray<FFinalForce>& FinalForces)
{
	for (const FFinalForce& FinalForce : FinalForces)
	{
		if (!FinalForce.bShouldApply)
		{
			continue;
		}

		const int32 Index = static_cast<int32>(FinalForce.FlipperIndex);
		UPrimitiveComponent* Target =
			(FlipperPrimitives.IsValidIndex(Index) && FlipperPrimitives[Index])
			? FlipperPrimitives[Index].Get()
			: RootPrimitive.Get();

		if (!Target)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("FlipperForceApplicator: No valid target for FlipperIndex %d"), Index);
			continue;
		}

		Target->AddForceAtLocation(FinalForce.Force, FinalForce.ApplicationPoint);
	}
}
