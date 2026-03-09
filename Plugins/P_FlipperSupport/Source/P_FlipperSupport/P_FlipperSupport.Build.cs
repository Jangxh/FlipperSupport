// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class P_FlipperSupport : ModuleRules
{
	public P_FlipperSupport(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"P_TankVehicle",
				"ChaosVehicles",
			}
			);
	}
}
