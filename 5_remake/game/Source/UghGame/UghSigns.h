// The pads' boards in the diorama.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UghPadSigns.h"
#include "UghSigns.generated.h"

class FUghSprites;
class UStaticMesh;
class UStaticMeshComponent;
class UTexture2D;

/**
 * The boards with the pads' numbers of the level being played (UghPadSigns): the models of Blender/signs.py (a
 * weathered board pegged to a post, the number carved in tally marks), each standing on its ground at
 * UghPadSigns::Depth facing the camera; where they are not imported, a card with the original's board (the sprite
 * the drawing has there, else the one of its marks) at the board's front.
 */
UCLASS()
class AUghSigns : public AActor
{
	GENERATED_BODY()

public:
	AUghSigns();

	/** Shows `Signs` (none: nothing), the cards with the sprites of `Sprites`. */
	void Show(const TArray<FUghPadSign>& Signs, const FUghSprites& Sprites);

protected:
	virtual void BeginPlay() override;

private:
	/** A new component of the actor: board `Model` at `Location` (static: its shadow is cached). */
	UStaticMeshComponent* AddModel(UStaticMesh* Model, const FVector& Location);

	UPROPERTY() TArray<TObjectPtr<UStaticMesh>> Models;   // by marks; empty when one is not imported
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Boards;   // of the level shown
	UPROPERTY() TMap<int32, TObjectPtr<UTexture2D>> SpriteTextures;   // of the cards
};
