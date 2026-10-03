using UnrealBuildTool;

/** The frontend: draws the logic (module UghLogic, through its C API) as grey boxes and feeds it the keys. */
public class UghGame : ModuleRules
{
	public UghGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;

		PrivateDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "InputCore", "Json", "UghLogic",
			"DLSSBlueprint", "StreamlineDLSSGBlueprint"   // the upscalers (plugins DLSS, Streamline; FSR needs no code)
		});
	}
}
