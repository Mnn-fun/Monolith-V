using UnrealBuildTool;
using System.Collections.Generic;

// Dedicated server target — produces the headless Linux binary deployed in
// P2.1 (local VM) and P5.3 (Oracle Cloud production VM).
public class MonolithVServerTarget : TargetRules
{
	public MonolithVServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		ExtraModuleNames.AddRange(new string[] { "MonolithV" });
	}
}
