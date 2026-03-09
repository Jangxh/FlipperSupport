// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class P_TankVehicle : ModuleRules
{
	public P_TankVehicle(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"ChaosVehicles",
				"PhysicsCore",
			});

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
			});
	}
}
