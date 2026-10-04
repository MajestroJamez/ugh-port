// The imported 3D assets the game asks for.
#pragma once

#include "CoreMinimal.h"

class UStaticMesh;

/**
 * The 3D assets of Assets.json the frontend uses, by id: fetch-assets.ps1 downloads them to assets/3d, the commandlet
 * UghImportAssets (build.ps1) imports each to Content/Imported/<id> (a model: its meshes and materials; a texture
 * set: its textures and the material instance MI_<id> of UghMaterials::Pbr). Nothing of it is in git, so a lookup may
 * find nothing: the frontend then shows its clay shapes, and the log says what is missing.
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

	/** The content folder of asset `Id`. */
	inline FString Folder(const FString& Id) { return FString(ImportedRoot) / Id; }
	/** The static meshes imported for `Ids` (each one's in the order of their names); a missing one is logged. */
	TArray<UStaticMesh*> Meshes(TConstArrayView<const TCHAR*> Ids);
}
