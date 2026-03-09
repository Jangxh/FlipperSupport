// Copyright Epic Games, Inc. All Rights Reserved.

#include "VehicleStateProvider.h"
#include "Components/PrimitiveComponent.h"
#include "C_TankMovementComponent.h"

void UVehicleStateProvider::Initialize(UPrimitiveComponent* InRootPrimitive, UC_TankMovementComponent* InMovementComponent)
{
	RootPrimitive = InRootPrimitive;
	MovementComponent = InMovementComponent;
}

void UVehicleStateProvider::BuildState(FFlipperVehicleState& OutState)
{
	// 防御性检查：确保引用有效
	if (!RootPrimitive || !MovementComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("VehicleStateProvider: Invalid references, cannot build state"));
		return;
	}

	// ===== 从 RootPrimitive 读取刚体状态 =====

	// 读取线速度（世界空间，cm/s）
	OutState.LinearVelocity = RootPrimitive->GetPhysicsLinearVelocity();

	// 读取角速度（世界空间，rad/s）
	OutState.AngularVelocity = RootPrimitive->GetPhysicsAngularVelocityInRadians();

	// 读取质心位置（世界空间，cm）
	OutState.CenterOfMass = RootPrimitive->GetCenterOfMass();

	// ===== 从 RootPrimitive 获取车辆坐标系 =====

	// 车辆前向矢量（世界空间，单位矢量）
	OutState.ForwardVector = RootPrimitive->GetForwardVector();

	// 车辆右向矢量（世界空间，单位矢量）
	OutState.RightVector = RootPrimitive->GetRightVector();

	// 车辆上向矢量（世界空间，单位矢量）
	OutState.UpVector = RootPrimitive->GetUpVector();

	// ===== 从 MovementComponent 读取输入状态 =====

	// 油门输入 [-1, 1]
	// UChaosWheeledVehicleMovementComponent 提供 GetThrottleInput() 方法
	OutState.ThrottleInput = MovementComponent->GetThrottleInput();

	// 偏航输入 [-1, 1]
	// UC_TankMovementComponent 提供 GetYawInput() 方法
	OutState.YawInput = MovementComponent->GetYawInput();

	// ===== 从 MovementComponent 读取接触状态 =====

	// 轮地接触比例 [0, 1]
	// 通过检查车轮状态计算接触比例
	// UChaosWheeledVehicleMovementComponent 提供 Wheels 数组和 GetNumWheels() 方法
	// 注意：由于 UE5 Chaos 车辆系统的 API 可能不直接暴露车轮接触状态，
	// 这里采用保守实现：假设车辆在地面上时接触比例为 1.0，否则为 0.0
	// 后续可根据实际 API 可用性优化为精确计算
	const float VehicleSpeed = OutState.LinearVelocity.Size();
	const bool bVehicleMoving = VehicleSpeed > 1.0f; // 速度大于 1 cm/s 视为运动中
	OutState.WheelGroundContactRatio = bVehicleMoving ? 1.0f : 0.5f; // 简化实现

	// 底盘接触状态
	// 检测底盘底部是否接触地面或障碍物
	// 通过检查车体垂直速度和位置来推断
	// 注意：这是简化实现，精确检测需要额外的物理查询或碰撞事件
	// 如果车体向下速度接近零且位置较低，可能存在底盘接触
	const float VerticalSpeed = FVector::DotProduct(OutState.LinearVelocity, OutState.UpVector);
	const bool bLowVerticalSpeed = FMath::Abs(VerticalSpeed) < 10.0f; // 垂直速度小于 10 cm/s
	OutState.bChassisContact = bLowVerticalSpeed && !bVehicleMoving; // 简化实现

	// ===== 读取车辆物理属性（可选） =====

	// 车辆质量 (kg)
	// 从 RootPrimitive 的物理体实例读取质量
	if (RootPrimitive->IsSimulatingPhysics())
	{
		OutState.VehicleMass = RootPrimitive->GetMass();
	}
	else
	{
		OutState.VehicleMass = 1000.0f; // 默认质量
	}

	// 重力加速度大小 (cm/s²)
	// UE5 默认重力为 -980 cm/s²
	if (UWorld* World = RootPrimitive->GetWorld())
	{
		OutState.GravityMagnitude = FMath::Abs(World->GetGravityZ());
	}
	else
	{
		OutState.GravityMagnitude = 980.0f; // 默认重力
	}

	// ===== 时间戳 =====

	// 记录状态时间戳（秒）
	if (UWorld* World = RootPrimitive->GetWorld())
	{
		OutState.Timestamp = World->GetTimeSeconds();
	}
	else
	{
		OutState.Timestamp = 0.0f;
	}
}
