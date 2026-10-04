// Materials of the imported models that work on Nanite meshes and skeletal meshes.
#pragma once

#include "CoreMinimal.h"

/**
 * The models are imported as Nanite meshes (and the caveman as a skeletal mesh), and a material that does not allow
 * its mesh is drawn as the engine's default material in the game (grey). The masters of the materials Interchange
 * makes for glTF are the engine's and allow neither, and the engine's content is not ours to change: so an imported
 * material is reparented to a copy of its master in <UghAssets::ImportedRoot>/_Masters that allows both (made once,
 * shared by all assets; a copy from before an added usage gets it).
 */
namespace UghNaniteMaterials
{
	/** Lets every material imported to the content folder `Folder` draw Nanite meshes and skeletal meshes. */
	void Allow(const FString& Folder);
}
