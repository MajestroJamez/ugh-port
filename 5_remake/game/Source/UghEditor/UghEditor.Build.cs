using UnrealBuildTool;

/**
 * The editor's part of the game: the commandlets that make the materials (UghMakeAssets), import the 3D assets
 * (UghImportAssets) and copy the assets of the Electric Dreams sample (UghCopyElectricDreams).
 */
public class UghEditor : ModuleRules
{
	public UghEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;
		PrivateDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "UnrealEd", "MaterialEditor", "AssetRegistry", "Json",
			"InterchangeCore", "InterchangeEngine",   // the import of glTF, FBX and images
			"UghGame"   // UghMaterials.h, UghAssets.h: the names the game uses
		});
	}
}
