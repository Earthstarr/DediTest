// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DediTest : ModuleRules
{
	public DediTest(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"DediTest",
			"DediTest/Variant_Platforming",
			"DediTest/Variant_Platforming/Animation",
			"DediTest/Variant_Combat",
			"DediTest/Variant_Combat/AI",
			"DediTest/Variant_Combat/Animation",
			"DediTest/Variant_Combat/Gameplay",
			"DediTest/Variant_Combat/Interfaces",
			"DediTest/Variant_Combat/UI",
			"DediTest/Variant_SideScrolling",
			"DediTest/Variant_SideScrolling/AI",
			"DediTest/Variant_SideScrolling/Gameplay",
			"DediTest/Variant_SideScrolling/Interfaces",
			"DediTest/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
