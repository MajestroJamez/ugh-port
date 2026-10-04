// Materials of the imported models that work on Nanite meshes.
#pragma once

#include "CoreMinimal.h"

/**
 * The models are imported as Nanite meshes, and a material that does not allow them is drawn as the engine's default
 * material in the game (grey). The masters of the materials Interchange makes for glTF are the engine's and do not
 * allow them, and the engine's content is not ours to change: so an imported material is reparented to a copy of its
 * master in <UghAssets::ImportedRoot>/_Masters that does (made once, shared by all assets).
 */
namespace UghNaniteMaterials
{
	/** Lets every material imported to the content folder `Folder` draw Nanite meshes. */
	void Allow(const FString& Folder);
}
