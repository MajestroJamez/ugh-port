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

	/** Palms (straight, bent). */
	inline const TCHAR* Palm = TEXT("palm");
	/** Boulders and stones. */
	inline const TCHAR* const Rocks[] = { TEXT("boulder_01"), TEXT("namaqualand_boulder_02"), TEXT("stone_01") };
	/**
	 * The texture sets of the cliff's layers (UghMaterials::CliffLayers in the same order): a warm layered rock, a
	 * grey rock face, grass, moss, red soil with stones.
	 */
	inline const TCHAR* const CliffSets[] = { TEXT("cliff_side"), TEXT("rock_face_03"), TEXT("grass004"),
		TEXT("moss002"), TEXT("red_laterite_soil_stones") };
	/** The skies: a calm evening with clouds, a cloudy day (darkened for a storm). */
	inline const TCHAR* SkyCalm = TEXT("sky_belfast_sunset");
	inline const TCHAR* SkyStorm = TEXT("sky_kloofendal_cloudy");
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
