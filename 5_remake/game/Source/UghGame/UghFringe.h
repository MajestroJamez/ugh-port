// The soft edges of the level: the plants at the stone's edges a copter runs into.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math/RandomStream.h"
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
	 * Pixels a plant is pushed at most where a copter holds it (FUghFringeSway: the rest of it follows as a hanging
	 * chain): a curtain outwards and towards the camera (away from it, behind the copter), the overhang a little apart
	 * and towards the camera (it drapes over the copter, which stays hidden in it). A bump shakes them further.
	 */
	constexpr double SideAside = 7, SideForward = 3, TopAside = 2.2, TopForward = 3.5;

	/** The plants, the same every time (in an order where a piece of a curtain follows the one it hangs from). */
	TArray<FUghFringePlant> Plan();
	/** What of a copter with its top left corner at `At` (pixels) pushes the plants. */
	FBox2D Reach(const FVector2D& At);
	/** How near a copter's `Reach` is to `Box` (pixels): 0 far, 1 touching it or in it. */
	double Touch(const FBox2D& Box, const FBox2D& Reach);
	/**
	 * Where a copter's `Reach` pushes the part `Box` (pixels) of a plant on `Side`, `Depth` units deep (pixels: x aside -
	 * positive to the screen's right -, y towards the camera); none when it is not near.
	 */
	FVector2D Push(FUghFringePlant::ESide Side, double Depth, const FBox2D& Box, const FBox2D& Reach);
	/** Where `Plant` is pushed at rest against a copter's `Reach` (Push of its box); none when it does not bend. */
	FVector2D Bend(const FUghFringePlant& Plant, const FBox2D& Reach);
	/** How near a copter's `Reach` is to `Plant`: 0 far (or it does not bend), 1 touching it or in it. */
	double Touch(const FUghFringePlant& Plant, const FBox2D& Reach);
}

/**
 * How the plants of the fringe that bend sway (Jan, step 25c): each a hanging chain - a curtain's pieces one chain from
 * the top of the hollow down to the sea, each plant of the overhang its own - of points ChainStep pixels apart, hung
 * from its top, moved aside and towards the camera (pixels) as a damped string: where a copter holds it, it is pushed
 * (UghFringe::Push), above that it leans from its top to there, below it hangs on the same distance aside - not more,
 * as a lever would swing it -, reaching down as a wave a little later and damped, then it settles. A bump shakes
 * where it hits. A plant shown is turned and moved so that its pivot and its tip go where its chain has them.
 */
class FUghFringeSway
{
public:
	/** The chains of `Plants` (those that bend). */
	void Build(const TArray<FUghFringePlant>& Plants);
	/**
	 * `Seconds` on, the copters' `Reaches` (UghFringe::Reach) holding the chains, `Bumps` (pixels a second into an edge,
	 * 0 none) shaking them; false when nothing moves (any more).
	 */
	bool Step(TConstArrayView<FBox2D> Reaches, TConstArrayView<double> Bumps, double Seconds);
	/** Whether `Plant` moved in the last step (its Motion changed). */
	bool Moved(int32 Plant) const;
	/** How `Plant` is moved from its place at rest: the world about its pivot (none for a plant that does not bend). */
	FTransform Motion(int32 Plant) const;

	/** A chain (for a look at it): its side, its points at rest (pixels), how far each is moved now (pixels). */
	struct FChain
	{
		FUghFringePlant::ESide Side = FUghFringePlant::ESide::Top;
		double Width = 0, Depth = 0;           // pixels across, units deep (its plants')
		double Spacing = 1;                    // pixels between its points (along it)
		double Wave = 1;                       // pixels a second a bend runs down it
		TArray<FVector2D> Rest;                // its points, the first its top (held)
		TArray<FVector2D> Offset, Speed;       // pixels, pixels a second
		TArray<double> Held;                   // how hard a copter holds each point now (0 .. 1)
		bool bMoving = false, bMoved = false;   // now; in the last step
	};
	TConstArrayView<FChain> Chains() const { return ChainList; }

private:
	/** A plant on its chain: where its pivot and its tip are along it (points, fractional), in the world at rest. */
	struct FOn
	{
		int32 Chain = INDEX_NONE;
		double From = 0, To = 0;
		FVector Pivot = FVector::ZeroVector, Tip = FVector::ZeroVector;
	};
	/** A chain's offset at a point along it (fractional). */
	static FVector2D OffsetAt(const FChain& Chain, double Along);

