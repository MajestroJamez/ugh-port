// The imported 3D assets the game asks for.
#pragma once

#include "CoreMinimal.h"

class UMaterialInterface;
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

	/** The content folder of asset `Id`. */
	inline FString Folder(const FString& Id) { return FString(ImportedRoot) / Id; }
	/** The static meshes imported for `Ids` (each one's in the order of their names); a missing one is logged. */
	TArray<UStaticMesh*> Meshes(TConstArrayView<const TCHAR*> Ids);
	/** The material instance MI_<id> of texture set `Id`; none (and a log line) when it is missing. */
	UMaterialInterface* Material(const TCHAR* Id);
	/** The texture of sky `Id`; none (and a log line) when it is missing. */
	UTexture* Texture(const TCHAR* Id);
}
