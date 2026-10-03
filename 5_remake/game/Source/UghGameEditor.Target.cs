using UnrealBuildTool;

public class UghGameEditorTarget : TargetRules
{
	public UghGameEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.AddRange(new string[] { "UghLogic", "UghGame", "UghEditor" });
	}
}
