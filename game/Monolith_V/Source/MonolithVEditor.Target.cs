using UnrealBuildTool;
using System.Collections.Generic;

public class MonolithVEditorTarget : TargetRules
{
	public MonolithVEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		ExtraModuleNames.AddRange(new string[] { "MonolithV" });
	}
}
