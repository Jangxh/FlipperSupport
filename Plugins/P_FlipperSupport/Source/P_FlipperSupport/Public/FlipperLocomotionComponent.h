#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FlipperLocomotionComponent.generated.h"

class UC_TankMovementComponent;
class UChaosWheeledVehicleMovementComponent;
class UPhysicsConstraintComponent;
class USkeletalMeshComponent;
class UPrimitiveComponent;

UENUM(BlueprintType)
enum class EFlipperTargetAxis : uint8
{
	Pitch UMETA(DisplayName="Pitch"),
	Yaw   UMETA(DisplayName="Yaw"),
	Roll  UMETA(DisplayName="Roll")
};

/**
 * 支撑臂运动组件
 * 
 * 1. 控制支撑臂旋转移动
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), HideCategories=(Sockets))
class P_FLIPPERSUPPORT_API UFlipperLocomotionComponent : public UActorComponent
{
	GENERATED_BODY()

#pragma region General
	
public:
	UFlipperLocomotionComponent();

protected:
	virtual void BeginPlay() override;
	
#pragma endregion

#pragma region Flipper Locomotion
	
public:
	
	/**
	 * 前支撑臂输入
	 * @param AxisValue 输入值，>0 前转，<0 后转
	 */
	UFUNCTION(BlueprintCallable, Category="Flipper|Locomotion")
	void InputFront(float AxisValue);

	/**
	 * 后支撑臂输入
	 * @param AxisValue 输入值，>0 前转，<0 后转
	 */
	UFUNCTION(BlueprintCallable, Category="Flipper|Locomotion")
	void InputRear(float AxisValue);

	/**
	 * 恢复初始姿态：前后都回到 0 度（参考帧）
	 */
	UFUNCTION(BlueprintCallable, Category="Flipper|Locomotion")
	void ResetAllToInitial();

	/**
	 * 前支撑臂回到 0 度
	 */
	UFUNCTION(BlueprintCallable, Category="Flipper|Locomotion")
	void ResetFrontToInitial();

	/**
	 * 前支撑臂回到 0 度
	 */
	UFUNCTION(BlueprintCallable, Category="Flipper|Locomotion")
	void ResetRearToInitial();
	
	/**
	 * 直接设置前支撑臂目标角（度）
	 * @param Degrees 
	 */
	UFUNCTION(BlueprintCallable, Category="Flipper|Locomotion")
	void SetFrontTargetDegrees(float Degrees);

	/**
	 * 直接设置后支撑臂目标角（度）
	 * @param Degrees 
	 */
	UFUNCTION(BlueprintCallable, Category="Flipper|Locomotion")
	void SetRearTargetDegrees(float Degrees);

protected:
	/**
	 * 每次用户输入触发时增加的角度（度）
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flipper|Locomotion|Control")
	float StepDegrees = 5.0f;

	/**
	 * 输入死区：|Axis| <= DeadZone 时忽略（避免摇杆微抖导致疯狂加角）
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flipper|Locomotion|Control")
	float InputDeadZone = 0.2f;

	/**
	 * 是否按“离散步进”方式工作：true=每次触发加 StepDegrees；false=按 AxisValue * StepDegrees 加
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flipper|Locomotion|Control")
	bool bDiscreteStep = true;
	
	/**
	 * 前支撑臂允许最小角度（度）
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flipper|Locomotion|Limit")
	float FrontMinDegrees = -90.0f;

	/**
	 * 前支撑臂允许最大角度（度）
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flipper|Locomotion|Limit")
	float FrontMaxDegrees = 90.0f;

	/**
	 * 后支撑臂允许最小角度（度）
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flipper|Locomotion|Limit")
	float RearMinDegrees = -90.0f;

	/**
	 * 后支撑臂允许最大角度（度）
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flipper|Locomotion|Limit")
	float RearMaxDegrees = 90.0f;

	/** 
	 * 目标旋转哪个轴 
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flipper|Locomotion|Control")
	EFlipperTargetAxis TargetAxis = EFlipperTargetAxis::Pitch;

	/** 
	 * 前左物理限制组件名称 
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flipper|Locomotion|Front")
	FName FrontLeftConstraintName;

	/** 
	 * 前右物理限制组件名称 
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flipper|Locomotion|Front")
	FName FrontRightConstraintName;

	/** 
	 * 后左物理限制组件名称 
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flipper|Locomotion|Rear")
	FName RearLeftConstraintName;

	/** 
	 * 后右物理限制组件名称 
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flipper|Locomotion|Rear")
	FName RearRightConstraintName;

private:
	/**
	 * 前左物理约束组件
	 */
	UPROPERTY(Transient) 
	TObjectPtr<UPhysicsConstraintComponent> FrontLeftConstraint = nullptr;
	
	/**
	 * 前右物理约束组件
	 */
	UPROPERTY(Transient) 
	TObjectPtr<UPhysicsConstraintComponent> FrontRightConstraint = nullptr;
	
	/**
	 * 后左物理约束组件
	 */
	UPROPERTY(Transient) 
	TObjectPtr<UPhysicsConstraintComponent> RearLeftConstraint = nullptr;
	
	/**
	 * 后右物理约束组件
	 */
	UPROPERTY(Transient) 
	TObjectPtr<UPhysicsConstraintComponent> RearRightConstraint = nullptr;
	
	/**
	 * 前支撑臂相对于初始帧角度
	 */
	UPROPERTY(Transient)
	float FrontTargetDegrees = 0.0f;
	
	/**
	 * 后支撑臂相对于初始帧角度
	 */
	UPROPERTY(Transient) 
	float RearTargetDegrees  = 0.0f;
	
	/**
	 * 更新物理约束组件
	 */
	void ResolveAndCache();
	
	/**
	 * 根据名称查找物理约束组件
	 * @param CompName 
	 * @return 
	 */
	UPhysicsConstraintComponent* FindConstraintByName(const FName& CompName) const;

	/**
	 * 应用前支持臂角度
	 */
	void ApplyFrontTarget() const;
	
	/**
	 * 应用后支撑臂角度
	 */
	void ApplyRearTarget() const;

	/**
	 * 针对特定物理约束组件设置角驱动度数
	 * @param Constraint 
	 * @param Degrees 
	 */
	void ApplyTargetToConstraint(UPhysicsConstraintComponent* Constraint, float Degrees) const;

	/**
	 * 根据输入值输出步进角度
	 * @param AxisValue 
	 * @param StepDegrees 
	 * @param bDiscrete 
	 * @param DeadZone 
	 * @return 
	 */
	static float StepFromAxis(float AxisValue, float StepDegrees, bool bDiscrete, float DeadZone);
	
	/**
	 * 约束角度到最大值和最小值范围内
	 * @param Degrees 
	 * @param MinDeg 
	 * @param MaxDeg 
	 * @return 
	 */
	static float ClampDegrees(float Degrees, float MinDeg, float MaxDeg);

#pragma endregion
};