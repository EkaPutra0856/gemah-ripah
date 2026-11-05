using UnrealBuildTool;

public class GemahRipah : ModuleRules
{
	public GemahRipah(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"GameplayTags",
			"DeveloperSettings",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"SlateCore",
			"Json",
			"JsonUtilities",
			"MoviePlayer",
			"IKRig",
			"AnimationCore",
			"Landscape"
		});

		// Editor-only builders (Core/GRScriptLibrary: BuildTerrain).
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "AssetRegistry" });
		}

		// Subfolders (Core/, Player/, Systems/, ...) are included relative to the module root.
		PublicIncludePaths.Add(ModuleDirectory);
	}
}
