// Materials of the imported models that work on Nanite meshes, skeletal meshes and instances.
#pragma once

#include "CoreMinimal.h"

/**
 * The models are imported as Nanite meshes (and the caveman as a skeletal mesh; the decorations are drawn instanced),
 * and a material that does not allow its use is drawn as the engine's default material in the game (grey). The
 * masters of the materials Interchange makes (glTF, FBX) are the engine's and allow none of them, and the engine's
 * content is not ours to change: so an imported material is reparented to a copy of its master in
 * <UghAssets::ImportedRoot>/_Masters that allows them all (made once, shared by all assets; a copy from before an
 * added usage gets it). Nanite draws no translucency either: a material that blends (glTF alphaMode BLEND: the grass,
 * leaves) is cut out by its alpha instead.
 */
namespace UghNaniteMaterials
{
	/** Lets every material imported to the content folder `Folder` draw its uses (cut out). */
	void Allow(const FString& Folder);
}
