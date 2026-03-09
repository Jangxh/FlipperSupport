// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlipperContactSensor.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

void UFlipperContactSensor::Initialize(UPrimitiveComponent* InRootPrimitive, const TArray<UPrimitiveComponent*>& InFlipperPrimitives)
{
	RootPrimitive = InRootPrimitive;
	FlipperPrimitives = InFlipperPrimitives;

	// 初始化历史数据数组，为四个支撑臂预分配空间
	PreviousPatches.SetNum(4);
	for (int32 i = 0; i < 4; ++i)
	{
		PreviousPatches[i].FlipperIndex = static_cast<EFlipperIndex>(i);
		PreviousPatches[i].bIsValid = false;
		PreviousPatches[i].Confidence = 0.0f;
		PreviousPatches[i].bIsStable = false;
	}

	UE_LOG(LogTemp, Log, TEXT("FlipperContactSensor: Initialized with %d flipper primitives"), FlipperPrimitives.Num());
}

void UFlipperContactSensor::DetectContacts(float DeltaTime, TArray<FContactPatchData>& OutContactPatches)
{
	// 确保输出数组大小为 4
	OutContactPatches.SetNum(4);

	// 对每个支撑臂进行接触探测
	for (int32 i = 0; i < 4; ++i)
	{
		// 初始化当前帧的接触片数据
		FContactPatchData& CurrentPatch = OutContactPatches[i];
		CurrentPatch.FlipperIndex = static_cast<EFlipperIndex>(i);

		// 执行物理查询
		FHitResult HitResult;
		const bool bDetected = DetectSingleFlipperContact(i, HitResult);

		if (bDetected)
		{
			// 检测到有效接触
			CurrentPatch.bIsValid = true;
			CurrentPatch.ContactPoint = HitResult.ImpactPoint;
			CurrentPatch.ContactNormal = HitResult.ImpactNormal;

			// 计算接触度量（使用穿透深度，若无穿透则使用距离的倒数）
			// 注：这里使用简化模型，实际可根据 HitResult.PenetrationDepth 或 HitResult.Distance 调整
			CurrentPatch.ContactMetric = FMath::Max(0.0f, HitResult.PenetrationDepth);
			if (CurrentPatch.ContactMetric < KINDA_SMALL_NUMBER)
			{
				// 若无穿透深度，使用距离的倒数作为接近度量
				const float Distance = HitResult.Distance;
				if (Distance > KINDA_SMALL_NUMBER)
				{
					CurrentPatch.ContactMetric = FMath::Clamp(DetectionRadius / Distance, 0.0f, ContactMetricMaxThreshold);
				}
			}

			// 检查接触度量是否在有效范围内
			if (CurrentPatch.ContactMetric < ContactMetricMinThreshold)
			{
				CurrentPatch.bIsValid = false;
			}
		}
		else
		{
			// 未检测到接触
			CurrentPatch.bIsValid = false;
			CurrentPatch.ContactMetric = 0.0f;
		}

		// 从上一帧恢复置信度和力历史
		if (i < PreviousPatches.Num())
		{
			CurrentPatch.Confidence = PreviousPatches[i].Confidence;
			CurrentPatch.PreviousForce = PreviousPatches[i].PreviousForce;
		}

		// 更新置信度
		UpdateConfidence(CurrentPatch, CurrentPatch.bIsValid, DeltaTime);

		// 计算接触动力学
		if (CurrentPatch.bIsValid)
		{
			ComputeContactDynamics(CurrentPatch);
		}
		else
		{
			CurrentPatch.ContactVelocity = FVector::ZeroVector;
			CurrentPatch.TangentialVelocity = FVector::ZeroVector;
		}

		// 更新时间戳
		if (RootPrimitive && RootPrimitive->GetWorld())
		{
			CurrentPatch.LastUpdateTime = RootPrimitive->GetWorld()->GetTimeSeconds();
		}
	}

	// 保存当前帧数据作为下一帧的历史
	PreviousPatches = OutContactPatches;
}

void UFlipperContactSensor::SetDetectionRadius(float Radius)
{
	DetectionRadius = FMath::Max(0.0f, Radius);
}

void UFlipperContactSensor::SetContactMetricThresholds(float MinThreshold, float MaxThreshold)
{
	ContactMetricMinThreshold = FMath::Max(0.0f, MinThreshold);
	ContactMetricMaxThreshold = FMath::Max(ContactMetricMinThreshold, MaxThreshold);
}

