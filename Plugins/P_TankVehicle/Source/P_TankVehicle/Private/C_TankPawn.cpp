// Fill out your copyright notice in the Description page of Project Settings.


#include "C_TankPawn.h"

#include "C_TankMovementComponent.h"

FName AC_TankPawn::VehicleMovementComponentName(TEXT("TankMovementComp"));
FName AC_TankPawn::VehicleMeshComponentName(TEXT("TankMesh"));

AC_TankPawn::AC_TankPawn(const FObjectInitializer& ObjectInitializer): Super(ObjectInitializer)
{
	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(VehicleMeshComponentName);
	Mesh->SetCollisionProfileName(UCollisionProfile::Vehicle_ProfileName);
	Mesh->BodyInstance.bSimulatePhysics = true;
	Mesh->BodyInstance.bNotifyRigidBodyCollision = true;
	Mesh->BodyInstance.bUseCCD = true;
	Mesh->SetGenerateOverlapEvents(true);
	Mesh->SetCanEverAffectNavigation(false);
	RootComponent = Mesh;

	VehicleMovementComponent = CreateDefaultSubobject<UC_TankMovementComponent, UC_TankMovementComponent>(VehicleMovementComponentName);
	VehicleMovementComponent->SetIsReplicated(true);
	VehicleMovementComponent->UpdatedComponent = Mesh;
}

UC_TankMovementComponent* AC_TankPawn::GetVehicleMovementComponent() const
{
	return VehicleMovementComponent;
}
