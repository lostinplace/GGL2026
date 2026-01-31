// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ProjectChairs : ModuleRules
{
	public ProjectChairs(ReadOnlyTargetRules Target) : base(Target)
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
			"Slate",
			"ChessGame"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"ProjectChairs",
			"ProjectChairs/Variant_Platforming",
			"ProjectChairs/Variant_Platforming/Animation",
			"ProjectChairs/Variant_Combat",
			"ProjectChairs/Variant_Combat/AI",
			"ProjectChairs/Variant_Combat/Animation",
			"ProjectChairs/Variant_Combat/Gameplay",
			"ProjectChairs/Variant_Combat/Interfaces",
			"ProjectChairs/Variant_Combat/UI",
			"ProjectChairs/Variant_SideScrolling",
			"ProjectChairs/Variant_SideScrolling/AI",
			"ProjectChairs/Variant_SideScrolling/Gameplay",
			"ProjectChairs/Variant_SideScrolling/Interfaces",
			"ProjectChairs/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
