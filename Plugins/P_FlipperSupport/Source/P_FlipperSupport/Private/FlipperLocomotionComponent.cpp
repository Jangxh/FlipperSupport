#include "FlipperLocomotionComponent.h"

#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "DrawDebugHelpers.h"

#pragma region General

UFlipperLocomotionComponent::UFlipperLocomotionComponent()
{
	// 禁用tick
	PrimaryComponentTick.bCanEverTick = false;
}

void UFlipperLocomotionComponent::BeginPlay()
{
	Super::BeginPlay();

	// 解析物理约束组件
	ResolveAndCache();
}

#pragma endregion


#pragma region Flipper Locomotion

void UFlipperLocomotionComponent::InputFront(const float AxisValue)
{
	ResolveAndCache();

	const float Delta = StepFromAxis(AxisValue, StepDegrees, bDiscreteStep, InputDeadZone);
	if (FMath::IsNearlyZero(Delta)) return;

	FrontTargetDegrees = ClampDegrees(FrontTargetDegrees + Delta, FrontMinDegrees, FrontMaxDegrees);
	ApplyFrontTarget();
}

void UFlipperLocomotionComponent::InputRear(const float AxisValue)
{
	ResolveAndCache();

	const float Delta = StepFromAxis(AxisValue, StepDegrees, bDiscreteStep, InputDeadZone);
	if (FMath::IsNearlyZero(Delta)) return;

	RearTargetDegrees = ClampDegrees(RearTargetDegrees + Delta, RearMinDegrees, RearMaxDegrees);
	ApplyRearTarget();
}

void UFlipperLocomotionComponent::ResetAllToInitial()
{
	ResolveAndCache();

	FrontTargetDegrees = 0.0f;
	RearTargetDegrees  = 0.0f;

	ApplyFrontTarget();
	ApplyRearTarget();
}

void UFlipperLocomotionComponent::ResetFrontToInitial()
{
	ResolveAndCache();

	FrontTargetDegrees = 0.0f;
	ApplyFrontTarget();
}

void UFlipperLocomotionComponent::ResetRearToInitial()
{
	ResolveAndCache();

	RearTargetDegrees = 0.0f;
	ApplyRearTarget();
}

void UFlipperLocomotionComponent::SetFrontTargetDegrees(float Degrees)
{
	ResolveAndCache();

	FrontTargetDegrees = ClampDegrees(Degrees, FrontMinDegrees, FrontMaxDegrees);
	ApplyFrontTarget();
}

void UFlipperLocomotionComponent::SetRearTargetDegrees(float Degrees)
{
	ResolveAndCache();

	RearTargetDegrees = ClampDegrees(Degrees, RearMinDegrees, RearMaxDegrees);
	ApplyRearTarget();
}

void UFlipperLocomotionComponent::ResolveAndCache()
{
	// 只要有任何一个为空，就尝试刷新一次缓存
	if (FrontLeftConstraint && FrontRightConstraint
		&& RearLeftConstraint && RearRightConstraint)
	{
		return;
	}
	
	FrontLeftConstraint  = FindConstraintByName(FrontLeftConstraintName);
	FrontRightConstraint = FindConstraintByName(FrontRightConstraintName);
	RearLeftConstraint   = FindConstraintByName(RearLeftConstraintName);
	RearRightConstraint  = FindConstraintByName(RearRightConstraintName);
}

UPhysicsConstraintComponent* UFlipperLocomotionComponent::FindConstraintByName(const FName& CompName) const
{
	if (CompName.IsNone()) return nullptr;

	AActor* Owner = GetOwner();
	if (!Owner) return nullptr;

	TArray<UPhysicsConstraintComponent*> Constraints;
	Owner->GetComponents<UPhysicsConstraintComponent>(Constraints);

	for (UPhysicsConstraintComponent* C : Constraints)
	{
		if (C && C->GetFName() == CompName)
		{
			return C;
		}
	}
	return nullptr;
}

void UFlipperLocomotionComponent::ApplyFrontTarget() const
{
	ApplyTargetToConstraint(FrontLeftConstraint,  FrontTargetDegrees);
	ApplyTargetToConstraint(FrontRightConstraint, FrontTargetDegrees);
}

void UFlipperLocomotionComponent::ApplyRearTarget() const
{
	ApplyTargetToConstraint(RearLeftConstraint,  RearTargetDegrees);
	ApplyTargetToConstraint(RearRightConstraint, RearTargetDegrees);
}

void UFlipperLocomotionComponent::ApplyTargetToConstraint(UPhysicsConstraintComponent* Constraint, const float Degrees) const
{
	if (!Constraint) return;

	FRotator R = FRotator::ZeroRotator;
	switch (TargetAxis)
	{
	case EFlipperTargetAxis::Pitch:
		R.Pitch = Degrees;
		break;
	case EFlipperTargetAxis::Yaw:
		R.Yaw = Degrees;
		break;
	case EFlipperTargetAxis::Roll:
		R.Roll = Degrees;
		break;
	default:
		R.Pitch = Degrees;
		break;
	}

	Constraint->SetAngularOrientationTarget(R);
}

float UFlipperLocomotionComponent::StepFromAxis(float AxisValue, float InStepDegrees, bool bDiscrete, float DeadZone)
{
	if (FMath::Abs(AxisValue) <= DeadZone) return 0.0f;

	const float Sign = (AxisValue >= 0.0f) ? 1.0f : -1.0f;
	if (bDiscrete)
	{
		return Sign * InStepDegrees;
	}
	else
	{
		return AxisValue * InStepDegrees;
	}
}

float UFlipperLocomotionComponent::ClampDegrees(const float Degrees, float MinDeg, float MaxDeg)
{
	if (MinDeg > MaxDeg)
	{
		Swap(MinDeg, MaxDeg);
	}
	return FMath::Clamp(Degrees, MinDeg, MaxDeg);
}

#pragma endregion