	TArray<FChain> ChainList;
	TArray<FOn> On;   // by the plants
	FRandomStream Random{ 2510 };
};

/**
 * The copters' bumps into the edges of the screen (UghFringe::LeftEdge, RightEdge, TopEdge: the logic just stops a
 * copter there), frame by frame between the logic's views: a copter reaching an edge at BumpSpeed pixels a second into
 * it or faster bumps it once, again only once it left it. The soft edges' plants shake (AUghFringe), the camera and
 * the pilot's gamepad feel it (FUghImpacts).
 */
class FUghEdgeBumps
{
public:
	static constexpr double BumpSpeed = 12;
	/** A bump: whose copter, which edge (0 left, 1 right, 2 top), how fast into it (pixels a second). */
	struct FBump
	{
		int32 Player;
		int32 Edge;
		double Speed;
	};

	/** The frame `Seconds` after the last one between the views (Alpha); nothing outside the play. */
	void See(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha, double Seconds);
	/** Where copter `Player` is seen now (its top left corner, pixels), none outside the play. */
	TOptional<FVector2D> At(int32 Player) const
	{
		return Player >= 0 && Player < UE_ARRAY_COUNT(Seen) ? Seen[Player].At : TOptional<FVector2D>();
	}
	/** The bumps of this frame. */
	TConstArrayView<FBump> GetBumps() const { return Bumps; }

private:
	/** A copter as it was seen last: where, how fast (pixels a second), at which edges. */
	struct FCopterSeen
	{
		TOptional<FVector2D> At;
		FVector2D Velocity = FVector2D::ZeroVector;
		bool bAtEdge[3] = { false, false, false };   // left, right, top
	};
	FCopterSeen Seen[2];
	TArray<FBump> Bumps;
};

/**
 * The soft edges (UghFringe): the scanned plants of the Electric Dreams sample (lianas, ferns, roots, bushes, creepers;
 * a liana of Blender's vines.py without them, the rest then absent), instanced. A copter near them pushes them away -
 * swaying as hanging chains (FUghFringeSway), each plant turned and moved instance by instance (no material of the
 * sample changed) - a bump into the edge or the overhang shakes them, a few leaves fall (the burst Rustle) and they
 * swing back; the plants sway in the wind as the sample's do.
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
	/** The copters' bumps into the edges since the last call (FUghImpacts; also without the plants). */
	TArray<FUghEdgeBumps::FBump> TakeBumps() { return MoveTemp(Bumped); }
	/**
	 * How much the plants rustle now (0 .. 1: the points of the chain swaying fastest moving RustleSpeed pixels a second
	 * on average rustle fully) and where (the world: that chain's middle); 0 while nothing moves.
	 */
	float Rustle(FVector& OutPlace) const;
	static constexpr double RustleSpeed = 40;

protected:
	virtual void BeginPlay() override;

private:
	/** A plant shown: its instance, where it is at rest. */
	struct FShown
	{
		int32 Mesh = INDEX_NONE;   // in Components
		int32 Instance = INDEX_NONE;
		FTransform Rest;
	};
	TArray<FUghFringePlant> Plants;
	TArray<FShown> Shown;
	UPROPERTY() TArray<TObjectPtr<UInstancedStaticMeshComponent>> Components;
	FUghEdgeBumps Edges;
	double Quiet[2] = { 0, 0 };   // seconds since a copter's last leaves
	TArray<FUghEdgeBumps::FBump> Bumped;
	FUghFringeSway Sway;
	bool bStill = true;   // nothing bends
};
