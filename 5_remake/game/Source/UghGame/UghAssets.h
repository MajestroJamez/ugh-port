// The imported 3D assets the game asks for.
#pragma once

#include "CoreMinimal.h"

class UAnimSequence;
class UMaterialInterface;
class USkeletalMesh;
class UStaticMesh;
class UTexture;

/**
 * The 3D assets of Assets.json the frontend uses, by id: fetch-assets.ps1 downloads them to assets/3d, the commandlet
 * UghImportAssets (build.ps1) imports each to Content/Imported/<id> (a model: its meshes and materials; a texture
 * set: its textures and the material instance MI_<id> of UghMaterials::Pbr; a sky: its texture). Nothing of it is in
 * git, so a lookup may find nothing: the frontend then shows what it can without it (clay shapes, the drawing's
 * colours, the engine's sky), and the log says what is missing.
 */
namespace UghAssets
{
	/** Where the commandlet puts the assets: <ImportedRoot>/<id>. */
	inline const TCHAR* ImportedRoot = TEXT("/Game/Imported");
	/** The material instance of a texture set is <MaterialPrefix><id>. */
	inline const TCHAR* MaterialPrefix = TEXT("MI_");

	/**
	 * The decorations (UghDecorations, AUghScenery): tufts of grass, flowers, boulders and stones, a fern, bushes,
	 * jungle plants, a stump, palms (straight, bent), the campfire's stones and logs.
	 */
	inline const TCHAR* const Grasses[] = { TEXT("grass_medium_01"), TEXT("grass_medium_02") };
	inline const TCHAR* const Flowers[] = { TEXT("flower_gazania"), TEXT("flower_empodium"), TEXT("periwinkle_plant") };
	inline const TCHAR* const Rocks[] = { TEXT("boulder_01"), TEXT("namaqualand_boulder_02"), TEXT("rock_moss_set_01"),
		TEXT("stone_01") };
	inline const TCHAR* Fern = TEXT("fern_02");
	inline const TCHAR* const Bushes[] = { TEXT("shrub_02"), TEXT("shrub_04") };
	inline const TCHAR* const Plants[] = { TEXT("calathea_orbifolia_01"), TEXT("anthurium_botany_01") };
	inline const TCHAR* Stump = TEXT("tree_stump_01");
	inline const TCHAR* Palm = TEXT("palm");
	inline const TCHAR* Campfire = TEXT("campfire");
	/** Made by the scripts of Blender/ too: bones and skulls, totems, huts, lianas. */
	inline const TCHAR* Bones = TEXT("bones");
	inline const TCHAR* Totem = TEXT("totem");
	inline const TCHAR* Hut = TEXT("hut");
	inline const TCHAR* Vines = TEXT("vines");
	/**
	 * The texture sets of the cliff's layers (UghMaterials::CliffLayers in the same order): a warm layered rock, a
	 * grey rock face, grass, moss, red soil with stones.
	 */
	inline const TCHAR* const CliffSets[] = { TEXT("cliff_side"), TEXT("rock_face_03"), TEXT("grass004"),
		TEXT("moss002"), TEXT("red_laterite_soil_stones") };
	/** The skies of the moods (UghMood): a day with clouds, a golden evening, a dusk, a night, an overcast storm. */
	inline const TCHAR* SkyDay = TEXT("sky_kloofendal_cloudy");
	inline const TCHAR* SkyEvening = TEXT("sky_belfast_sunset");
	inline const TCHAR* SkyDusk = TEXT("sky_qwantani_dusk");
	inline const TCHAR* SkyNight = TEXT("sky_qwantani_night");
	inline const TCHAR* SkyStorm = TEXT("sky_kloofendal_overcast");
	/** Made by the scripts of Blender/: the copters' parts, the caveman (rigged, with actions), the stone passenger. */
	inline const TCHAR* Copter = TEXT("copter");
	inline const TCHAR* Caveman = TEXT("caveman");
	inline const TCHAR* StonePassenger = TEXT("stone_passenger");
	/** Made by the scripts of Blender/ too: the enemies (rigged, with actions) and the bonus items (a mesh each). */
	inline const TCHAR* Pterodactyl = TEXT("pterodactyl");
	inline const TCHAR* Triceratops = TEXT("triceratops");
	inline const TCHAR* Blower = TEXT("blower");
	inline const TCHAR* FruitTree = TEXT("fruit_tree");
	inline const TCHAR* BonusItems = TEXT("bonus_items");

	/** The content folder of asset `Id`. */
	inline FString Folder(const FString& Id) { return FString(ImportedRoot) / Id; }
	/** The static meshes imported for `Ids` (each one's in the order of their names); a missing one is logged. */
	TArray<UStaticMesh*> Meshes(TConstArrayView<const TCHAR*> Ids);
	/** The static mesh `Name` imported for `Id`; none (and a log line) when it is missing. */
	UStaticMesh* Mesh(const TCHAR* Id, const TCHAR* Name);
	/** The skeletal mesh imported for `Id`; none (and a log line) when it is missing. */
	USkeletalMesh* SkeletalMesh(const TCHAR* Id);
	/**
	 * The animation `Action` imported for `Id` (Interchange names it <mesh><action>); none (and a log line)
	 * when it is missing.
	 */
	UAnimSequence* Animation(const TCHAR* Id, const TCHAR* Action);
	/** The material instance MI_<id> of texture set `Id`; none (and a log line) when it is missing. */
	UMaterialInterface* Material(const TCHAR* Id);
	/** The texture of sky `Id`; none (and a log line) when it is missing. */
	UTexture* Texture(const TCHAR* Id);
}
