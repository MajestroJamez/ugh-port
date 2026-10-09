// The ghost copters.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UghRotorSpin.h"
#include "UghGhosts.generated.h"

class FUghGhost;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

/**
 * The copters of the ghost of the best run (FUghGhost) between two of its steps: only each copter's body and its turning
 * rotor (no pilot, no passengers, no chain), see-through and unlit (UghMaterials::Ghost: a glowing rim, pale blue), no
 * shadow, nothing in Lumen or the ray traced scene - cheap at every quality, the same at Low. Without the copter's
 * models none is shown.
 */
UCLASS()
class AUghGhosts : public AActor
{
	GENERATED_BODY()

public:
	AUghGhosts();

	/** Shows `Ghost`'s copters (Alpha between its views, `Seconds` after the last frame); none when it is null or not shown. */
	void Show(const FUghGhost* Ghost, double Alpha, double Seconds);
	/** Where copter `Player`'s body was shown last (the world), none when hidden: for the tests. */
	TOptional<FVector> GetShown(int32 Player) const;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Bodies;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Rotors;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Material;
	TArray<FUghRotorSpin> Spins;
};
