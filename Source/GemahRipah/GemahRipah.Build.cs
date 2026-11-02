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
			"SlateCore"
		});

		// Subfolders (Core/, Player/, Systems/, ...) are included relative to the module root.
		PublicIncludePaths.Add(ModuleDirectory);
	}
}
