// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FlipperForceSystemTypes.generated.h"

/**
 * 支撑臂索引枚举
 * 标识接触片属于四个支撑臂中的哪一个
 */
UENUM(BlueprintType)
enum class EFlipperIndex : uint8
{
	/** 前左支撑臂 */
	FrontLeft = 0   UMETA(DisplayName = "Front Left"),
	
	/** 前右支撑臂 */
	FrontRight = 1  UMETA(DisplayName = "Front Right"),
	
	/** 后左支撑臂 */
	RearLeft = 2    UMETA(DisplayName = "Rear Left"),
	
	/** 后右支撑臂 */
	RearRight = 3   UMETA(DisplayName = "Rear Right")
};

/**
 * 车辆状态快照
 * 包含力计算所需的所有车辆状态信息
 */
USTRUCT(BlueprintType)
struct FFlipperVehicleState
{
	GENERATED_BODY()

	/** 车体线速度（世界空间，cm/s） */
	UPROPERTY(BlueprintReadOnly, Category = "Vehicle State")
	FVector LinearVelocity = FVector::ZeroVector;

	/** 车体角速度（世界空间，rad/s） */
	UPROPERTY(BlueprintReadOnly, Category = "Vehicle State")
	FVector AngularVelocity = FVector::ZeroVector;

	/** 质心位置（世界空间，cm） */
	UPROPERTY(BlueprintReadOnly, Category = "Vehicle State")
	FVector CenterOfMass = FVector::ZeroVector;

	/** 车辆前向矢量（世界空间，单位矢量） */
	UPROPERTY(BlueprintReadOnly, Category = "Vehicle State")
	FVector ForwardVector = FVector::ForwardVector;

	/** 车辆右向矢量（世界空间，单位矢量） */
	UPROPERTY(BlueprintReadOnly, Category = "Vehicle State")
	FVector RightVector = FVector::RightVector;

	/** 车辆上向矢量（世界空间，单位矢量） */
	UPROPERTY(BlueprintReadOnly, Category = "Vehicle State")
	FVector UpVector = FVector::UpVector;

	/** 油门输入 [-1, 1]，正值前进，负值后退 */
	UPROPERTY(BlueprintReadOnly, Category = "Vehicle State")
	float ThrottleInput = 0.0f;

	/** 偏航输入 [-1, 1]，正值右转，负值左转 */
	UPROPERTY(BlueprintReadOnly, Category = "Vehicle State")
	float YawInput = 0.0f;

	/** 轮地接触比例 [0, 1]，0=完全离地，1=完全接地 */
	UPROPERTY(BlueprintReadOnly, Category = "Vehicle State")
	float WheelGroundContactRatio = 0.0f;

	/** 底盘底部是否接触地面或障碍物 */
	UPROPERTY(BlueprintReadOnly, Category = "Vehicle State")
	bool bChassisContact = false;

	/** 车辆质量 (kg)，可选，用于调参和后续扩展 */
	UPROPERTY(BlueprintReadOnly, Category = "Vehicle State")
	float VehicleMass = 0.0f;

	/** 重力加速度大小 (cm/s²)，可选，用于调参和后续扩展 */
	UPROPERTY(BlueprintReadOnly, Category = "Vehicle State")
	float GravityMagnitude = 980.0f;

	/** 状态时间戳（秒），用于调试、历史追踪和后续时序扩展 */
	UPROPERTY(BlueprintReadOnly, Category = "Vehicle State")
	float Timestamp = 0.0f;
};

/**
 * 单个支撑臂的接触片数据
 * 包含接触几何、动力学和历史信息
 */
USTRUCT(BlueprintType)
struct FContactPatchData
{
	GENERATED_BODY()

	/** 支撑臂索引，标识该接触片属于哪个支撑臂 */
	UPROPERTY(BlueprintReadOnly, Category = "Contact Patch")
	EFlipperIndex FlipperIndex = EFlipperIndex::FrontLeft;

	/** 当前帧是否检测到有效接触 */
	UPROPERTY(BlueprintReadOnly, Category = "Contact Patch")
	bool bIsValid = false;

	/** 置信度 [0, 1]，平滑增减，用于过滤接触闪烁 */
	UPROPERTY(BlueprintReadOnly, Category = "Contact Patch")
	float Confidence = 0.0f;

	/** 是否稳定（Confidence >= StableConfidenceThreshold） */
	UPROPERTY(BlueprintReadOnly, Category = "Contact Patch")
	bool bIsStable = false;

	/** 接触点位置（世界空间，cm） */
	UPROPERTY(BlueprintReadOnly, Category = "Contact Patch")
	FVector ContactPoint = FVector::ZeroVector;

	/** 接触法线（世界空间，单位矢量，指向远离地面/障碍物表面） */
	UPROPERTY(BlueprintReadOnly, Category = "Contact Patch")
	FVector ContactNormal = FVector::UpVector;

	/** 接触压缩量（cm），ContactMetric = max(0, TargetContactGap - ActualGap) */
	UPROPERTY(BlueprintReadOnly, Category = "Contact Patch")
	float ContactMetric = 0.0f;

