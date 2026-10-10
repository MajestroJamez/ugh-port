// The scanned assets of Epic's Electric Dreams sample the game uses.
#pragma once

#include "CoreMinimal.h"

class UMaterialInterface;
class UMaterialParameterCollection;
class UStaticMesh;
class UTexture;

/**
 * The assets of Epic's "Electric Dreams Environment" sample (Megascans: scanned rocks, cliffs, roots, jungle plants;
 * licensed for Unreal Engine projects, not ours to share) the frontend uses, by their paths in the sample's content
 * (its /Game/<path>). electric-dreams.ps1 copies them with everything they need (the commandlet UghCopyElectricDreams)
 * to Content/External/ElectricDreams, so the sample's /Game/<path> is <Root>/<path> here: the [CoreRedirects] of
 * Config/DefaultEngine.ini send the sample's references there. Nothing of it is in git, so a lookup may find nothing:
 * the frontend then shows the free assets of UghAssets (or what it shows without them), and the log says it.
 */
namespace UghElectricDreams
{
	/** Where the copy is: the sample's /Game/<path> is <Root>/<path>. */
	inline const TCHAR* Root = TEXT("/Game/External/ElectricDreams");

	/**
	 * The decorations (AUghScenery, by kind of UghDecorations): tufts of grass, flowers, stones, ferns, bushes, jungle
	 * plants with big leaves, stumps, palms, lianas (strands from the ceilings, curtains down the back wall).
	 */
	inline const TCHAR* const Grasses[] = { TEXT("Megascans/3D_Plants/KikuyuGrass/SM_KikuyuGrass_01"),
		TEXT("Megascans/3D_Plants/KikuyuGrass/SM_KikuyuGrass_03"),
		TEXT("Megascans/3D_Plants/KikuyuGrass/SM_KikuyuGrass_05"),
		TEXT("Megascans/3D_Plants/KikuyuGrass/SM_KikuyuGrass_06") };
	inline const TCHAR* const Flowers[] = { TEXT("Megascans/3D_Plants/WhiteWindflower/SM_WhiteWindflower_01"),
		TEXT("Megascans/3D_Plants/WhiteWindflower/SM_WhiteWindflower_03"),
		TEXT("Megascans/3D_Plants/RedLachenalia/SM_RedLachenalia_06"),
		TEXT("Megascans/3D_Plants/Amaryllis/SM_Amaryllis_03"),
		TEXT("Megascans/3D_Plants/GroundCover/SM_GroundCover_07") };
	inline const TCHAR* const Rocks[] = { TEXT("Megascans/3D_Assets/SmallStonesPack/SM_SmallStonesPack_01"),
		TEXT("Megascans/3D_Assets/SmallStonesPack/SM_SmallStonesPack_03"),
		TEXT("Megascans/3D_Assets/SmallStonesPack/SM_SmallStonesPack_05"),
		TEXT("Megascans/3D_Assets/MossyForestRock/SM_MossyForestRock_01"),
		TEXT("Megascans/3D_Assets/LichenedForestBoulder/SM_LichenedForestBoulder_01") };
	inline const TCHAR* const Ferns[] = { TEXT("Megascans/3D_Plants/Fern/SM_Fern_01"),
		TEXT("Megascans/3D_Plants/Fern/SM_Fern_02"),
		TEXT("Megascans/3D_Plants/Fern/SM_Fern_05"), TEXT("Megascans/3D_Plants/BeechFern/SM_BeechFern_04"),
		TEXT("Megascans/3D_Plants/SilverLadyFern/SM_SilverLadyFern_01"),
		TEXT("Megascans/3D_Plants/BostonFern/SM_BostonFern_02") };
	inline const TCHAR* const Bushes[] = { TEXT("Megascans/3D_Plants/ArrowheadPlant/SM_ArrowheadPlant_01"),
		TEXT("Megascans/3D_Plants/ArrowheadPlant/SM_ArrowheadPlant_02"),
		TEXT("Megascans/3D_Plants/ArrowheadPlant/SM_ArrowheadPlant_05"),
		TEXT("Megascans/3D_Plants/VariegatedCroton/SM_VariegatedCroton_02"),
		TEXT("Megascans/3D_Plants/CastorOilPlant/SM_CastorOilPlant_01") };
	inline const TCHAR* const Plants[] = { TEXT("Megascans/3D_Plants/Taro/SM_Taro_02"),
		TEXT("Megascans/3D_Plants/Taro/SM_Taro_05"),
		TEXT("Megascans/3D_Plants/Taro/SM_Taro_06"), TEXT("Megascans/3D_Plants/Taro/SM_Taro_10"),
		TEXT("Megascans/3D_Plants/ArrowheadPlant/SM_ArrowheadPlant_04"),
		TEXT("Megascans/3D_Plants/PinkCordyline/SM_PinkCordyline_02"),
		TEXT("Megascans/3D_Plants/VariegatedCroton/SM_VariegatedCroton_01"),
		TEXT("Megascans/3D_Plants/BirdOfParadise/SM_BirdOfParadise_01") };
	inline const TCHAR* const Stumps[] = { TEXT("Megascans/3D_Assets/TreeStump/SM_TreeStump_01"),
		TEXT("Megascans/3D_Assets/RottenTreeStump/SM_RottenTreeStump_02") };
	inline const TCHAR* const Palms[] = { TEXT("Megascans/3D_Plants/AlexandraPalm/SM_AlexandraPalm_01"),
		TEXT("Megascans/3D_Plants/AlexandraPalm/SM_AlexandraPalm_02"),
		TEXT("Megascans/3D_Plants/AlexandraPalm/SM_AlexandraPalm_03"),
		TEXT("Megascans/3D_Plants/FanPalm/SM_FanPalm_01"),
		TEXT("Megascans/3D_Plants/ArecaPalm/SM_ArecaPalm_03"), TEXT("Megascans/3D_Plants/CatPalm/SM_CatPalm_01") };
	inline const TCHAR* const Vines[] = { TEXT("Megascans/3D_Plants/Ivy/SM_Ivy_11"),
		TEXT("Megascans/3D_Plants/Ivy/SM_Ivy_13"),
		TEXT("Megascans/3D_Plants/Ivy/SM_Ivy_15"), TEXT("Megascans/3D_Plants/Ivy/SM_Ivy_17"),
		TEXT("Megascans/3D_Plants/Ivy/SM_Ivy_21"), TEXT("Megascans/3D_Plants/Ivy/SM_Ivy_23"),
		TEXT("Custom/IvyTest/SM_HangingVine_09"), TEXT("Custom/IvyTest/SM_HangingVine_10") };
	inline const TCHAR* const Creepers[] = { TEXT("Megascans/3D_Plants/Ivy/SM_Ivy_09"),
		TEXT("Megascans/3D_Plants/Ivy/SM_Ivy_12"),
		TEXT("Megascans/3D_Plants/Ivy/SM_Ivy_16"), TEXT("Megascans/3D_Plants/Ivy/SM_Ivy_18"),
		TEXT("Megascans/3D_Plants/Ivy/SM_Ivy_19"), TEXT("Megascans/3D_Plants/Ivy/SM_Ivy_22") };
	/**
	 * The campfires (AUghCampfire): a ring of scanned stones around branches burnt to charcoal; small stones its
	 * glowing coals.
	 */
	inline const TCHAR* const FireStones[] = { TEXT("Megascans/3D_Assets/SmallStonesPack/SM_SmallStonesPack_02"),
		TEXT("Megascans/3D_Assets/SmallStonesPack/SM_SmallStonesPack_04"),
		TEXT("Megascans/3D_Assets/SmallStonesPack/SM_SmallStonesPack_06"),
		TEXT("Megascans/3D_Assets/SmallStonesPack/SM_SmallStonesPack_08"),
		TEXT("Megascans/3D_Assets/SmallStonesPack/SM_SmallStonesPack_10") };
	inline const TCHAR* const FireLogs[] = { TEXT("Megascans/3D_Assets/OldTreeBranch/SM_OldTreeBranch_01"),
		TEXT("Megascans/3D_Assets/OldTreeBranch/SM_OldTreeBranch_03"),
		TEXT("Megascans/3D_Assets/DryBranches/SM_DryBranches_02"),
		TEXT("Megascans/3D_Assets/DryBranches/SM_DryBranches_05") };

