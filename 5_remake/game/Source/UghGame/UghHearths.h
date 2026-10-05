// The hearths of the campfires: their stones, logs and coals.
#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"

class AActor;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;

/**
 * The hearths of a level's campfires (AUghCampfire): a ring of the scanned stones of the Electric Dreams sample
 * (UghElectricDreams::FireStones) around its scanned branches burnt to charcoal (UghElectricDreams::FireLogs) leaning
 * together over a bed of glowing coals (UghMaterials::Embers: their embers glow in the cracks, the logs' more on their
 * undersides); without the sample Kenney's campfire (UghAssets::Campfire) in clay of darker colours, without it two
 * clay logs. One instanced component a mesh, on the owner.
 */
class FUghHearths
{
public:
	/** Makes the components on `Owner`. */
	void Make(AActor* Owner);
	/** A hearth `Width` wide (units) on `Foot`, turned `Yaw` degrees, its own way (`Seed`); how high its logs reach. */
	double Add(const FVector& Foot, double Width, double Yaw, int32 Seed);
	/** Shows the hearths added since the last Show, and only them. */
	void Show();

private:
	/** The instances of one mesh. */
	struct FPart
	{
		UInstancedStaticMeshComponent* Component = nullptr;
		FBox Bounds;
		TArray<FTransform> Boxes;
	};

	/** A new part of `Mesh` (its own materials, or `Material` in every slot). */
	int32 NewPart(AActor* Owner, UStaticMesh* Mesh, UMaterialInterface* Material);
	/** An instance of part `Part` as long as `Size` along its longest side, its middle at `At`, turned `Rotation`. */
	void Put(int32 Part, const FVector& At, const FQuat& Rotation, double Size);
	double AddScanned(const FVector& Foot, double Width, double Yaw, FRandomStream& Random);
	double AddKenney(const FVector& Foot, double Width, double Yaw);

	TArray<FPart> Parts;
	TArray<int32> Stones, Logs, Coals, Kenney;
	int32 ClayLogs = INDEX_NONE;
};
