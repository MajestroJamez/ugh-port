// Which burst an event of the logic shows, and where.
#pragma once

#include "CoreMinimal.h"
#include "ugh_logic.h"
#include "UghBursts.h"

class FUghFigureActions;
class FUghSprites;

/** How an event plays its burst (as FUghSounds::EAction its sound). */
enum class EUghEffectAction : uint8
{
	Play,   // once
	Loop,   // again and again, following its entity, until the event that stops it
	Stop    // stops the loop of the same burst and entity
};

/** Where on the screen an event's burst happens. */
enum class EUghAnchor : uint8
{
	Copter,        // the middle of the player's copter's body (every copter without a player)
	CopterTop,     // above it
	CopterSkids,   // under its body
	Entity,        // the middle of the event's entity's sprite (else the player's copter)
	EntityFeet,    // its bottom
	EntityTop,     // its top (the tree's crown)
	EntityMouth,   // its side it faces, halfway up (the blower's nostrils)
	EntityWater,   // under it on the water's surface
	Shore          // the water line in the middle of the screen
};

/** The burst of an event of the logic (FUghEffectPlayer::Cues). */
struct FUghEffectCue
{
	int32 Event;                  // UGH_LOGIC_EVENT_...
	EUghBurst Burst;
	EUghEffectAction Action;
	EUghAnchor Anchor;
	double Scale = 1;
	bool bPoints = false;         // the event's value is a score shown rising from it
	TOptional<EUghBurst> ForFlyer;   // the burst instead when its entity is the flyer
};

/** A burst to show: where and how (AUghEffects). */
struct FUghEffectOrder
{
	int32 Event = 0;              // the event of the logic; 0 for what the view showed (a landing)
	EUghBurst Burst = EUghBurst::Dust;
	EUghEffectAction Action = EUghEffectAction::Play;
	FVector2D Place = FVector2D::ZeroVector;   // pixels of the screen
	double Floor = TNumericLimits<double>::Max();   // the row (pixels) of the ground or water under it, none: max
	bool bFacingLeft = false;
	double Scale = 1;
	FLinearColor Tint = FLinearColor::White;
	int32 Owner = INDEX_NONE;     // the entity (an enemy's index) a loop follows and its stop names
	int32 Points = 0;             // a score shown rising from it, 0 none
};

/**
 * The bursts of the events of the logic, as FUghSoundPlayer plays their sounds: every event has one (Cues: Table of
 * the events), placed by its entity, its player's copter or the water line; a few are seen in the view instead,
 * which has no event for them: a copter landing on a pad (dust), a bonus item landing (a little dust), a copter
 * crashing into the water (a splash with its explosion). Where an event's entity is gone from the view (a collected
 * bonus item, a boarded passenger) it is where it was last seen. Only decoration: nothing goes back to the logic.
 */
class FUghEffectPlayer
{
public:
	/** A copter landing faster than this many pixels a step raises dust. */
	static constexpr double LandingPixelsPerStep = 0.4;
	/** The tints of a collected bonus item's glints by its kind (the event's value: energy, a life, the multiplier). */
	static const FLinearColor BonusTints[3];

	static TConstArrayView<FUghEffectCue> Cues();
	/** The cue of an event of the logic, nullptr for none. */
	static const FUghEffectCue* CueOf(int32 Event);

	/** What the bursts need to know: the level's pads and rock, the sprites' sizes, what the enemies are (any null). */
	void Load(const ugh_logic* InLogic, const FUghSprites* InSprites, const FUghFigureActions* InActions);

	/** An event of the logic after the steps that led to `View`. */
	void OnEvent(const ugh_logic_event& Event, const ugh_logic_view& View);
	/** The views of the last step of a frame (after its events): the landings, where the figures were last seen. */
	void OnView(const ugh_logic_view& Previous, const ugh_logic_view& Current);
	/** The bursts since the last call. */
	TArray<FUghEffectOrder> TakeOrders();

	/** Where entity `Kind` `Index` is in `View` (its sprite's box, pixels), none when it is not shown. */
	TOptional<FBox2D> Find(const ugh_logic_view& View, int32 Kind, int32 Index) const;
	/** An order of `Burst` at `Place` in `View` (its floor, its facing), for a shot (FUghShot) or an event. */
	FUghEffectOrder OrderAt(EUghBurst Burst, const FVector2D& Place, const ugh_logic_view& View) const;
	/** Where a shot shows `Burst` in `View`: by the first copter, on the ground or the water under it (UghBursts::ERest). */
	FVector2D ShotPlace(EUghBurst Burst, const ugh_logic_view& View) const;

private:
	/** The place of `Anchor` for `Event` in `View`; none without its entity and copter. */
	TOptional<FVector2D> Place(EUghAnchor Anchor, const ugh_logic_event& Event, const ugh_logic_view& View,
		int32 Copter) const;
	/** The entity's box now, else where it was last seen. */
	TOptional<FBox2D> Seen(const ugh_logic_view& View, int32 Kind, int32 Index) const;
	/** The row of the ground (solid pixel) or the water's surface under `Place` in `View`. */
	double FloorUnder(const FVector2D& Place, const ugh_logic_view& View) const;
	/** The pad whose surface the body of copter `Player` rests on, none in the air. */
	TOptional<ugh_logic_pad> PadUnder(const ugh_logic_copter& Copter) const;
	void Forget();

	const ugh_logic* Logic = nullptr;
	const FUghSprites* Sprites = nullptr;
	const FUghFigureActions* Actions = nullptr;
	TArray<FUghEffectOrder> Orders;
	TMap<int32, FBox2D> LastSeen;   // by EntityKey
	TSet<int32> Falling;            // the bonus items (slots) falling in the last step
	bool bAirborne[2] = { false, false };
	double Descent[2] = { 0, 0 };   // pixels a step the copter went down in its last step in the air
};
