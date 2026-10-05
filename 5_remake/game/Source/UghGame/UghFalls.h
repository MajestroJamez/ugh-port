// The springs of a level: their water, their footbridges.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UghStreams.h"
#include "UghFalls.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

/** What the footbridges are made of. */
namespace UghFalls
{
	/** Units: the logs' tops are this far under the ledge's surface (the figures' feet on them, none in them). */
	constexpr double UnderSurface = 0.3;

	/**
	 * The logs of the footbridge of `Stream` (each the engine's cylinder, UghShapes::EShape::Cylinder, fitted into its
	 * transform): side by side along it from its front to its back, their tops under the ledge's surface, on two
	 * beams; a handrail at its back, behind the figures walking over it (no taller than ground cover); the same every
	 * time.
	 */
	TArray<FTransform> Bridge(const FUghStream& Stream);
}

/**
 * The streams of a level (UghStreams): out of the hole in the back wall (cut into the rock with its channel,
 * FUghRockField::CarveChannels) the stream running in its channel across the ledge and under its footbridge of logs,
 * its waterfall down the rock's face into the sea (one mesh of the flowing water's material, UghMaterials::Flow), the
 * spray and mist rising where it pours in (UghMaterials::Mist; the sea foams and churns there itself, AUghWater), all
 * within the stream's area (FUghStream::Area). Only shown: nothing of it decides anything. The rising sea swallows
 * the waterfall; once it reaches a ledge, that spring's water is the sea's.
 */
UCLASS()
class AUghFalls : public AActor
{
	GENERATED_BODY()

public:
	AUghFalls();

	/** The streams of the level being played (none: nothing). */
	void Show(const TArray<FUghStream>& Streams);
	/** The sea's surface (pixels from the top of the screen). */
	void SetWater(double Surface);
	/** Where the waterfalls pour into the sea at the surface `Surface` (UghStreams::Feet): those still falling. */
	TArray<FVector4> Feet(double Surface) const;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Water;   // a new mesh a level
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Mist;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Logs;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> FlowMaterial;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> MistMaterial;
	TArray<FUghStream> Streams;
};
