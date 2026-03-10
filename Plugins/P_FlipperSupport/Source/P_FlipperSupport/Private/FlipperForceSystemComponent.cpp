// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlipperForceSystemComponent.h"

#include "C_TankMovementComponent.h"
#include "FlipperContactSensor.h"
#include "VehicleStateProvider.h"
#include "FlipperSupportSolver.h"
#include "FlipperAntiSlipSolver.h"
#include "FlipperTractionAssistSolver.h"
#include "FlipperForceComposer.h"
#include "FlipperForceSafetyFilter.h"
#include "FlipperForceApplicator.h"
#include "Components/PrimitiveComponent.h"
#include "DrawDebugHelpers.h"

UFlipperForceSystemComponent::UFlipperForceSystemComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bEnableSystem = true;
	bEnableDebugVisualization = false;

	ContactPatches.SetNum(4);
	CandidateForces.Reserve(4);
	FinalForces.Reserve(4);
}

void UFlipperForceSystemComponent::BeginPlay()
{
	Super::BeginPlay();

	ResolveAndCacheComponents();
	if (!ValidateReferences())
	{
		UE_LOG(LogTemp, Error, TEXT("FlipperForceSystem: 引用校验失败，禁用系统"));
		bEnableSystem = false;
		return;
	}

	ContactSensor = NewObject<UFlipperContactSensor>(this);
	StateProvider = NewObject<UVehicleStateProvider>(this);
	SupportSolver = NewObject<UFlipperSupportSolver>(this);
	AntiSlipSolver = NewObject<UFlipperAntiSlipSolver>(this);
	TractionAssistSolver = NewObject<UFlipperTractionAssistSolver>(this);
	ForceComposer = NewObject<UFlipperForceComposer>(this);
	SafetyFilter = NewObject<UFlipperForceSafetyFilter>(this);
	ForceApplicator = NewObject<UFlipperForceApplicator>(this);

	InitializeSubmodules();

	UE_LOG(LogTemp, Log, TEXT("FlipperForceSystem: 系统初始化成功"));
}

void UFlipperForceSystemComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                 FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bEnableSystem)
	{
		return;
	}

	if (!RootPrimitive || !TankMovementComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("FlipperForceSystem: 运行时关键引用丢失，禁用系统"));
		bEnableSystem = false;
		return;
	}

	ExecutePipeline(DeltaTime);

	if (bEnableDebugVisualization)
	{
		DrawDebugVisualization();
	}
}

void UFlipperForceSystemComponent::InitializeSubmodules()
{
	ContactSensor->Initialize(RootPrimitive, FlipperPrimitives);
	ContactSensor->SetDetectionRadius(Config.ContactDetectionRadius);
	ContactSensor->SetTargetContactGap(Config.TargetContactGap);
	ContactSensor->SetContactMetricThresholds(Config.ContactMetricMinThreshold, 0.0f);
	ContactSensor->SetConfidenceRates(Config.ConfidenceIncreaseRate, Config.ConfidenceDecreaseRate);
	ContactSensor->SetStableConfidenceThreshold(Config.StableConfidenceThreshold);

	StateProvider->Initialize(RootPrimitive, TankMovementComponent);

	SupportSolver->SetMaxForce(Config.MaxSupportForce);
	SupportSolver->SetStiffness(Config.SupportForceStiffness);
	SupportSolver->SetDamping(Config.SupportForceDamping);

	AntiSlipSolver->SetMaxForce(Config.MaxAntiSlipForce);
	AntiSlipSolver->SetGain(Config.AntiSlipForceGain);
	AntiSlipSolver->SetVelocityDeadband(Config.AntiSlipVelocityDeadband);

	TractionAssistSolver->SetMaxForce(Config.MaxTractionAssistForce);
	TractionAssistSolver->SetThrottleGain(Config.TractionAssistThrottleGain);
	TractionAssistSolver->SetConditionThresholds(
		Config.WheelContactRatioThreshold,
		Config.YawInputThreshold,
		Config.ThrottleInputThreshold);
	TractionAssistSolver->SetClimbAssistParams(
		Config.ClimbAssistUpwardBias,
		Config.MinTractionConfidenceThreshold);

	ForceComposer->SetFrictionCoefficient(Config.FrictionCoefficient);
	ForceComposer->SetObstacleClimbFrictionMultiplier(Config.ObstacleClimbFrictionMultiplier);

	SafetyFilter->Initialize(RootPrimitive);
	SafetyFilter->SetChassisContactReductionFactor(Config.ChassisContactReductionFactor);
	SafetyFilter->SetMomentLimits(Config.MaxPitchTorque, Config.MaxRollTorque);
	SafetyFilter->SetSmoothingParameters(Config.ForceDirectionSmoothingRate, Config.ForceJerkLimit);
	SafetyFilter->SetForceDecayRate(Config.ForceDecayRate);

	ForceApplicator->Initialize(RootPrimitive, FlipperPrimitives);
}

