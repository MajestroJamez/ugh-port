// The level around the figures.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UghLevelArt.h"
#include "UghShapes.h"
#include "UghBackground.generated.h"

class FUghRockMesh;
class FUghSprites;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;
class UTexture2D;

/**
 * The diorama of the level being played: the cliff with its cave (FUghRockMesh) in the cliff's material (the imported
 * rock, grass, moss and soil steered by the original's drawing; without them the drawing's colours), the boards with
 * the pads' numbers as the original draws them, the cliff going on beyond the screen (its shadow beyond its mesh); the
 * rock is wet at the water (AUghWater) and under it.
 */
UCLASS()
class AUghBackground : public AActor
{
	GENERATED_BODY()

public:
	/** The boards with the pads' numbers stand on the cave's floor just behind the slab of the play, units. */
	static constexpr double SignDepth = UghShapes::PlaneThickness / 2 + 3;

	AUghBackground();

	/** Shows the rock of a level (empty: none), `Art` its drawing (FUghLevelArt), its `Signs` (sprites of `Sprites`). */
	void Build(const FUghRockMesh& Mesh, const TArray<FColor>& Art, const TArray<FUghArtTile>& Signs,
		const FUghSprites& Sprites);
	/** The water's surface, pixels from the top of the screen: the rock is wet there and under it. */
	void SetWater(double Surface);

protected:
	virtual void BeginPlay() override;

private:
	/**
	 * The cliff's material with the layers of the scanned surfaces (UghElectricDreams), else of the imported texture
	 * sets; none when one of them is missing.
	 */
	UMaterialInstanceDynamic* MakeCliffMaterial();
	/** Gives `Cliff` the layers of the scanned surfaces, of the imported texture sets; false when one is missing. */
	static bool SetScannedLayers(UMaterialInstanceDynamic* Cliff);
	static bool SetImportedLayers(UMaterialInstanceDynamic* Cliff);
	void ShowSigns(const TArray<FUghArtTile>& Signs, const FUghSprites& Sprites);
	/** The unseen cliff around the rock's grid that only shades it (ShroudThickness). */
	void AddShroud();

	UPROPERTY() TObjectPtr<UStaticMeshComponent> Rock;   // of the level shown, none before one
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> RockMaterial;
	UPROPERTY() TObjectPtr<UTexture2D> RockArt;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> SignCards;   // made as many as needed, hidden when unused
	UPROPERTY() TMap<int32, TObjectPtr<UTexture2D>> SignTextures;
	bool bCliff = false;   // RockMaterial is the cliff's (else the drawing's colours)
};
