// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FlipperForceSystemTypes.h"
#include "FlipperForceSystemComponent.generated.h"

class UPrimitiveComponent;
class UC_TankMovementComponent;
class UFlipperContactSensor;
class UVehicleStateProvider;
class UFlipperSupportSolver;
class UFlipperAntiSlipSolver;
class UFlipperTractionAssistSolver;
class UFlipperForceComposer;
class UFlipperForceSafetyFilter;
class UFlipperForceApplicator;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class P_FLIPPERSUPPORT_API UFlipperForceSystemComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFlipperForceSystemComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                          FActorComponentTickFunction* ThisTickFunction) override;

	// ===== 组件名称配置 =====

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Component Names")
	FName RootPrimitiveName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Component Names")
	FName TankMovementComponentName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Component Names")
	FName FrontLeftFlipperName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Component Names")
	FName FrontRightFlipperName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Component Names")
	FName RearLeftFlipperName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Component Names")
	FName RearRightFlipperName;

	// ===== 系统配置 =====

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	FForceSystemConfig Config;

	// ===== 运行时控制 =====

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Control")
	bool bEnableSystem = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bEnableDebugVisualization = false;

private:
	// ===== 缓存的组件引用 =====

	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> RootPrimitive = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UC_TankMovementComponent> TankMovementComponent = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPrimitiveComponent>> FlipperPrimitives;

	// ===== 子模块实例 =====

	UPROPERTY()
	TObjectPtr<UFlipperContactSensor> ContactSensor = nullptr;

	UPROPERTY()
	TObjectPtr<UVehicleStateProvider> StateProvider = nullptr;

	UPROPERTY()
	TObjectPtr<UFlipperSupportSolver> SupportSolver = nullptr;

	UPROPERTY()
	TObjectPtr<UFlipperAntiSlipSolver> AntiSlipSolver = nullptr;

	UPROPERTY()
	TObjectPtr<UFlipperTractionAssistSolver> TractionAssistSolver = nullptr;

	UPROPERTY()
	TObjectPtr<UFlipperForceComposer> ForceComposer = nullptr;

	UPROPERTY()
	TObjectPtr<UFlipperForceSafetyFilter> SafetyFilter = nullptr;

	UPROPERTY()
	TObjectPtr<UFlipperForceApplicator> ForceApplicator = nullptr;

	// ===== 运行时数据 =====

	TArray<FContactPatchData> ContactPatches;
	FFlipperVehicleState VehicleState;
	TArray<FCandidateForce> CandidateForces;
	TArray<FFinalForce> FinalForces;

	/** 三态接触状态枚举，用于 ExecutePipeline 分支决策 */
	enum class EContactState : uint8
	{
		NoContact,
		PreStable,
		Stable
	};

	/** 当前帧接触状态 */
	EContactState CurrentContactState = EContactState::NoContact;

private:
	void InitializeSubmodules();
	void ResolveAndCacheComponents();
	UPrimitiveComponent* FindPrimitiveByName(const FName& CompName) const;
	bool ValidateReferences();
	void ExecutePipeline(float DeltaTime);
	bool ShouldEarlyExit();
	EContactState ClassifyContactState() const;
	void DrawDebugVisualization();
};
