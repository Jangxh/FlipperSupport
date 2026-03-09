// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlipperForceComposer.h"

void UFlipperForceComposer::ComposeForces(const TArray<FContactPatchData>& ContactPatches,
                                          const TArray<FVector>& SupportForces,
                                          const TArray<FVector>& AntiSlipForces,
                                          const TArray<FVector>& TractionAssistForces,
                                          TArray<FCandidateForce>& OutCandidateForces)
{
	// 清空输出数组
	OutCandidateForces.Reset();

	// 验证输入数组大小一致性
	const int32 NumPatches = ContactPatches.Num();
	if (SupportForces.Num() != NumPatches || 
	    AntiSlipForces.Num() != NumPatches || 
	    TractionAssistForces.Num() != NumPatches)
	{
		UE_LOG(LogTemp, Warning, TEXT("FlipperForceComposer: 输入数组大小不一致，跳过力组合"));
		return;
	}

	// 对每个接触片进行力组合
	for (int32 i = 0; i < NumPatches; ++i)
	{
		const FContactPatchData& Patch = ContactPatches[i];
		
		// 创建候选力结构
		FCandidateForce CandidateForce;
		CandidateForce.FlipperIndex = Patch.FlipperIndex;
		CandidateForce.ApplicationPoint = Patch.ContactPoint;
		
		// 仅处理有效且稳定的接触片
		if (!Patch.bIsValid || !Patch.bIsStable)
		{
			CandidateForce.bIsValid = false;
			CandidateForce.SupportForce = FVector::ZeroVector;
			CandidateForce.AntiSlipForce = FVector::ZeroVector;
			CandidateForce.TractionAssistForce = FVector::ZeroVector;
			CandidateForce.TotalForce = FVector::ZeroVector;
			OutCandidateForces.Add(CandidateForce);
			continue;
		}

		// 获取三类力
		CandidateForce.SupportForce = SupportForces[i];
		CandidateForce.AntiSlipForce = AntiSlipForces[i];
		CandidateForce.TractionAssistForce = TractionAssistForces[i];

		// 计算法向力大小 Fn
		const float Fn = CandidateForce.SupportForce.Size();

		// 计算切向力 Ft = AntiSlipForce + TractionAssistForce
		FVector Ft = CandidateForce.AntiSlipForce + CandidateForce.TractionAssistForce;
		const float FtMag = Ft.Size();

		// 检查摩擦锥约束：|Ft| <= μ * Fn
		const float MaxTangentialForce = FrictionCoefficient * Fn;
		
		if (FtMag > MaxTangentialForce && FtMag > KINDA_SMALL_NUMBER)
		{
			// 违反约束，缩放切向力以满足约束
			// 保持方向，仅缩放大小
			const float ScaleFactor = MaxTangentialForce / FtMag;
			Ft *= ScaleFactor;
			
			// 按比例缩放两个切向力分量
			CandidateForce.AntiSlipForce *= ScaleFactor;
			CandidateForce.TractionAssistForce *= ScaleFactor;
		}

		// 计算总力
		CandidateForce.TotalForce = CandidateForce.SupportForce + Ft;
		CandidateForce.bIsValid = true;

		OutCandidateForces.Add(CandidateForce);
	}
}

void UFlipperForceComposer::SetFrictionCoefficient(float Mu)
{
	FrictionCoefficient = FMath::Max(0.0f, Mu);
}

void UFlipperForceComposer::ApplyFrictionConeConstraint(FCandidateForce& Force)
{
	// 计算法向力大小 Fn
	const float Fn = Force.SupportForce.Size();

	// 计算切向力 Ft = AntiSlipForce + TractionAssistForce
	FVector Ft = Force.AntiSlipForce + Force.TractionAssistForce;
	const float FtMag = Ft.Size();

	// 检查摩擦锥约束：|Ft| <= μ * Fn
	const float MaxTangentialForce = FrictionCoefficient * Fn;
	
	if (FtMag > MaxTangentialForce && FtMag > KINDA_SMALL_NUMBER)
	{
		// 违反约束，缩放切向力以满足约束
		// 保持方向，仅缩放大小
		const float ScaleFactor = MaxTangentialForce / FtMag;
		
		// 按比例缩放两个切向力分量
		Force.AntiSlipForce *= ScaleFactor;
		Force.TractionAssistForce *= ScaleFactor;
		
		// 重新计算总力
		Force.TotalForce = Force.SupportForce + Force.AntiSlipForce + Force.TractionAssistForce;
	}
}
