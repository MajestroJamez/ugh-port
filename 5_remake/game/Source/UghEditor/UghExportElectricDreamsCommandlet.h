// Exports scanned assets of the Electric Dreams sample for the Blender scripts.
#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "UghExportElectricDreamsCommandlet.generated.h"

class UStaticMesh;

/**
 * Exports the static meshes (and the textures, as <folder>/<surface>/<texture>.png) of UghElectricDreams::ForBlender
 * from the copy of the sample (UghCopyElectricDreams first) for the Blender scripts that make the scanned enemies: to <folder>/<mesh name>/ the mesh (its source, the full
 * scan, not Nanite's fallback) as <name>.obj in metres with Z up and the front of the scan towards -Y (Blender's axes;
 * one group a material slot, named as the slot), every texture parameter of each slot's material as
 * <slot>_<parameter>.png (as stored: a normal map DirectX's, green down) and <name>.json (the slots, their textures
 * and scalar parameters). Only what is missing or older than the copy is written. electric-dreams.ps1 runs it:
 * UnrealEditor-Cmd UghGame.uproject -run=UghExportElectricDreams -Out=<folder> (assets/3d/electricdreams).
 */
UCLASS()
class UUghExportElectricDreamsCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UUghExportElectricDreamsCommandlet();
	virtual int32 Main(const FString& Params) override;

private:
	/** Writes `Mesh` as an OBJ file `File`; false when it has no source mesh. */
	static bool WriteObj(const UStaticMesh* Mesh, const FString& File);
	/** Writes the textures of `Mesh`'s material slots and their list to `Folder`; false when one cannot be written. */
	static bool WriteMaterials(const UStaticMesh* Mesh, const FString& Folder);
};
