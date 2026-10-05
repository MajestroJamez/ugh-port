// Where scanned cliffs and roots dress the cave of a level.
#pragma once

#include "CoreMinimal.h"
#include "UghDecorations.h"

class FUghRockField;
struct ugh_logic;

/**
 * A scanned model set into the cave's back wall of a level: a cliff (facing the camera, give or take) or roots hanging
 * from a ceiling. Its model fits a ball of Radius around X, Y (turned by Rotation); it is pushed into the wall so that
 * Out of its depth comes out of the wall's surface at Surface, but never nearer the camera than Limit.
 */
struct FUghRockPiece
{
	enum class EKind : uint8 { Cliff, Root };

	EKind Kind = EKind::Cliff;
	/** Pixels of the screen: the middle of its ball. */
	double X = 0, Y = 0;
	/** Pixels: the radius of the ball its model fits in. */
	double Radius = 0;
	/** Units behind the plane of the play: the wall's surface it comes out of. */
	double Surface = 0;
	/** How much of its model's depth comes out of the surface (0 .. 1). */
	double Out = 0;
	/** Units: its model comes no nearer the camera than this. */
	double Limit = 0;
	FRotator Rotation = FRotator::ZeroRotator;
	/** Which of the models of its kind (modulo their count). */
	int32 Variant = 0;

	/** The nearest its model may come to the camera, units. */
	double Nearest() const;

	bool operator==(const FUghRockPiece& Other) const = default;
};

/**
 * The scanned rock dressing the cave of the level being played (AUghCliffDressing shows it): cliffs overlapping into
 * a wall of scanned rock on the cave's back wall and roots hanging from its ceilings there, behind everything a figure
 * reaches (BackFront) and behind the middles of the decorations in front of them, none seen in a cave's entrance
 * (FUghCavePortal). The rock's edge in the plane of the play stays the collision mask's. The same level always gets the same ones (its level_id seeds the choice).
 */
namespace UghRockDressing
{
	/** A piece stays this far behind the plane of the play (units): behind every figure's sweep. */
	constexpr double BackFront = UghDecorations::SweepReach + 70;

	/**
	 * The pieces of the level being played (its rock `Field`, level `LevelId`), behind the middles of its `Decorations`
	 * (but the ground cover, whose rows in front stay in view, and the lianas, which may hang into the cliffs); none
	 * before a level.
	 */
	TArray<FUghRockPiece> Plan(const ugh_logic* Logic, const FUghRockField& Field, int32 LevelId,
		const TArray<FUghDecoration>& Decorations);
	/** Where the model of `Piece` may be on the screen (pixels). */
	FBox2D ScreenBox(const FUghRockPiece& Piece);
	/**
	 * `Decoration` is seen where the model of `Piece` may be (their boxes on the screen meet), and it is neither ground
	 * cover nor a liana.
	 */
	bool InFrontOf(const FUghDecoration& Decoration, const FUghRockPiece& Piece);
}
