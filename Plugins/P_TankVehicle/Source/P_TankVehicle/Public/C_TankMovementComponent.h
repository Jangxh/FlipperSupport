// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "C_TankMovementComponent.generated.h"


/**
 * 采用ChaosWheeledVehicleMovementComponent仿真坦克
 * 
 */
UCLASS(ClassGroup=(Physics), meta=(BlueprintSpawnableComponent))
class P_TANKVEHICLE_API UC_TankMovementComponent : public UChaosWheeledVehicleMovementComponent
{
	GENERATED_BODY()

public:
	UC_TankMovementComponent();
	
	UFUNCTION(BlueprintCallable, Category="Game|Components|TankMovement")
	void SetLeftThrottle(float Throttle);
	
	UFUNCTION(BlueprintCallable, Category="Game|Components|TankMovement")
	void SetRightThrottle(float Throttle);
	
	UFUNCTION(BlueprintPure, Category="Game|Components|TankMovement")
	float GetYawInput() const;
	
protected:
	virtual void CalcThrottleAndSteering(float& ThrottleOut, float& SteeringOut);

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
private:
	/**
	 * 左油门原始输入
	 */
	float RawLeftThrottleInput;
	
	/**
	 * 右油门原始输入
	 */
	float RawRightThrottleInput;
	
	/**
	 * 最大旋转角速度(角度)
	 */
	UPROPERTY(EditDefaultsOnly, Category="Game|Components|TankMovement", meta=(ClampMin=0, ClampMax=360))
	float MaxRotationalSpeed = 72.f;
	
	float LastThrottleOut;
	float RawYawInput;
};
