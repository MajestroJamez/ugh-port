// Imports the 3D assets of Assets.json.
#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "UghImportAssetsCommandlet.generated.h"

struct FUghManifestAsset;

/**
 * Imports the assets of Assets.json that fetch-assets.ps1 downloaded (assets/3d) to Content/Imported/<id> through
 * Interchange: a model's glTF or FBX files with their meshes (Nanite), materials and textures, a sky's HDR image, a
 * texture set's maps with the material instance MI_<id> of UghMaterials::Pbr. build.ps1 runs it after the materials:
 * UnrealEditor-Cmd UghGame.uproject -run=UghImportAssets [-Force]. Idempotent: an asset whose files are as at its
 * last import (Content/Imported/<id>/Import.stamp) is left alone unless -Force (one imported again is deleted
 * first); one not downloaded is skipped with a log line (the game shows its clay shapes instead). Fails only when an
 * import or a save fails.
 */
UCLASS()
class UUghImportAssetsCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UUghImportAssetsCommandlet();
	virtual int32 Main(const FString& Params) override;

private:
	/** Imports the asset to UghAssets::Folder(Id); false when something could not be imported. */
	static bool Import(const FUghManifestAsset& Asset);
	static bool ImportTextureSet(const FUghManifestAsset& Asset);
	/** The objects imported from `File` to the content folder `Folder` (none when it failed). */
	static TArray<UObject*> ImportFile(const FString& File, const FString& Folder);
	/**
	 * Saves the changed packages of the content folder `Folder` (its materials allowed on Nanite meshes first); false when
	 * one could not be saved.
	 */
	static bool SaveFolder(const FString& Folder);
	/** What the import of the asset depends on: its files (names, sizes, times) and the version of this import. */
	static FString Fingerprint(const FUghManifestAsset& Asset);
};
