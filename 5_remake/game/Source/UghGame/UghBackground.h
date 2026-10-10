// The level around the figures.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UghBackground.generated.h"

class FUghRockMesh;
class FUghTurf;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMeshComponent;
class UTexture2D;
struct FMeshDescription;

/**
 * The diorama of the level being played: the cliff with its cave (FUghRockMesh) in the cliff's material (the imported
 * rock, grass, moss and soil steered by the original's drawing; without them the drawing's colours, the boards with
 * the pads' numbers among them), the cliff going on beyond the screen (its shadow beyond its mesh); the rock is wet at
 * the water (AUghWater) and under it; its turf (FUghTurf: the paths trodden to the cave entrances, the grass hanging
 * over the edges in M_UghTurf). The pads' boards are AUghSigns'.
 */
UCLASS()
class AUghBackground : public AActor
{
	GENERATED_BODY()

public:
	AUghBackground();

	/** Shows the rock of a level (empty: none), `Art` its drawing (FUghLevelArt), its `Turf` (none: no turf). */
	void Build(const FUghRockMesh& Mesh, const TArray<FColor>& Art, const FUghTurf* Turf = nullptr);
	/**
	 * The same in steps (a flight between two levels builds it a step a frame while the old rock is still seen): the
	 * new drawing (`Turf` its paths; the old rock keeps its own material), each piece of the rock
	 * (FUghRockMesh::Describe) and the turf's grass made hidden, then at once the old rock gone and the new one shown
	 * (FinishBuild).
	 */
	void BeginBuild(const TArray<FColor>& Art, const FUghTurf* Turf);
	void AddRock(const FMeshDescription& Piece);
	void AddBlades(const FUghTurf& Turf);
	void FinishBuild();
	/** The rock shows the drawing's colours (the cliff's surfaces are missing), the pads' boards among them. */
	bool ShowsArt() const { return !bCliff; }
	/** The water's surface, pixels from the top of the screen: the rock is wet there and under it. */
	void SetWater(double Surface);

	/**
	 * The cliff's material (on `Outer`; the stone around the level's too, AUghSeaStack), each layer the scanned surface
	 * of UghElectricDreams where it has one and it was copied, else its imported texture set; none when that is
	 * missing too.
	 */
	static UMaterialInstanceDynamic* MakeCliffMaterial(UObject* Outer);
	/**
	 * The unseen cliff around the rock's grid (above it and at its sides) that only shades the cave, as boxes of
	 * UghShapes (the stone around the level holds it, FUghStackField).
	 */
	static TArray<FTransform> ShroudBoxes();

protected:
	virtual void BeginPlay() override;

private:
	/** Gives `Cliff` layer `Layer` of the scanned surfaces, of the imported texture sets; false when it is missing. */
	static bool SetScannedLayer(UMaterialInstanceDynamic* Cliff, int32 Layer);
	static bool SetImportedLayer(UMaterialInstanceDynamic* Cliff, int32 Layer);
	/** The unseen cliff around the rock's grid that only shades it (ShroudThickness). */
	void AddShroud();

	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Rocks;   // the pieces of the level shown, none before one
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> NewRocks;   // of the level being built (hidden)
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> RockMaterials[2];   // the shown rock's, the one being built's
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Blades;   // the turf's grass over the edges, none without it
	UPROPERTY() TObjectPtr<UStaticMeshComponent> NewBlades;   // of the level being built (hidden)
	UPROPERTY() TObjectPtr<UMaterialInterface> BladesMaterial;
	UPROPERTY() TObjectPtr<UTexture2D> RockArts[2];
	int32 ShownMaterial = 0;   // of RockMaterials: the shown rock's (the other one the next level's)
	bool bCliff = false;   // RockMaterial is the cliff's (else the drawing's colours)
};
