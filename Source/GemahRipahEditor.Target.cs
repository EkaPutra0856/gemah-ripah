using UnrealBuildTool;
using System.Collections.Generic;

public class GemahRipahEditorTarget : TargetRules
{
	public GemahRipahEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("GemahRipah");
	}
}