	/** 接触点速度（世界空间，cm/s），V_contact = V_linear + ω × r */
	UPROPERTY(BlueprintReadOnly, Category = "Contact Patch")
	FVector ContactVelocity = FVector::ZeroVector;

	/** 切向速度分量（接触平面内，cm/s），用于计算防滑力 */
	UPROPERTY(BlueprintReadOnly, Category = "Contact Patch")
	FVector TangentialVelocity = FVector::ZeroVector;

	/** 上一帧过滤后的最终力（世界空间，UE 力单位：kg·cm/s²），用于方向平滑和 jerk 限制 */
	UPROPERTY(BlueprintReadOnly, Category = "Contact Patch")
	FVector PreviousForce = FVector::ZeroVector;

	/** 是否已经有过至少一次有效接触（显式布尔，不用零向量兼职状态机） */
	UPROPERTY(BlueprintReadOnly, Category = "Contact Patch")
	bool bHasLastValidContact = false;

	/** 最后一次有效接触的接触点（世界空间，cm） */
	UPROPERTY(BlueprintReadOnly, Category = "Contact Patch")
	FVector LastValidContactPoint = FVector::ZeroVector;

	/** 最后一次有效接触的法线（世界空间，单位矢量） */
	UPROPERTY(BlueprintReadOnly, Category = "Contact Patch")
	FVector LastValidContactNormal = FVector::UpVector;

	/** 距上次有效接触经过的时间（秒） */
	UPROPERTY(BlueprintReadOnly, Category = "Contact Patch")
	float TimeSinceLastValidContact = 999.0f;

	/** 最后更新时间 (秒) */
	UPROPERTY(BlueprintReadOnly, Category = "Contact Patch")
	float LastUpdateTime = 0.0f;
};

/**
 * 力组合器输出的候选力
 * 包含各个力分量和摩擦锥约束后的总力
 */
USTRUCT(BlueprintType)
struct FCandidateForce
{
	GENERATED_BODY()

	/** 支撑臂索引，标识该力属于哪个支撑臂 */
	UPROPERTY(BlueprintReadOnly, Category = "Candidate Force")
	EFlipperIndex FlipperIndex = EFlipperIndex::FrontLeft;

	/** 支撑力（法向，UE 力单位：kg·cm/s²），沿接触法线方向 */
	UPROPERTY(BlueprintReadOnly, Category = "Candidate Force")
	FVector SupportForce = FVector::ZeroVector;

	/** 防滑力（切向，UE 力单位：kg·cm/s²），在接触平面内，与滑动方向相反 */
	UPROPERTY(BlueprintReadOnly, Category = "Candidate Force")
	FVector AntiSlipForce = FVector::ZeroVector;

	/** 牵引辅助力（切向，UE 力单位：kg·cm/s²），在接触平面内，沿车辆纵向 */
	UPROPERTY(BlueprintReadOnly, Category = "Candidate Force")
	FVector TractionAssistForce = FVector::ZeroVector;

	/** 总力（UE 力单位：kg·cm/s²），三类力的矢量和，已满足摩擦锥约束 |Ft| <= μ*Fn */
	UPROPERTY(BlueprintReadOnly, Category = "Candidate Force")
	FVector TotalForce = FVector::ZeroVector;

	/** 施力点位置（世界空间，cm） */
	UPROPERTY(BlueprintReadOnly, Category = "Candidate Force")
	FVector ApplicationPoint = FVector::ZeroVector;

	/** 该候选力是否有效 */
	UPROPERTY(BlueprintReadOnly, Category = "Candidate Force")
	bool bIsValid = false;
};

/**
 * 安全过滤器输出的最终力
 * 准备施加到对应支撑臂刚体（若缺失则 fallback 到 RootPrimitive）
 */
USTRUCT(BlueprintType)
struct FFinalForce
{
	GENERATED_BODY()

	/** 支撑臂索引，标识该力属于哪个支撑臂 */
	UPROPERTY(BlueprintReadOnly, Category = "Final Force")
	EFlipperIndex FlipperIndex = EFlipperIndex::FrontLeft;

	/** 最终力矢量（世界空间，UE 力单位：kg·cm/s²），已通过所有安全过滤和平滑 */
	UPROPERTY(BlueprintReadOnly, Category = "Final Force")
	FVector Force = FVector::ZeroVector;

	/** 施力点位置（世界空间，cm） */
	UPROPERTY(BlueprintReadOnly, Category = "Final Force")
	FVector ApplicationPoint = FVector::ZeroVector;

	/** 是否应该施加该力 */
	UPROPERTY(BlueprintReadOnly, Category = "Final Force")
	bool bShouldApply = false;
};

/**
 * 完整的系统配置结构
 * 集中管理所有力系统参数的配置
 * 注意：所有力相关参数使用 UE 物理单位体系（长度 cm，质量 kg，时间 s）
 */
USTRUCT(BlueprintType)
struct FForceSystemConfig
{
	GENERATED_BODY()

	// ===== 接触探测配置 =====
	
