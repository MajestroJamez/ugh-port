// The manifest of the 3D assets (Assets.json).
#pragma once

#include "CoreMinimal.h"

/** An asset of Assets.json as the import needs it. */
struct FUghManifestAsset
{
	FString Id;
	/** texture (a PBR set), model, hdri or generated (made by a Blender script, imported like a model). */
	FString Kind;
	/** Where fetch-assets.ps1 put it: <folder of the manifest>/<path>, absolute. */
	FString Folder;
	/** A texture set's maps: role (color, normal, arm, roughness, ao, height) -> file in Folder. */
	TMap<FString, FString> Maps;
	/** A model's or sky's files in Folder to import. */
	TArray<FString> Import;

	/** The files the import reads (Maps and Import), in Folder. */
	TArray<FString> Files() const;
};

/**
 * Reads 5_remake/game/Assets.json (fetch-assets.ps1 downloads what it lists, the commandlet UghImportAssets imports
 * it): the assets with their folders under the repository's `folder` (assets/3d); not the fonts (the game reads them
 * as they are) nor the local ones (only inputs of the Blender scripts).
 */
namespace UghAssetManifest
{
	/** The assets of the manifest at `Path` (its folder is relative to `RepositoryDir`); false and why when it cannot. */
	bool Read(const FString& Path, const FString& RepositoryDir, TArray<FUghManifestAsset>& OutAssets, FString& OutError);
}