void UFlipperContactSensor::SetConfidenceRates(float IncreaseRate, float DecreaseRate)
{
	ConfidenceIncreaseRate = FMath::Max(0.0f, IncreaseRate);
	ConfidenceDecreaseRate = FMath::Max(0.0f, DecreaseRate);
}

void UFlipperContactSensor::SetStableConfidenceThreshold(float Threshold)
{
	StableConfidenceThreshold = FMath::Clamp(Threshold, 0.0f, 1.0f);
}

bool UFlipperContactSensor::DetectSingleFlipperContact(int32 FlipperIndex, FHitResult& OutHit)
{
	// 验证索引和组件有效性
	if (!FlipperPrimitives.IsValidIndex(FlipperIndex) || !FlipperPrimitives[FlipperIndex])
	{
		return false;
	}

	UPrimitiveComponent* FlipperPrimitive = FlipperPrimitives[FlipperIndex];
	UWorld* World = FlipperPrimitive->GetWorld();
	if (!World)
	{
		return false;
	}

	// 获取支撑臂的位置和方向
	// 注：这里使用支撑臂组件的底部中心作为探测起点
	// 实际项目中可以使用 Socket 或自定义偏移点
	const FVector FlipperLocation = FlipperPrimitive->GetComponentLocation();
	const FVector FlipperDown = -FlipperPrimitive->GetUpVector();

	// 探测起点：支撑臂位置
	const FVector TraceStart = FlipperLocation;
	// 探测终点：沿支撑臂下方向延伸探测半径
	const FVector TraceEnd = TraceStart + FlipperDown * DetectionRadius;

	// 设置碰撞查询参数
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(FlipperPrimitive->GetOwner()); // 忽略自身
	QueryParams.bTraceComplex = false; // 使用简单碰撞
	QueryParams.bReturnPhysicalMaterial = true; // 返回物理材质信息

	// 执行线性追踪
	const bool bHit = World->LineTraceSingleByChannel(
		OutHit,
		TraceStart,
		TraceEnd,
		ECC_Visibility, // 使用 Visibility 通道，可根据需要调整
		QueryParams
	);

	return bHit;
}

void UFlipperContactSensor::UpdateConfidence(FContactPatchData& Patch, bool bDetected, float DeltaTime)
{
	if (bDetected)
	{
		// 检测到接触，增加置信度
		Patch.Confidence += ConfidenceIncreaseRate * DeltaTime;
		Patch.Confidence = FMath::Clamp(Patch.Confidence, 0.0f, 1.0f);
	}
	else
	{
		// 丢失接触，减少置信度
		Patch.Confidence -= ConfidenceDecreaseRate * DeltaTime;
		Patch.Confidence = FMath::Clamp(Patch.Confidence, 0.0f, 1.0f);
	}

	// 根据置信度阈值设置稳定标志
	Patch.bIsStable = (Patch.Confidence >= StableConfidenceThreshold);
}

void UFlipperContactSensor::ComputeContactDynamics(FContactPatchData& Patch)
{
	if (!RootPrimitive)
	{
		Patch.ContactVelocity = FVector::ZeroVector;
		Patch.TangentialVelocity = FVector::ZeroVector;
		return;
	}

	// 获取车体线速度和角速度
	const FVector LinearVelocity = RootPrimitive->GetPhysicsLinearVelocity();
	const FVector AngularVelocity = RootPrimitive->GetPhysicsAngularVelocityInRadians();

	// 计算从质心到接触点的矢量
	const FVector CenterOfMass = RootPrimitive->GetCenterOfMass();
	const FVector r = Patch.ContactPoint - CenterOfMass;

	// 计算接触点速度：V_contact = V_linear + ω × r
	const FVector CrossProduct = FVector::CrossProduct(AngularVelocity, r);
	Patch.ContactVelocity = LinearVelocity + CrossProduct;

	// 投影切向速度到接触平面
	// V_tangential = V_contact - (V_contact · N) * N
	const FVector& N = Patch.ContactNormal;
	const float NormalComponent = FVector::DotProduct(Patch.ContactVelocity, N);
	Patch.TangentialVelocity = Patch.ContactVelocity - (NormalComponent * N);
}
