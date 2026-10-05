using UnrealBuildTool;

/** The frontend: draws the logic (module UghLogic, through its C API) as a diorama (rock mesh, plasticine figures, campfire, imported decorations) and feeds it the keys. */
public class UghGame : ModuleRules
{
	public UghGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;
		PublicIncludePaths.Add(ModuleDirectory);   // UghMaterials.h, UghAssets.h for the editor module
		bUseUnity = false;   // each file has its own file-local constants (Width, Depth ...)

		PrivateDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "InputCore", "Json", "ImageWrapper", "UghLogic",
			"Slate", "SlateCore",   // the menu and the HUD (UghUi)
			"MeshDescription", "StaticMeshDescription",   // the rock as a static mesh built at run time (FUghRockMesh)
			"AssetRegistry",   // the imported assets (UghAssets)
			"HairStrandsCore",   // the MetaHumans' grooms (UghMetaHumans)
			"DLSSBlueprint", "StreamlineDLSSGBlueprint"   // the upscalers (plugins DLSS, Streamline; FSR needs no code)
		});
	}
}