void UFlipperForceSystemComponent::ResolveAndCacheComponents()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		UE_LOG(LogTemp, Error, TEXT("FlipperForceSystem: Owner 为空"));
		return;
	}

	if (RootPrimitiveName.IsNone())
	{
		RootPrimitive = Cast<UPrimitiveComponent>(Owner->GetRootComponent());
		if (RootPrimitive)
		{
			UE_LOG(LogTemp, Log, TEXT("FlipperForceSystem: 自动使用 RootComponent 作为 RootPrimitive"));
		}
	}
	else
	{
		RootPrimitive = FindPrimitiveByName(RootPrimitiveName);
		if (RootPrimitive)
		{
			UE_LOG(LogTemp, Log, TEXT("FlipperForceSystem: 找到 RootPrimitive: %s"), *RootPrimitiveName.ToString());
		}
	}

	if (TankMovementComponentName.IsNone())
	{
		TankMovementComponent = Owner->FindComponentByClass<UC_TankMovementComponent>();
		if (TankMovementComponent)
		{
			UE_LOG(LogTemp, Log, TEXT("FlipperForceSystem: 自动找到 TankMovementComponent"));
		}
	}
	else
	{
		TArray<UC_TankMovementComponent*> MovementComponents;
		Owner->GetComponents<UC_TankMovementComponent>(MovementComponents);
		for (UC_TankMovementComponent* Comp : MovementComponents)
		{
			if (Comp && Comp->GetFName() == TankMovementComponentName)
			{
				TankMovementComponent = Comp;
				UE_LOG(LogTemp, Log, TEXT("FlipperForceSystem: 找到 TankMovementComponent: %s"),
					*TankMovementComponentName.ToString());
				break;
			}
		}
	}

	FlipperPrimitives.Empty();
	FlipperPrimitives.SetNum(4);
	FlipperPrimitives[0] = FindPrimitiveByName(FrontLeftFlipperName);
	FlipperPrimitives[1] = FindPrimitiveByName(FrontRightFlipperName);
	FlipperPrimitives[2] = FindPrimitiveByName(RearLeftFlipperName);
	FlipperPrimitives[3] = FindPrimitiveByName(RearRightFlipperName);

	const TArray<FName> FlipperNames = {
		FrontLeftFlipperName, FrontRightFlipperName,
		RearLeftFlipperName, RearRightFlipperName
	};
	const TArray<FString> FlipperLabels = {
		TEXT("前左"), TEXT("前右"), TEXT("后左"), TEXT("后右")
	};

	for (int32 i = 0; i < 4; ++i)
	{
		if (FlipperPrimitives[i])
		{
			UE_LOG(LogTemp, Log, TEXT("FlipperForceSystem: 找到%s支撑臂: %s"),
				*FlipperLabels[i], *FlipperNames[i].ToString());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("FlipperForceSystem: 未找到%s支撑臂: %s"),
				*FlipperLabels[i], *FlipperNames[i].ToString());
		}
	}
}

UPrimitiveComponent* UFlipperForceSystemComponent::FindPrimitiveByName(const FName& CompName) const
{
	if (CompName.IsNone())
	{
		return nullptr;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	TArray<UPrimitiveComponent*> Primitives;
	Owner->GetComponents<UPrimitiveComponent>(Primitives);

	for (UPrimitiveComponent* Comp : Primitives)
	{
		if (Comp && Comp->GetFName() == CompName)
		{
			return Comp;
		}
	}

	return nullptr;
}

bool UFlipperForceSystemComponent::ValidateReferences()
{
	if (!RootPrimitive)
	{
		UE_LOG(LogTemp, Error, TEXT("FlipperForceSystem: RootPrimitive 引用无效"));
		return false;
	}

	if (!TankMovementComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("FlipperForceSystem: TankMovementComponent 引用无效"));
		return false;
	}

	if (FlipperPrimitives.Num() != 4)
	{
		UE_LOG(LogTemp, Error, TEXT("FlipperForceSystem: FlipperPrimitives 数组大小不为 4（当前: %d）"),
			FlipperPrimitives.Num());
		return false;
	}

	for (int32 i = 0; i < FlipperPrimitives.Num(); ++i)
	{
		if (!FlipperPrimitives[i])
		{
			UE_LOG(LogTemp, Error, TEXT("FlipperForceSystem: FlipperPrimitives[%d] 引用无效"), i);
			return false;
		}
	}

	return true;
}