	/**
	 * ComponentSweep 沿每个方向的位移量 (cm)
	 * 应 >= TargetContactGap，保证能探测到目标间隙内的接触。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contact Detection")
	float ContactDetectionRadius = 8.0f;

	/**
	 * 目标接触间隙 (cm)
	 * ContactMetric = max(0, TargetContactGap - ActualGap)
	 * 建议 < ContactDetectionRadius，让接近过程有一段缓冲区。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contact Detection")
	float TargetContactGap = 3.0f;

	/** 接触度量最小阈值，低于此值视为无效（通常保持 0，由置信度负责稳定） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contact Detection")
	float ContactMetricMinThreshold = 0.0f;

	/** 置信度增加速率 (1/s)，检测到接触时置信度增长速度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contact Detection")
	float ConfidenceIncreaseRate = 6.0f;

	/** 置信度减少速率 (1/s)，丢失接触时置信度衰减速度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contact Detection")
	float ConfidenceDecreaseRate = 2.0f;

	/** 稳定置信度阈值 [0, 1]，置信度超过此值时接触片视为稳定 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contact Detection")
	float StableConfidenceThreshold = 0.45f;

	/**
	 * 衰减施力时间窗口 (秒)
	 * EarlyExit 路径中，丢失接触后最多允许继续施加衰减力的时长。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contact Detection")
	float DecayApplyWindowSeconds = 0.15f;

	// ===== 力大小限制 =====
	
	/** 最大支撑力（UE 力单位：kg·cm/s²），限制单个支撑臂的法向支撑力上限；初始默认值需按车型和质量重新调参 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Force Limits")
	float MaxSupportForce = 5000.0f;

	/** 最大防滑力（UE 力单位：kg·cm/s²），限制单个支撑臂的切向防滑力上限；初始默认值需按车型和质量重新调参 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Force Limits")
	float MaxAntiSlipForce = 3000.0f;

	/** 最大牵引辅助力（UE 力单位：kg·cm/s²），限制单个支撑臂的牵引辅助力上限；初始默认值需按车型和质量重新调参 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Force Limits")
	float MaxTractionAssistForce = 2000.0f;

	// ===== 支撑力参数 =====
	
	/**
	 * 支撑力刚度系数（kg/s²）
	 * F_support = Stiffness * ContactMetric(cm) - Damping * Vn
	 * ContactMetric 现为真实压缩量(cm)。
	 * 满压缩(TargetContactGap=3cm)时出力 ≈ Stiffness * 3。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Support Force")
	float SupportForceStiffness = 1000.0f;

	/** 支撑力阻尼系数（kg/s），用于减少接触切换时的振荡 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Support Force")
	float SupportForceDamping = 1500.0f;

	// ===== 防滑力参数 =====
	
	/** 防滑力增益（kg/s），F_antislip = Gain * |V_tangential| */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anti-Slip Force")
	float AntiSlipForceGain = 100.0f;

	/** 防滑力速度死区 (cm/s)，切向速度低于此值时不施加防滑力 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anti-Slip Force")
	float AntiSlipVelocityDeadband = 0.1f;

	// ===== 牵引辅助力参数 =====
	
	/** 牵引辅助油门增益（UE 力单位：kg·cm/s²），F_traction = Gain * |Throttle| * ContactDeficit * Confidence */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traction Assist Force")
	float TractionAssistThrottleGain = 1000.0f;

	/** 轮地接触比例阈值 [0, 1]，低于此值时启用牵引辅助 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traction Assist Force")
	float WheelContactRatioThreshold = 0.5f;

	/** 偏航输入阈值 [0, 1]，偏航输入低于此值时启用牵引辅助 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traction Assist Force")
	float YawInputThreshold = 0.3f;

	/** 油门输入阈值 [0, 1]，油门输入高于此值时启用牵引辅助 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traction Assist Force")
	float ThrottleInputThreshold = 0.1f;

	// ===== 约束参数 =====
	
	/** 摩擦系数 μ，用于摩擦锥约束 |Ft| <= μ * Fn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Constraints")
	float FrictionCoefficient = 0.8f;

	/** 底盘接触降额因子 [0, 1]，底盘接触时牵引辅助力的缩放比例 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Constraints")
	float ChassisContactReductionFactor = 0.5f;

	/** 最大俯仰扭矩（UE 扭矩单位：kg·cm²/s²），限制所有支撑臂力产生的总俯仰扭矩；初始默认值需按车型和质量重新调参 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Constraints")
	float MaxPitchTorque = 50000.0f;

	/** 最大横滚扭矩（UE 扭矩单位：kg·cm²/s²），限制所有支撑臂力产生的总横滚扭矩；初始默认值需按车型和质量重新调参 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Constraints")
	float MaxRollTorque = 40000.0f;

	// ===== 平滑参数 =====
	
	/** 力方向平滑速率 (1/s)，控制力方向变化的平滑程度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smoothing")
	float ForceDirectionSmoothingRate = 5.0f;

	/** 力 Jerk 限制（UE 力单位/秒：kg·cm/s³），限制力大小的变化率，防止突变 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smoothing")
	float ForceJerkLimit = 50000.0f;

	/** 力衰减速率 (1/s)，无接触时缓存力的指数衰减速率 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smoothing")
	float ForceDecayRate = 3.0f;
};
