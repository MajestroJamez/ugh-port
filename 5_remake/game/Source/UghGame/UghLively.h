// Lively passengers: what a person on land does besides what its sprite says.
#pragma once

#include "CoreMinimal.h"
#include "ugh_logic.h"

struct FUghFigureAction;

/** What a passenger on land shows (FUghLively): nothing of its own, standing, waving, ducking, glad to be home. */
enum class EUghLively : uint8 { None, Idle, Wave, Duck, Joy };

/**
 * Lively passengers (step 29d), only their animation: a person waiting on its pad stands looking about, shifting its
 * weight (the action idle), waves impatiently while a copter is Near (wave; as when the logic has it wave), ducks
 * shielding its head while a copter flies close and low by it (duck), and delivered walks off glad, its arms up (cheer:
 * the walk's legs on the sprite's frames) for JoySeconds. What the logic says stays: the place, the facing, the frames,
 * a door - only the action's name changes. Decided from the views (the copters between two steps; a passenger shown
 * again walking after it rode hidden in a copter: delivered), a wave or a duck kept a while so that it does not flicker
 * at a threshold.
 */
class FUghLively
{
public:
	/** Near: a copter's body's middle within this many pixels of the passenger's (no longer beyond Far). */
	static constexpr double Near = 72, Far = 88;
	/**
	 * Close and low: the copter's body at most Close pixels beside the passenger (no longer beyond Apart), its bottom
	 * less than Above pixels over the passenger's head (no longer: AboveOut) and at least Clear above its feet (a copter
	 * on the pad beside it is landed, not flying low), flying at least Moving pixels a step.
	 */
	static constexpr double Close = 20, Apart = 28, Above = 14, AboveOut = 20, Clear = 3, Moving = 0.5;
	/** Seconds a wave and a duck last at least; the joy after a delivery. */
	static constexpr double WaveHold = 1.0, DuckHold = 0.7, JoySeconds = 1.6;

	/** A passenger seen lively: what it shows, for how long, the middle of its sprite (pixels). */
	struct FSeen
	{
		EUghLively State = EUghLively::None;
		double Seconds = 0;
		FVector2D Middle = FVector2D::ZeroVector;
	};

	/** Begins a frame: no passenger seen yet. */
	void Begin();
	/**
	 * Makes `Action` (the rules' for the sprite of `Passenger`, `Before` it in the view of the step before) lively, the
	 * passenger's sprite of `Size` px at `At` (pixels), the copters between `Previous` and `Current` (Alpha), `Seconds`
	 * after the last frame; what it shows now.
	 */
	EUghLively Apply(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha, double Seconds,
		const ugh_logic_entity& Passenger, const ugh_logic_entity* Before, const FVector2D& At, const FIntPoint& Size,
		FUghFigureAction& Action);
	/** Ends a frame: forgets the passengers not seen in it. */
	void End();
	void Reset() { States.Reset(); }

	/** The passenger showing `State` the longest; none when none shows it. */
	TOptional<FSeen> Longest(EUghLively State) const;
	/** What passenger `Index` shows (None when not seen). */
	EUghLively Of(int32 Index) const;

	/** A copter (its corner `Corner`, pixels, moving `Speed` px a step) flies close and low by a sprite at `At`, `Size`
	 * (with `bAlready`: still, the looser bounds of one already ducking). */
	static bool IsClose(const FVector2D& Corner, double Speed, const FVector2D& At, const FIntPoint& Size, bool bAlready);
	/** A copter's corner `Corner` is near a sprite at `At`, `Size` (with `bAlready`: still). */
	static bool IsNear(const FVector2D& Corner, const FVector2D& At, const FIntPoint& Size, bool bAlready);

private:
	struct FState : FSeen
	{
		bool bSeen = false;
	};
	TMap<int32, FState> States;   // by the passenger's index
};
