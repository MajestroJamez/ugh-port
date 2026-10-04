// The scanned assets of Epic's Electric Dreams sample the game uses.
#pragma once

#include "CoreMinimal.h"

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
	 * The scanned rock dressing the cave (UghRockDressing): cliffs of sandstone on its back wall (their scans face +Y,
	 * the camera, and are open at the back), roots hanging from its ceilings (lying along +Y from the trunk's end).
	 */
	inline const TCHAR* const Roots[] = { TEXT("Custom/RootsTest/SM_Roots_01"), TEXT("Custom/RootsTest/SM_Roots_02"),
		TEXT("Custom/RootsTest/SM_Roots_03"), TEXT("Custom/RootsTest/SM_Roots_04"),
		TEXT("Custom/RootsTest/SM_Roots_05") };
	inline const TCHAR* const Cliffs[] = { TEXT("Megascans/3D_Assets/HugeSandstoneCliff/SM_HugeSandstoneCliff_01"),
		TEXT("Megascans/3D_Assets/HugeSandstoneCliff/SM_HugeSandstoneCliff_03"),
		TEXT("Megascans/3D_Assets/MassiveSandstoneCliff/SM_MassiveSandstoneCliff_02") };

	/**
	 * A layer of the cliff's material (UghMaterials::CliffLayers, the same order): its tiling textures (base colour,
	 * normal, a packed map with roughness in green), which channels of the packed map make its relief (HeightMask)
	 * and how many metres one texture covers.
	 */
	struct FCliffLayer
	{
		const TCHAR* BaseColor;
		const TCHAR* Normal;
		const TCHAR* Packed;
		FLinearColor HeightMask;
		float Size;
	};
	/**
	 * The scanned sandstone of a beach cliff (layered, cracked; the cliff's material hides its tiles), a grey rock of
	 * the sample's tileable sets (patches in the sandstone, the cave's walls, the fine relief of all the rock), mossy
	 * grass, moss, the jungle's soil.
	 */
	inline const FCliffLayer CliffLayers[] = {
		{ TEXT("Megascans/Surfaces/BeachCliff/T_BeachCliff_01_BC"),
		TEXT("Megascans/Surfaces/BeachCliff/T_BeachCliff_01_N"),
			TEXT("Megascans/Surfaces/BeachCliff/T_BeachCliff_01_AoRDp"), FLinearColor(0, 0, 1, 0), 5.f },
		{ TEXT("SmartAssets/TileableTextures/T_Rock_03_Albedo"), TEXT("SmartAssets/TileableTextures/T_Rock_03_Normal"),
			TEXT("SmartAssets/TileableTextures/T_Rock_03_Material"), FLinearColor(1, 0, 0, 0), 2.5f },
		{ TEXT("Megascans/Surfaces/MossyGrass/T_MossyGrass_01_BC"),
		TEXT("Megascans/Surfaces/MossyGrass/T_MossyGrass_01_N"),
			TEXT("Megascans/Surfaces/MossyGrass/T_MossyGrass_01_AoRDp"), FLinearColor(0, 0, 1, 0), 1.5f },
		{ TEXT("Megascans/Surfaces/NordicMoss/T_NordicMoss_01_BC"),
		TEXT("Megascans/Surfaces/NordicMoss/T_NordicMoss_01_N"),
			TEXT("Megascans/Surfaces/NordicMoss/T_NordicMoss_01_AoRDp"), FLinearColor(0, 0, 1, 0), 1.5f },
		{ TEXT("Megascans/Surfaces/JungleGround/T_JungleGround_01_BC"),
		TEXT("Megascans/Surfaces/JungleGround/T_JungleGround_01_N"),
			TEXT("Megascans/Surfaces/JungleGround/T_JungleGround_01_AoRDp"), FLinearColor(0, 0, 1, 0), 2.f } };

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
}
