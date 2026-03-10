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
	// UE5.3 Chaos 不提供每轮接触状态的公共 API（WheelStatus/bInContact 为 protected）。
	// 真实的 TractionAssist 激活闸门由 FlipperContactSensor 的置信度机制保证：
	// 平地正常行驶时支撑臂无接触 → TractionAssist 不激活；
	// 越障时支撑臂有接触且置信度 > 阈值 → TractionAssist 激活。
	// 因此固定返回 0.0f 使 WheelContactRatioThreshold 条件始终满足，不再是约束瓶颈。
	OutState.WheelGroundContactRatio = 0.0f;

	// 底盘接触状态
	// TODO: 正确实现需要从底盘底部向下射线检测
	// 旧的速度+静止启发式会在车辆卡在障碍物时误触发，导致 ChassisContactReductionFactor
	// 在最需要牵引辅助力的场景下错误地削减 50% 的力。暂时禁用以避免误触发降额。
	OutState.bChassisContact = false;

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
