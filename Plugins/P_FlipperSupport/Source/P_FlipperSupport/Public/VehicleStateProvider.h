// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FlipperForceSystemTypes.h"
#include "VehicleStateProvider.generated.h"

class UPrimitiveComponent;
class UC_TankMovementComponent;

/**
 * 车辆状态提供模块
 * 
 * 职责：
 * - 读取车辆状态并生成状态快照
 * - 从 RootPrimitive 读取刚体状态（速度、角速度、质心、坐标系）
 * - 从 TankMovementComponent 读取输入和接触状态
 * 
 * 实现要点：
 * - 每帧构建完整的 FVehicleState 快照
 * - 提供车辆物理状态和控制输入的统一访问接口
 */
UCLASS()
class P_FLIPPERSUPPORT_API UVehicleStateProvider : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * 初始化状态提供器
	 * @param InRootPrimitive 车体主刚体，用于读取物理状态
	 * @param InMovementComponent 坦克运动组件，用于读取输入和接触状态
	 */
	void Initialize(UPrimitiveComponent* InRootPrimitive, UC_TankMovementComponent* InMovementComponent);

	/**
	 * 构建车辆状态快照
	 * @param OutState 输出车辆状态结构
	 */
	void BuildState(FFlipperVehicleState& OutState);

private:
	// ===== 外部引用 =====

	/** 车体主刚体，用于读取物理状态 */
	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> RootPrimitive = nullptr;

	/** 坦克运动组件，用于读取输入和接触状态 */
	UPROPERTY(Transient)
	TObjectPtr<UC_TankMovementComponent> MovementComponent = nullptr;
};