	/**
	 * The scanned rock dressing the cave (UghRockDressing): grey cliffs of fractured rock on its back wall (their scans
	 * face +Y, the camera, and are open at the back), roots hanging from its ceilings (lying along +Y from the trunk's
	 * end).
	 */
	inline const TCHAR* const Roots[] = { TEXT("Custom/RootsTest/SM_Roots_01"), TEXT("Custom/RootsTest/SM_Roots_02"),
		TEXT("Custom/RootsTest/SM_Roots_03"), TEXT("Custom/RootsTest/SM_Roots_04"),
		TEXT("Custom/RootsTest/SM_Roots_05") };
	inline const TCHAR* const Cliffs[] = {
		TEXT("Megascans/3D_Assets/HugeNordicCoastalCliff/SM_HugeNordicCoastalCliff_01"),
		TEXT("Megascans/3D_Assets/HugeNordicCoastalCliff/SM_HugeNordicCoastalCliff_02"),
		TEXT("Megascans/3D_Assets/MassiveNordicCoastalCliff/SM_MassiveNordicCoastalCliff_01") };
	/** The texture parameters of the cliffs' materials (the sample's Megascans master): albedo, normal, packed. */
	inline const TCHAR* ScanAlbedo = TEXT("Albedo");
	inline const TCHAR* ScanNormal = TEXT("Normal");
	inline const TCHAR* ScanPacked = TEXT("DR");

