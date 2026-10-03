using UnrealBuildTool;

/** The frontend: draws the logic (module UghLogic, through its C API) as a diorama (rock mesh, plasticine figures, campfire) and feeds it the keys. */
public class UghGame : ModuleRules
{
	public UghGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;
		PublicIncludePaths.Add(ModuleDirectory);   // UghMaterials.h for the editor module
		bUseUnity = false;   // each file has its own file-local constants (Width, Depth ...)

		PrivateDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "InputCore", "Json", "ImageWrapper", "ProceduralMeshComponent", "UghLogic",
			"DLSSBlueprint", "StreamlineDLSSGBlueprint"   // the upscalers (plugins DLSS, Streamline; FSR needs no code)
		});
	}
}
