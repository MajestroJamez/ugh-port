// The soft edges of the level: the plants at the stone's edges a copter runs into.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ugh_logic.h"
#include "UghFringe.generated.h"

class FUghEffectPlayer;
class UInstancedStaticMeshComponent;
class UStaticMesh;

/**
 * A plant of the fringe (UghFringe::Plan): where it grows from (its pivot, which it bends about), which way it grows
 * and how far.
 */
struct FUghFringePlant
{
	/**
	 * A liana hanging (a piece of a curtain hangs from the tip of the one above it), a fern or a root hanging from the
	 * overhang, a bush on the stone's face, a creeper (wall ivy: on the face, a leafy piece of a curtain, the
	 * overhang's veil).
	 */
	enum class EKind : uint8 { Strand, Fern, Root, Bush, Creeper };

	EKind Kind = EKind::Strand;
	/** Pixels of the screen: its pivot; units behind the plane of the play (negative towards the camera). */
	double X = 0, Y = 0, Depth = 0;
	/** Which way it grows from its pivot (the world: X right, Y towards the camera, Z up). */
	FVector Grows = -FVector::UpVector;
	/** Pixels: how far it grows (from its pivot to its tip), how wide it is. */
	double Length = 0, Width = 0;
	/** Degrees it is turned about the way it grows; which of its kind's meshes (modulo their count). */
	double Twist = 0;
	int32 Variant = 0;
	/** The piece of its curtain it hangs from (its tip this one's pivot), INDEX_NONE for none. */
	int32 Above = INDEX_NONE;
	/** Where it is: beside the level on the left or the right, or above it. */
	enum class ESide : uint8 { Left, Right, Top } Side = ESide::Top;

	/** It bends away from a copter (what hangs from the overhang, a curtain's piece; not what grows on the face). */
	bool bSways = false;

	bool Bends() const { return bSways; }
	/** Its tip at rest (pixels of the screen x, y and depth units). */
	FVector Tip() const;
	/** Its box across the screen at rest (pixels): around the line from its pivot to its tip, its width wide. */
	FBox2D Box() const;

	bool operator==(const FUghFringePlant& Other) const = default;
};

/**
 * The soft edges of the level (Jan, step 24d). In the original a copter stops at the screen's left and right edge
 * (CopterPhysics: its top left corner at LeftEdge, RightEdge pixels) and can fly off its top up to TopEdge pixels,
 * without a crash or a lost life; now the stone around the level is seen (AUghStage::Play), so the copter must not seem
 * to fly into rock there. Beside the level curtains of lianas hang from the top of the stone's hollow down to the sea,
 * bushes and creepers on the stone's face by them; above it a dense overhang: bushes and creepers on the face over the
 * hollow, a veil of ivy, lianas, ferns and roots hanging from it in front of the copter's top limit, which it
 * disappears into. Every plant off the screen of the play (none over it), the same for every level. Only decoration:
 * the logic does not know them.
 */
namespace UghFringe
{
	/**
	 * Where the logic stops a copter (its top left corner, pixels; CopterPhysics' LEFT_EDGE, RIGHT_EDGE, TOP_EDGE: -608/32;
	 * the golden replays of the original reach the top one at up to 2.4 pixels a frame, its speed only zeroed, no crash).
	 */
	constexpr double LeftEdge = -16, RightEdge = 304, TopEdge = -19;
	/**
	 * What of a copter pushes the plants, from its top left corner (pixels): its rotor's sweep across (its hub over the
	 * middle of the body, 13 px around), from its top to its skids.
	 */
	constexpr double ReachLeft = 3, ReachRight = 29, ReachTop = -1, ReachBottom = 20;
	/** A plant bends this many pixels before the copter's reach touches it (its leaves spread wider than its line). */
	constexpr double Near = 6;
	/**
	 * Degrees a plant bends at most, held by a copter: a curtain's piece outwards and towards the camera (away from it,
	 * behind the copter), the overhang a little apart and towards the camera (it drapes over the copter, which stays
	 * hidden in it). A bump shakes them further (AUghFringe).
	 */
	constexpr double SideAside = 14, SideForward = 6, TopAside = 8, TopForward = 12;

	/** The plants, the same every time (in an order where a piece of a curtain follows the one it hangs from). */
	TArray<FUghFringePlant> Plan();
	/** What of a copter with its top left corner at `At` (pixels) pushes the plants. */
	FBox2D Reach(const FVector2D& At);
	/**
	 * How far `Plant` bends at rest against a copter's `Reach` (degrees: x aside - positive its tip to the screen's
	 * right -, y towards the camera); none when it does not bend or the copter is not near.
	 */
	FVector2D Bend(const FUghFringePlant& Plant, const FBox2D& Reach);
	/** How near a copter's `Reach` is to `Plant`: 0 far, 1 touching it or in it. */
	double Touch(const FUghFringePlant& Plant, const FBox2D& Reach);
}

/**
 * The soft edges (UghFringe): the scanned plants of the Electric Dreams sample (lianas, ferns, roots, bushes, creepers;
 * a liana of Blender's vines.py without them, the rest then absent), instanced. A copter near them bends them away -
 * each plant a damped spring about its pivot (a curtain's pieces hanging from each other), turned there instance by
 * instance (no material of the sample changed) - a bump into the edge or the overhang shakes them, a few leaves fall
 * (the burst Rustle) and they swing back; the plants sway in the wind as the sample's do.
 */
UCLASS()
class AUghFringe : public AActor
{
	GENERATED_BODY()

public:
	AUghFringe();

	/**
	 * The frame `Seconds` after the last one between the views (Alpha): the copters push the plants, a bump orders
	 * leaves from `Effects`.
	 */
	void Show(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha, double Seconds,
		FUghEffectPlayer& Effects);

protected:
	virtual void BeginPlay() override;

private:
	/** A plant shown: its instance, where it is at rest, how it bends now (degrees, and degrees a second). */
	struct FShown
	{
		int32 Mesh = INDEX_NONE;   // in Components
		int32 Instance = INDEX_NONE;
		FTransform Rest;
		FVector Pivot = FVector::ZeroVector;   // the world
		FVector2D Angle = FVector2D::ZeroVector, Spin = FVector2D::ZeroVector;
		FTransform Bent;   // how its bend moves the world (with the pieces above it)
		bool bMoved = false;
	};
	/** A copter as the fringe saw it last: where (its top left corner, pixels), how fast (pixels a second). */
	struct FCopterSeen
	{
		TOptional<FVector2D> At;
		FVector2D Velocity = FVector2D::ZeroVector;
		bool bAtEdge[3] = { false, false, false };   // left, right, top
		double Quiet = 0;   // seconds since its last leaves
	};

	TArray<FUghFringePlant> Plants;
	TArray<FShown> Shown;
	UPROPERTY() TArray<TObjectPtr<UInstancedStaticMeshComponent>> Components;
	FCopterSeen Seen[2];
	bool bStill = true;   // nothing bends
};
