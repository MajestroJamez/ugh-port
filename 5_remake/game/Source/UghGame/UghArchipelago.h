// The archipelago of the level selection: a sea stack a level.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UghIsles.h"
#include "UghShapes.h"
#include "UghArchipelago.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UStaticMesh;
class UTexture2D;

/**
 * The shape of a stone of the archipelago (FUghIsles) as a field (positive in the stone; pixels of the screen as
 * FUghRockField: x, y down, the depth), its foot at the world's 0 (x 0, the screen's bottom, depth 0): a sea stack of
 * slate smaller than the level's (FUghStackField) - a rounded tower narrowing upwards, its foot spreading under the
 * sea, its top a bumpy dome, in beds of flakes with sharp lips dipping as the level's (UghRockNoise::Slate), lumpy,
 * fluted -, one of FUghIsles::Variants shapes, on a grid Cell pixels apart.
 */
class FUghIsleField
{
public:
	static constexpr double Cell = 10;
	/** Nodes across (x and the depth) and up; one less divides by 4 (the coarser levels of detail). */
	static constexpr int32 Size = 85, Height = 93;
	static inline const FVector Origin =
		FVector(-(Size - 1) * Cell / 2, UghShapes::ScreenHeight - 820, -(Size - 1) * Cell / 2);

	/** The field of a shape at the grid's nodes. */
	void Build(int32 Variant);
	float At(int32 I, int32 J, int32 K) const { return Values[(K * Height + J) * Size + I]; }
	/** The field of shape `Variant` at a point (pixels): positive in the stone, about how far from its surface. */
	static double Value(const FVector& Point, int32 Variant);
	/** Which way it grows at the point (into the stone). */
	static FVector Gradient(const FVector& Point, int32 Variant);
	/** The vertex colour of its surface (as FUghStackField::Shade): how open, at the face, below its top, its patches. */
	static FColor Shade(const FVector& Point, const FVector& Outward, int32 Variant);

private:
	TArray<float> Values;
};

/** The field of FUghIsleField every `Step` nodes: a grid of UghSurfaceNets (a level of detail). */
template <int32 Step>
struct TUghIsleGrid
{
	static constexpr int32 Columns = (FUghIsleField::Size - 1) / Step + 1, Rows = (FUghIsleField::Height - 1) / Step + 1;
	const FUghIsleField& Field;
	static int32 Layers() { return Columns; }
	float At(int32 I, int32 J, int32 K) const { return Field.At(I * Step, J * Step, K * Step); }
	static FVector Node(int32 I, int32 J, int32 K)
	{
		return FUghIsleField::Origin + FVector(I, J, K) * (FUghIsleField::Cell * Step);
	}
};

/**
 * The archipelago of the level selection (FUghIsles): a stone a level of the mode where FUghIsles::Layout puts it -
 * instances of a few shapes of stone (FUghIsleField) in the cliff's material as the level's stone (AUghSeaStack),
 * each with three levels of detail (the far ones a few hundred triangles) - with a pole and a flag on its top in the
 * colour of its progress (green done, yellow open, red locked; the screen shows their numbers, UghUi). Made with the
 * stage (in the black, about half a second), hidden until the selection opens and again once a game begins.
 */
UCLASS()
class AUghArchipelago : public AActor
{
	GENERATED_BODY()

public:
	AUghArchipelago();

	/** Makes the stones' shapes (once). */
	void Make();
	/** Shows the stones of `Isles` (their places and colours) or hides them all. */
	void Show(const FUghIsles* Isles);
	bool IsShown() const { return bShown; }
	/** The water's surface, pixels from the top of the screen: the stones are wet there and under it. */
	void SetWater(double Surface);

	/** The triangles of each level of detail of a shape (a look at the cost). */
	TArray<int32> Triangles;

private:
	UPROPERTY() TArray<TObjectPtr<UStaticMesh>> Shapes;
	UPROPERTY() TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> Stones;   // one a shape
	UPROPERTY() TArray<TObjectPtr<UInstancedStaticMeshComponent>> Flags;    // one a state (EUghIsle)
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Poles;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> StoneMaterial;
	UPROPERTY() TObjectPtr<UTexture2D> StoneArt;
	bool bCliff = false;
	bool bShown = false;
	int32 ShownPlayers = 0;
	TArray<EUghIsle> ShownStates;
};
