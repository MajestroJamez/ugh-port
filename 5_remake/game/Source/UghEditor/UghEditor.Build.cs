using UnrealBuildTool;

/** The editor's part of the game: the commandlet that makes the materials (UghMakeAssets). */
public class UghEditor : ModuleRules
{
	public UghEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;
		PrivateDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "UnrealEd", "MaterialEditor", "AssetRegistry",
			"UghGame"   // UghMaterials.h: the names the game uses
		});
	}
}
