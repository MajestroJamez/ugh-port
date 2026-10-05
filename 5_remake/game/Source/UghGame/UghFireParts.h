// What the campfires and the torches share: their flames, sparks, smoke and flickering lights.
#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"
#include "UghMeshes.h"

class AActor;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class UStaticMeshComponent;

/** The parts of a fire (AUghCampfire, AUghTorches): only decoration, the game does not know them. */
namespace UghFireParts
{
	/**
	 * How a fire's light flickers at a moment: its brightness (a factor around 1), its colour temperature (kelvin: redder
	 * as it dims) and where it is (units from its place: the flame licks, so the shadows and highlights on the walls
	 * dance). Noise of a few speeds, each fire (`Seed`) its own way; `Amount` how much (more in the wind).
	 */
	struct FFlicker
	{
		float Brightness;
		float Temperature;
		FVector Offset;

		static FFlicker At(double Time, int32 Seed, float Amount);
	};

	/** A light of a fire on `Owner`: a soft point light (Lumen colours the rock with it) casting soft shadows. */
	UPointLightComponent* NewLight(AActor* Owner, float Radius, float SourceRadius);
	/** Shows `Light` at `Place` flickering as `Flicker`, `Candelas` bright at its steady brightness. */
	void Flare(UPointLightComponent* Light, const FVector& Place, const FFlicker& Flicker, float Candelas);

	/** A flame's cards on `Owner` (their material plays `Flipbook`, UghFlames): instanced planes, no shadow. */
	UInstancedStaticMeshComponent* NewFlames(AActor* Owner, const TCHAR* Flipbook, float Intensity,
		TObjectPtr<UMaterialInstanceDynamic>& OutMaterial);
	/**
	 * The two crossed cards of a flame `Width` x `Height` (units) standing on `Foot`: one facing the camera, one across
	 * it (seen as the camera flies around).
	 */
	void AddFlame(TArray<FTransform>& Cards, const FVector& Foot, double Width, double Height);

	/** The sparks (UghMaterials::Sparks) and the smoke (UghMaterials::Smoke) of fires: a mesh component each. */
	UStaticMeshComponent* NewSparks(AActor* Owner, float Rise, float Spread,
		TObjectPtr<UMaterialInstanceDynamic>& OutMaterial);
	UStaticMeshComponent* NewSmoke(AActor* Owner, float Rise, float Size,
		TObjectPtr<UMaterialInstanceDynamic>& OutMaterial);
	/** `Count` sparks rising from a bed `Width` wide (units) at `Foot`, `Count` puffs from `Tip`. */
	void AddSparks(TArray<UghMeshes::FQuad>& Quads, const FVector& Foot, double Width, int32 Count,
		FRandomStream& Random);
	void AddSmoke(TArray<UghMeshes::FQuad>& Quads, const FVector& Tip, double Width, int32 Count, FRandomStream& Random);
}
