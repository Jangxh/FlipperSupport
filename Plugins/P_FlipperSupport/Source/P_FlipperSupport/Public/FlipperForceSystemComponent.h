// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FlipperForceSystemTypes.h"
#include "FlipperForceSystemComponent.generated.h"

// 前向声明
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

/**
 * 支撑臂力系统主组件
 * 
 * 职责：
 * - 协调所有子模块，管理整体执行流程
 * - 每帧探测接触、构建状态、计算力、应用约束、施加力
 * - 提供配置接口和调试可视化
 * 
 * 架构：
 * - 采用模块化设计，将功能分离为 8 个独立的 UObject 子模块
 * - 通过管道式处理流程，从接触探测到最终力施加
 * - 完全独立于 OnComponentHit 事件，使用主动物理查询
 * 
 * 执行流程：
 * 1. ContactSensor->DetectContacts() - 探测接触
 * 2. StateProvider->BuildState() - 构建车辆状态
 * 3. ShouldEarlyExit() - 提前退出检查
 * 4. 计算三类力（Support、AntiSlip、TractionAssist）
 * 5. ForceComposer->ComposeForces() - 组合力并应用摩擦锥约束
 * 6. SafetyFilter->FilterForces() - 安全过滤和平滑
 * 7. 更新 PreviousForce 历史
 * 8. ForceApplicator->ApplyForces() - 施加力到主刚体
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class P_FLIPPERSUPPORT_API UFlipperForceSystemComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/**
	 * 构造函数
	 * 设置默认值和组件属性
	 */
	UFlipperForceSystemComponent();

	/**
	 * 组件初始化
	 * 创建所有子模块实例，执行引用校验
	 */
	virtual void BeginPlay() override;

	/**
	 * 每帧更新
	 * 执行完整的力计算和施加管道
	 * @param DeltaTime 帧时间间隔（秒）
	 * @param TickType Tick 类型
	 * @param ThisTickFunction Tick 函数结构
	 */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, 
	                          FActorComponentTickFunction* ThisTickFunction) override;

	// ===== 组件名称配置 =====

	/** 车体主刚体组件名称，留空则自动查找 RootComponent */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Component Names")
	FName RootPrimitiveName;

	/** 坦克运动组件名称，留空则自动查找第一个 UC_TankMovementComponent */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Component Names")
	FName TankMovementComponentName;

	/** 前左支撑臂组件名称 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Component Names")
	FName FrontLeftFlipperName;

	/** 前右支撑臂组件名称 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Component Names")
	FName FrontRightFlipperName;

	/** 后左支撑臂组件名称 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Component Names")
	FName RearLeftFlipperName;

	/** 后右支撑臂组件名称 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Component Names")
	FName RearRightFlipperName;

	// ===== 系统配置 =====

	/** 完整的力系统配置结构，集中管理所有参数 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	FForceSystemConfig Config;

	// ===== 运行时控制 =====

	/** 是否启用力系统，禁用时跳过所有计算和力施加 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Control")
	bool bEnableSystem = true;

	/** 是否启用调试可视化，绘制接触点、法线和力矢量 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bEnableDebugVisualization = false;

private:
	// ===== 缓存的组件引用 =====

	/** 车体主刚体，接收所有计算出的力 */
	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> RootPrimitive = nullptr;

	/** 坦克运动组件，提供轮系推进和输入信息 */
	UPROPERTY(Transient)
	TObjectPtr<UC_TankMovementComponent> TankMovementComponent = nullptr;

	/** 四个支撑臂的刚体组件数组 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPrimitiveComponent>> FlipperPrimitives;

	// ===== 子模块实例 =====

	/** 接触探测模块，主动探测四个支撑臂的接触状态 */
	UPROPERTY()
	TObjectPtr<UFlipperContactSensor> ContactSensor = nullptr;

	/** 车辆状态提供模块，读取车辆状态并生成状态快照 */
	UPROPERTY()
	TObjectPtr<UVehicleStateProvider> StateProvider = nullptr;

	/** 支撑力求解模块，计算法向支撑力需求 */
	UPROPERTY()
	TObjectPtr<UFlipperSupportSolver> SupportSolver = nullptr;

	/** 防滑力求解模块，计算抗滑切向力需求 */
	UPROPERTY()
	TObjectPtr<UFlipperAntiSlipSolver> AntiSlipSolver = nullptr;

	/** 牵引辅助力求解模块，计算推进辅助切向力需求 */
	UPROPERTY()
	TObjectPtr<UFlipperTractionAssistSolver> TractionAssistSolver = nullptr;

	/** 力组合模块，组合三类力并应用摩擦锥约束 */
	UPROPERTY()
	TObjectPtr<UFlipperForceComposer> ForceComposer = nullptr;

	/** 安全过滤模块，对候选力应用安全裁剪和平滑 */
	UPROPERTY()
	TObjectPtr<UFlipperForceSafetyFilter> SafetyFilter = nullptr;

	/** 力施加模块，将最终力施加到车体主刚体 */
	UPROPERTY()
	TObjectPtr<UFlipperForceApplicator> ForceApplicator = nullptr;

	// ===== 运行时数据 =====

	/** 当前帧的接触片数据数组（4个元素） */
	TArray<FContactPatchData> ContactPatches;

	/** 当前帧的车辆状态快照 */
	FFlipperVehicleState VehicleState;

	/** 候选力数组，力组合器输出 */
	TArray<FCandidateForce> CandidateForces;

	/** 最终力数组，安全过滤器输出 */
	TArray<FFinalForce> FinalForces;

	// ===== 私有方法 =====

	/**
	 * 初始化所有子模块
	 * 创建子模块实例并配置参数
	 */
	void InitializeSubmodules();

	/**
	 * 解析并缓存组件引用
	 * 根据配置的组件名称查找并缓存所需的组件引用
	 */
	void ResolveAndCacheComponents();

	/**
	 * 根据名称查找 PrimitiveComponent
	 * @param CompName 组件名称
	 * @return 找到的组件指针，未找到返回 nullptr
	 */
	UPrimitiveComponent* FindPrimitiveByName(const FName& CompName) const;

	/**
	 * 验证所有必需的外部引用
	 * @return 引用是否全部有效
	 */
	bool ValidateReferences();

	/**
	 * 主执行管道
	 * 按顺序执行接触探测、状态构建、力计算、约束应用、力施加
	 * @param DeltaTime 帧时间间隔（秒）
	 */
	void ExecutePipeline(float DeltaTime);

	/**
	 * 提前退出检查
	 * 检查是否存在至少一个有效且稳定的接触片
	 * @return 是否应该提前退出（无有效接触）
	 */
	bool ShouldEarlyExit();

	/**
	 * 调试可视化
	 * 绘制接触点、法线和力矢量
	 */
	void DrawDebugVisualization();
};
