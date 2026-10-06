// Shows the bursts of the events of the logic.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UghEffectPlayer.h"
#include "UghEffects.generated.h"

class UMaterialInstanceDynamic;
class UPointLightComponent;
class UStaticMesh;
class UStaticMeshComponent;

/** A burst being shown: a mesh component and its material per part, how long it has been going. */
USTRUCT()
struct FUghBurstShown
{
	GENERATED_BODY()

	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Parts;
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> Materials;
	EUghBurst Burst = EUghBurst::Dust;
	double Elapsed = 0;
	bool bShown = false;
	bool bHeld = false;          // held at its moment (a shot)
	int32 Owner = INDEX_NONE;    // the entity a loop follows
};

/**
 * The bursts of the events (FUghEffectPlayer: which and where): each part a mesh of tiny quads whose material
 * (UghMaterials::Burst, Bits, Glint) moves and draws its particles on the GPU - nothing is done a frame but its clock;
 * a few of each burst at once (the oldest gives way), made the first time one is wanted, hidden when it is over. A
 * loop follows its entity until its event stops it; a burst with a flash lights its place for a moment (a light of
 * a few); a paid fare or a knocked out enemy shows its points rising (the HUD draws them). Only decoration: the logic
 * does not know them.
 */
UCLASS()
class AUghEffects : public AActor
{
	GENERATED_BODY()

public:
	/** A score shown rising from where it was earned (the HUD draws it). */
	struct FPopup
	{
		FVector Where;
		int32 Points;
		double Age;
		bool bHeld = false;   // for a shot (Hold)
	};
	/** The bursts of a kind shown at once; how long a score shows; the lights of the flashes. */
	static constexpr int32 MaxShown = 3, MaxFlashes = 2;
	static constexpr double PopupSeconds = 1.2;
	/** The score a held burst shows rising (Hold). */
	static constexpr int32 ShotPoints = 250;

	AUghEffects();

	FUghEffectPlayer& GetPlayer() { return Player; }

	/**
	 * Starts the bursts the player ordered and shows them `Seconds` later between the two views (Alpha): the loops
	 * follow their entities, nothing under the water's surface.
	 */
	void Show(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha, double Seconds);
	/**
	 * Shows `Burst` at `Place` (pixels) of `View` held `Age` seconds into it (a shot of it), until Clear; a burst of
	 * an event earning points with ShotPoints rising.
	 */
	void Hold(EUghBurst Burst, const FVector2D& Place, const ugh_logic_view& View, double Age);
	bool IsHolding() const;
	/** Every burst, flash and score gone (a game ends). */
	void Clear();
	/** Makes one of each of `Bursts` ahead, hidden (while a level is built in the black: none made in the play). */
	void Stock(TConstArrayView<EUghBurst> Bursts);

	TConstArrayView<FPopup> GetPopups() const { return Popups; }

private:
	struct FFlashing
	{
		UghBursts::FFlash Flash;
		FVector Where;
		double Elapsed = 0;
		bool bHeld = false;
	};

	/** Starts (or stops) the burst of `Order`; the one shown, none for a stop or a loop already going. */
	FUghBurstShown* Start(const FUghEffectOrder& Order);
	/** A burst of `Burst` to show: one not shown, else the oldest; its parts made the first time. */
	FUghBurstShown& Take(EUghBurst Burst);
	UStaticMesh* MeshOf(EUghBurst Burst, int32 Part);
	void Hide(FUghBurstShown& Shown);
	/** The lights of the flashes as they die away. */
	void Flare(double Seconds);

	FUghEffectPlayer Player;
	UPROPERTY() TArray<FUghBurstShown> Shown;
	UPROPERTY() TMap<int32, TObjectPtr<UStaticMesh>> Meshes;   // by burst * 16 + part
	UPROPERTY() TArray<TObjectPtr<UPointLightComponent>> Lights;
	TArray<FFlashing> Flashes;
	TArray<FPopup> Popups;
	float WaterZ = -1e6f;   // the world's z of the water's surface
};
