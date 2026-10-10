// The copters.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ugh_logic.h"
#include "UghCaveman.h"
#include "UghCopterModel.h"
#include "UghDunk.h"
#include "UghRotorSpin.h"
#include "UghCopters.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMeshComponent;

/**
 * A copter's model: its parts (the drive's chain links instances of one mesh), its pilot, its passenger in the cabin,
 * the stone passenger in its sling (the rope from the floor's hook to the sling apart: stretched when the stone rests
 * on the ground beside the copter, UghSling).
 */
USTRUCT()
struct FUghCopterParts
{
	GENERATED_BODY()

	UPROPERTY() TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Rotor;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Shaft;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Crank;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Drive;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Chain;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Sling;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Stone;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> SlingRope;
	UPROPERTY() TObjectPtr<USceneComponent> Pilot;
	UPROPERTY() TObjectPtr<USceneComponent> Rider;   // of the cargo look RiderLook; made when first needed
	FUghRotorSpin Spin;
	int32 RiderLook = 0;
	double Sway = 0;       // of the sling, radians
	FVector2D Push = FVector2D::ZeroVector;   // how far the stone on the ground is pushed aside, cm (UghSling)
};

/**
 * The copters of the play in the slab of the play, between the views of two steps (UghBetween): the pedal copters of
 * Blender/copter.py in each player's colours, the rotor turning and the pilot sitting and pedalling as fast as the
 * rotor's sprites change (FUghRotorSpin), his crank driving the rotor by the chain (FUghCopterChain), the layshaft
 * and the crown wheel, a passenger sitting behind him (a person of his look, FUghCaveman) or the stone passenger
 * hanging in the sling below (the logic never puts it in the cabin), swaying as the copter moves, never in the ground:
 * low over it the stone rests on it, pushed aside (UghSling). Without the
 * imported models: clay, a box with a rotor that gets shorter and longer as its sprites change; its riders are clay
 * passengers of AUghFigures.
 */
UCLASS()
class AUghCopters : public AActor
{
	GENERATED_BODY()

public:
	AUghCopters();

	/** Spare riders of each passenger's look made ahead (two copters may carry the same look). */
	static constexpr int32 RiderSpares = 2;

	/**
	 * Shows the copters between `Previous` and `Current` (Alpha 0 .. 1), `Seconds` after the last frame; none outside
	 * the play. Without the models, the riders go to `OutClayRiders` (boxes of the clay passengers). A copter afloat
	 * bobs and rocks as `Dunks` say (FUghDunks). The stone in a sling stays out of the ground of `Logic` (its collision
	 * mask; none: it always hangs freely).
	 */
	void Show(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha, double Seconds,
		TArray<FTransform>& OutClayRiders, const FUghDunks* Dunks = nullptr, const ugh_logic* Logic = nullptr);
	/** Makes spare riders ahead (while a level is built in the black): none is made in the play (FUghCaveman). */
	void Stock();

protected:
	virtual void BeginPlay() override;

private:
	bool LoadModels();
	void ShowModel(FUghCopterParts& Parts, const ugh_logic_copter& From, const ugh_logic_copter& To, double Alpha,
		double Seconds, const FUghCopterBob& Bob, const ugh_logic* Logic);
	void ShowCargo(FUghCopterParts& Parts, const ugh_logic_copter& Copter, double Seconds, double Velocity,
		const ugh_logic* Logic, const FVector2D& Under);
	void HideModel(FUghCopterParts& Parts);
	void ShowClay(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha,
		TArray<FTransform>& OutRiders, const FUghDunks* Dunks, const ugh_logic* Logic);

	UPROPERTY() FUghCaveman Caveman;
	FUghCopterChain Chain;
	TArray<FTransform> ChainLinks;   // a frame's, kept
	UPROPERTY() TArray<FUghCopterParts> Models;   // by player; none without the models
	UPROPERTY() TArray<TObjectPtr<UInstancedStaticMeshComponent>> ClayBodies;   // by player
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> ClayRotors;
};
