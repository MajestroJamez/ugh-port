// The blower's snort of dust.
#pragma once

#include "CoreMinimal.h"
#include "UghSnort.generated.h"

class AActor;
class UMaterialInstanceDynamic;
class USkeletalMeshComponent;
class UStaticMeshComponent;

/**
 * The dust and breath a blower snorts out of its nostrils while it blows out: a mesh of quads at its bone Nostrils
 * that UghMaterials::Puff moves along the nostrils' way from its bone Head, along the ground its head rests on (the
 * T-rex of Blender/blower_trex.py; the older blower has no nostrils and does not snort). The blower breathes in for
 * the first BreathIn of its blow's loop (the original's first 3 of 10 sprites: the copters are drawn in), then
 * snorts.
 */
USTRUCT()
struct FUghSnort
{
	GENERATED_BODY()

	static constexpr double BreathIn = 0.3;
	static constexpr int32 Puffs = 28;
	inline static const FName Nostrils = TEXT("nostrils"), Head = TEXT("head");

	/** Shows the snort of the blower `Model` (on `Owner`) blowing at `Phase` (0 .. 1) of its loop, or not blowing. */
	void Show(AActor* Owner, USkeletalMeshComponent* Model, bool bBlowing, double Phase);
	void Hide();

private:
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Puff;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Material;
};
