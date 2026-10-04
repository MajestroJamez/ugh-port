// What the figures of the play do.
#pragma once

#include "CoreMinimal.h"
#include "ugh_logic.h"

/** The models of the figures besides the copters (UghFigureModels). */
enum class EUghModel : uint8 { Caveman, Stone, Flyer, Walker, Blower, Tree, BonusItem };

/** Which way a figure looks: at the camera, away from it (into a door), to the left or the right of the screen. */
enum class EUghFacing : uint8 { Camera, Away, Left, Right };

/**
 * What a figure does: the model that shows it and its action (by the name its Blender script gives it), which way
 * it looks, whether the action follows the frames of the sprite's animation (the action's loop is the animation's:
 * walking, swimming, flying, blowing, swaying) or plays by itself, for a passenger whether it is in the water and
 * coming out of a door or going into one, the sprite's frame in its animation, a bonus item's kind (its mesh).
 */
struct FUghFigureAction
{
	EUghModel Model = EUghModel::Caveman;
	const TCHAR* Action = TEXT("");
	EUghFacing Facing = EUghFacing::Camera;
	bool bFollowsFrames = false;
	bool bInWater = false;
	int32 Door = 0;   // 1 coming out of a door, -1 going into one, 0 neither
	int32 Frame = 0, Frames = 1;
	FString Item;
};

/**
 * Which sprite of the logic means what (Table of rules): the logic names each sprite of its entities by the
 * animation of a kind it is a frame of ("kind1.walkLeft", "kind1-water.standing", "walker.chargeRight") or as a sprite
 * of its own ("standingPassenger", "shakenTree", a kind of bonus item "energy3"); the rules turn that name into a
 * model and an action. What the sprite does not say comes from the entity: an enemy a passenger knocked out (a
 * flyer falling shows a frame of its flight, a stunned blower one of its blowing) and a passenger in the water going
 * down (falling into it, sinking) rather than treading water.
 */
class FUghFigureActions
{
public:
	/** A passenger in the water going down this many pixels a step or more falls (or sinks). */
	static constexpr double FallingPixelsPerStep = 0.2;

	/** Names the sprites 0 .. `SpriteCount` - 1 of the data of `Logic`. */
	void Load(const ugh_logic* Logic, int32 SpriteCount);

	/** What `Entity` does, `Before` the entity in the step before (none: new); none for a sprite no rule knows. */
	TOptional<FUghFigureAction> Of(const ugh_logic_entity& Entity, const ugh_logic_entity* Before) const;

	/** The action a sprite named `Info` stands for (no entity yet); none when no rule knows it. */
	static TOptional<FUghFigureAction> Describe(const ugh_logic_sprite& Info);
	/** The actions of `Model`, in the order of its rig (none for the meshes without one: the stone, a bonus item). */
	static TConstArrayView<const TCHAR*> ActionsOf(EUghModel Model);

private:
	TMap<int32, FUghFigureAction> BySprite;
};