	/**
	 * A layer of the cliff's material from a scanned surface of the sample: its layer (of UghMaterials::CliffLayers),
	 * its tiling textures (base colour, normal, a packed map with roughness in green), which channels of the packed map
	 * make its relief (HeightMask) and how many metres one texture covers.
	 */
	struct FCliffLayer
	{
		const TCHAR* Layer;
		const TCHAR* BaseColor;
		const TCHAR* Normal;
		const TCHAR* Packed;
		FLinearColor HeightMask;
		float Size;
	};
	/**
	 * Mossy grass, moss, the jungle's soil. The cliff's rock and its grey stone are free texture sets
	 * (UghAssets::CliffSets): a grey karst limestone, which the sample does not have.
	 */
	inline const FCliffLayer CliffLayers[] = {
		{ TEXT("Grass"), TEXT("Megascans/Surfaces/MossyGrass/T_MossyGrass_01_BC"),
		TEXT("Megascans/Surfaces/MossyGrass/T_MossyGrass_01_N"),
			TEXT("Megascans/Surfaces/MossyGrass/T_MossyGrass_01_AoRDp"), FLinearColor(0, 0, 1, 0), 1.5f },
		{ TEXT("Moss"), TEXT("Megascans/Surfaces/NordicMoss/T_NordicMoss_01_BC"),
		TEXT("Megascans/Surfaces/NordicMoss/T_NordicMoss_01_N"),
			TEXT("Megascans/Surfaces/NordicMoss/T_NordicMoss_01_AoRDp"), FLinearColor(0, 0, 1, 0), 1.5f },
		{ TEXT("Soil"), TEXT("Megascans/Surfaces/JungleGround/T_JungleGround_01_BC"),
		TEXT("Megascans/Surfaces/JungleGround/T_JungleGround_01_N"),
			TEXT("Megascans/Surfaces/JungleGround/T_JungleGround_01_AoRDp"), FLinearColor(0, 0, 1, 0), 2.f } };

	/**
	 * The leaves the people wear (FUghLeaves): the material of the sample's taro and where its big leaves are in its
	 * picture (UV: Min.Y the tip at the top, Max.Y cut across the leaf above the notch of its stalk, so that the leaf
	 * hangs from a straight edge).
	 */
	inline const TCHAR* LeafMaterial = TEXT("Megascans/3D_Plants/Taro/MI_Taro_02");
	inline const FBox2f LeafPictures[] = { FBox2f({ 0.020f, 0.045f }, { 0.225f, 0.290f }),
		FBox2f({ 0.260f, 0.020f }, { 0.515f, 0.340f }), FBox2f({ 0.545f, 0.030f }, { 0.785f, 0.330f }) };

	/** The parameters of the wind in the sample's plants (its scalars and vectors: 'Wind Strength Plants' ...). */
	inline const TCHAR* FoliageWind =
		TEXT("MSPresets/MS_Foliage_Material_LATEST/MaterialParameterCollection/MPC_GlobalFoliageActor");

	/**
	 * What Blender scripts make figures of (the commandlet UghExportElectricDreams exports them from the copy to
	 * assets/3d/electricdreams/<name>): for the jungle tree with a face (Blender/tree_jungle.py) the taro (its big
	 * leaves; a plant of the game too) and the scanned bark of a buttress root (textures, to the surface's folder).
	 * What only Blender wants is not cooked.
	 */
	inline const TCHAR* const ForBlender[] = { TEXT("Megascans/3D_Plants/Taro/SM_Taro_02"),
		TEXT("Megascans/Surfaces/ButtressRoot/T_ButtressRoot_01_BC"),
		TEXT("Megascans/Surfaces/ButtressRoot/T_ButtressRoot_01_N"),
		TEXT("Megascans/Surfaces/ButtressRoot/T_ButtressRoot_01_AoRDp") };

	/** The copy is there (its first asset is; the log says it once when not). */
	bool IsCopied();
	/** Every asset of the lists above (what the commandlet copies, with what they need). */
	UGHGAME_API TArray<const TCHAR*> All();
	/** The object path of the asset at `Path` here: <Root>/<path>.<name>. */
	UGHGAME_API FString ObjectPath(const TCHAR* Path);
	/** The static meshes of `Paths` that were copied, in their order; a missing one is logged. */
	TArray<UStaticMesh*> Meshes(TConstArrayView<const TCHAR*> Paths);
	/** The texture at `Path`; none (and a log line) when it was not copied. */
	UTexture* Texture(const TCHAR* Path);
	/** The material at `Path`; none (and a log line) when it was not copied. */
	UMaterialInterface* Material(const TCHAR* Path);
	/** The material parameter collection at `Path`; none (and a log line) when it was not copied. */
	UMaterialParameterCollection* Collection(const TCHAR* Path);
}
