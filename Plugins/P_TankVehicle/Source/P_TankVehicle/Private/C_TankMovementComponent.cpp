// Fill out your copyright notice in the Description page of Project Settings.


#include "C_TankMovementComponent.h"


UC_TankMovementComponent::UC_TankMovementComponent()
{
	TorqueControl.Enabled = true;
	TorqueControl.YawTorqueScaling = 2.0f;
}

void UC_TankMovementComponent::SetLeftThrottle(float Throttle)
{
	RawLeftThrottleInput = FMath::Clamp(Throttle, -1.0f, 1.0f);
	
	float ThrottleOut, SteeringOut;
	CalcThrottleAndSteering(ThrottleOut, SteeringOut);
	if (ThrottleOut >= 0 && LastThrottleOut >= 0)
	{
		SetThrottleInput(ThrottleOut);
	}
	else if (ThrottleOut >= 0 && LastThrottleOut < 0)
	{
		SetBrakeInput(0);
		SetThrottleInput(ThrottleOut);
	}
	else if (ThrottleOut < 0 && LastThrottleOut >= 0)
	{
		SetBrakeInput(-ThrottleOut);
	}
	else
	{
		SetBrakeInput(-ThrottleOut);
	}
	SetYawInput(SteeringOut);
	RawYawInput = SteeringOut;
	LastThrottleOut = ThrottleOut;
}

void UC_TankMovementComponent::SetRightThrottle(float Throttle)
{
	RawRightThrottleInput = FMath::Clamp(Throttle, -1.0f, 1.0f);
	
	float ThrottleOut, SteeringOut;
	CalcThrottleAndSteering(ThrottleOut, SteeringOut);
	if (ThrottleOut >= 0 && LastThrottleOut >= 0)
	{
		SetThrottleInput(ThrottleOut);
	}
	else if (ThrottleOut >= 0 && LastThrottleOut < 0)
	{
		SetBrakeInput(0);
		SetThrottleInput(ThrottleOut);
	}
	else if (ThrottleOut < 0 && LastThrottleOut >= 0)
	{
		SetBrakeInput(-ThrottleOut);
	}
	else
	{
		SetBrakeInput(-ThrottleOut);
	}
	
	SetYawInput(SteeringOut);
	RawYawInput = SteeringOut;
	LastThrottleOut = ThrottleOut;
}

float UC_TankMovementComponent::GetYawInput() const
{
	return RawYawInput;
}

void UC_TankMovementComponent::CalcThrottleAndSteering(float& ThrottleOut, float& SteeringOut)
{
	ThrottleOut = (RawLeftThrottleInput + RawRightThrottleInput) / 2;
	ThrottleOut = FMath::Clamp(ThrottleOut, -1.0f, 1.0f);
	SteeringOut = (RawRightThrottleInput - RawLeftThrottleInput) / 2;
	SteeringOut = FMath::Clamp(SteeringOut, -1.0f, 1.0f);
	
	if (ThrottleOut == -1.f && !VehicleState.LocalGForce.IsNearlyZero(0.1))
	{
		ThrottleOut = 0.f;
		SteeringOut = 0.f;
	}
}


void UC_TankMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!IsValid(UpdatedPrimitive))
		return;

	const float CurrentRotationalSpeed = UpdatedPrimitive->GetPhysicsAngularVelocityInDegrees().Length();

	if (CurrentRotationalSpeed > MaxRotationalSpeed)
	{
		SetYawInput(0.f);
	}
}