UFlipperForceSystemComponent::EContactState UFlipperForceSystemComponent::ClassifyContactState() const
{
	bool bAnyStable = false;
	bool bAnyContacts = false;

	for (const FContactPatchData& Patch : ContactPatches)
	{
		if (Patch.bIsValid && Patch.bIsStable)
		{
			bAnyStable = true;
			break;
		}

		if (Patch.bIsValid && Patch.Confidence > Config.StableConfidenceThreshold * 0.33f)
		{
			bAnyContacts = true;
		}
	}

	if (bAnyStable)
	{
		return EContactState::Stable;
	}
	if (bAnyContacts)
	{
		return EContactState::PreStable;
	}
	return EContactState::NoContact;
}

void UFlipperForceSystemComponent::ExecutePipeline(float DeltaTime)
{
	ContactSensor->DetectContacts(DeltaTime, ContactPatches);
	StateProvider->BuildState(VehicleState);
	CurrentContactState = ClassifyContactState();

	if (CurrentContactState == EContactState::NoContact)
	{
		SafetyFilter->DecayForces(ContactPatches, DeltaTime, FinalForces);

		for (int32 i = 0; i < FinalForces.Num() && i < ContactPatches.Num(); ++i)
		{
			FFinalForce& FinalForce = FinalForces[i];
			const FContactPatchData& Patch = ContactPatches[i];

			const bool bInWindow = Patch.TimeSinceLastValidContact < Config.DecayApplyWindowSeconds;
			const bool bHasHistory = Patch.bHasLastValidContact;
			const bool bForceNonTrivial = FinalForce.Force.SizeSquared() > 1.0f;

			if (bInWindow && bHasHistory && bForceNonTrivial)
			{
				const FVector& N = Patch.LastValidContactNormal;
				const float NormalComp = FVector::DotProduct(FinalForce.Force, N);

				FVector ClippedForce = FVector::ZeroVector;
				if (NormalComp > 0.0f)
				{
					ClippedForce = N * NormalComp;
				}

				if (ClippedForce.SizeSquared() > 1.0f)
				{
					FinalForce.Force = ClippedForce;
					FinalForce.ApplicationPoint = Patch.LastValidContactPoint;
					FinalForce.bShouldApply = true;
				}
			}
		}

		for (int32 i = 0; i < ContactPatches.Num() && i < FinalForces.Num(); ++i)
		{
			ContactPatches[i].PreviousForce = FinalForces[i].Force;
		}

		ForceApplicator->ApplyForces(FinalForces);
		return;
	}

	TArray<FVector> SupportForces;
	TArray<FVector> AntiSlipForces;
	TArray<FVector> TractionAssistForces;
	SupportForces.Reserve(4);
	AntiSlipForces.Reserve(4);
	TractionAssistForces.Reserve(4);

	for (const FContactPatchData& Patch : ContactPatches)
	{
		if (Patch.bIsValid && Patch.bIsStable)
		{
			// 完全稳定的接触片：计算全量三类力
			SupportForces.Add(SupportSolver->ComputeForce(Patch, VehicleState));
			AntiSlipForces.Add(AntiSlipSolver->ComputeForce(Patch, VehicleState));
			TractionAssistForces.Add(TractionAssistSolver->ComputeForce(Patch, VehicleState));
		}
		else if (Patch.bIsValid)
		{
			// 有效但尚未稳定的接触片（PreStable 阶段或 Stable 整体状态下刚建立的新接触）：
			// - 支撑力按置信度缩放，防止突变
			// - 防滑力不施加（接触尚不稳定，切向力可能引起不稳定振荡）
			// - 牵引辅助力由求解器内部的置信度阈值控制，支持越障场景的早期激活
			FVector SupportForce = SupportSolver->ComputeForce(Patch, VehicleState);
			SupportForce *= Patch.Confidence;
			SupportForces.Add(SupportForce);
			AntiSlipForces.Add(FVector::ZeroVector);
			TractionAssistForces.Add(TractionAssistSolver->ComputeForce(Patch, VehicleState));
		}
		else
		{
			SupportForces.Add(FVector::ZeroVector);
			AntiSlipForces.Add(FVector::ZeroVector);
			TractionAssistForces.Add(FVector::ZeroVector);
		}
	}

	ForceComposer->ComposeForces(ContactPatches, SupportForces,
		AntiSlipForces, TractionAssistForces,
		CandidateForces);

	SafetyFilter->FilterForces(CandidateForces, ContactPatches,
		VehicleState, DeltaTime, FinalForces);

	for (int32 i = 0; i < ContactPatches.Num() && i < FinalForces.Num(); ++i)
	{
		ContactPatches[i].PreviousForce = FinalForces[i].Force;
	}

	ForceApplicator->ApplyForces(FinalForces);
}

