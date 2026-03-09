// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "C_TankPawn.generated.h"

class UC_TankMovementComponent;
/**
 * 坦克载具
 */
UCLASS(Abstract, BlueprintType)
class P_TANKVEHICLE_API AC_TankPawn : public APawn
{
	GENERATED_BODY()

public:
	AC_TankPawn(const FObjectInitializer& ObjectInitializer);
	UC_TankMovementComponent* GetVehicleMovementComponent() const;
	
	USkeletalMeshComponent* GetMesh() const { return Mesh; }
	
	UC_TankMovementComponent* GetVehicleMovement() const { return VehicleMovementComponent; }

private:
	/**
	 * 坦克Mesh
	 */
	UPROPERTY(Category = Vehicle, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> Mesh;

	/**
	 * 坦克移动组件
	 */
	UPROPERTY(Category = Vehicle, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UC_TankMovementComponent> VehicleMovementComponent;
	
	/**
	 * MeshComponent名称
	 * 
	 * 如果想阻止创建该组件（使用ObjectInitializer.DoNotCreateDefaultSubobject）
	 */
	static FName VehicleMeshComponentName;
	
	/**
	 * VehicleMovementComponent名称
	 * 
	 * 如果想要使用不同的类（使用ObjectInitializer.SetDefaultSubobjectClass）
	 */
	static FName VehicleMovementComponentName;
};