bool UFlipperForceSystemComponent::ShouldEarlyExit()
{
	return ClassifyContactState() == EContactState::NoContact;
}

void UFlipperForceSystemComponent::DrawDebugVisualization()
{
	if (!GetWorld())
	{
		return;
	}

	const float ForceScale = 0.01f;

	for (int32 i = 0; i < ContactPatches.Num(); ++i)
	{
		const FContactPatchData& Patch = ContactPatches[i];

		FColor PointColor;
		if (!Patch.bIsValid)
		{
			PointColor = FColor::Red;
		}
		else if (Patch.bIsStable)
		{
			PointColor = FColor::Green;
		}
		else
		{
			PointColor = FColor::Yellow;
		}

		const FVector DebugPoint = Patch.bIsValid ? Patch.ContactPoint : Patch.LastValidContactPoint;
		DrawDebugSphere(GetWorld(), DebugPoint, 10.0f, 8, PointColor, false, -1.0f, 0, 1.0f);

		if (Patch.bIsValid)
		{
			DrawDebugDirectionalArrow(
				GetWorld(),
				Patch.ContactPoint,
				Patch.ContactPoint + Patch.ContactNormal * 50.0f,
				5.0f,
				FColor::Blue,
				false,
				-1.0f,
				0,
				1.5f);

			DrawDebugString(
				GetWorld(),
				Patch.ContactPoint + FVector(0, 0, 20),
				FString::Printf(TEXT("Conf: %.2f"), Patch.Confidence),
				nullptr,
				FColor::White,
				0.0f,
				true);
		}

		if (i < CandidateForces.Num() && CandidateForces[i].bIsValid)
		{
			const FCandidateForce& Candidate = CandidateForces[i];

			if (!Candidate.SupportForce.IsNearlyZero())
			{
				DrawDebugDirectionalArrow(
					GetWorld(),
					Candidate.ApplicationPoint,
					Candidate.ApplicationPoint + Candidate.SupportForce * ForceScale,
					5.0f,
					FColor::Green,
					false,
					-1.0f,
					0,
					1.5f);
			}

			if (!Candidate.AntiSlipForce.IsNearlyZero())
			{
				DrawDebugDirectionalArrow(
					GetWorld(),
					Candidate.ApplicationPoint,
					Candidate.ApplicationPoint + Candidate.AntiSlipForce * ForceScale,
					5.0f,
					FColor::Yellow,
					false,
					-1.0f,
					0,
					1.5f);
			}

			if (!Candidate.TractionAssistForce.IsNearlyZero())
			{
				DrawDebugDirectionalArrow(
					GetWorld(),
					Candidate.ApplicationPoint,
					Candidate.ApplicationPoint + Candidate.TractionAssistForce * ForceScale,
					5.0f,
					FColor::Orange,
					false,
					-1.0f,
					0,
					1.5f);
			}
		}

		if (i < FinalForces.Num() && FinalForces[i].bShouldApply)
		{
			const FFinalForce& Force = FinalForces[i];
			if (!Force.Force.IsNearlyZero())
			{
				DrawDebugDirectionalArrow(
					GetWorld(),
					Force.ApplicationPoint,
					Force.ApplicationPoint + Force.Force * ForceScale,
					10.0f,
					FColor::Red,
					false,
					-1.0f,
					0,
					2.5f);
			}
		}
	}
}
