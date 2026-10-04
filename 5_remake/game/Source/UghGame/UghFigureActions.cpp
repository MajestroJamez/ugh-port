#include "UghFigureActions.h"

#include "UghCaveman.h"
#include "UghShapes.h"

namespace
{
	/**
	 * A rule: the sprites of the animation `Animation` of a kind matching `Kind` (wildcards; "" a sprite of its own,
	 * named `Animation`) are `Action` of `Model`, looking that way, following the frames or not; a door to come out of
	 * (1) or go into (-1). The first rule that matches counts.
	 */
	struct FRule
	{
		const TCHAR* Kind;
		const TCHAR* Animation;
		EUghModel Model;
		const TCHAR* Action;
		EUghFacing Facing;
		bool bFollowsFrames;
		int32 Door;
	};

	using enum EUghModel;
	using enum EUghFacing;

	const FRule Rules[] = {
		// the passengers in the water ("kind1-water") and on land ("kind1"): cavemen
		{ TEXT("kind*-water"), TEXT("standing"), Caveman, TEXT("tread"), Camera, false, 0 },
		{ TEXT("kind*-water"), TEXT("waving"), Caveman, TEXT("tread"), Camera, false, 0 },
		{ TEXT("kind*-water"), TEXT("walkLeft"), Caveman, TEXT("swim"), Left, true, 0 },
		{ TEXT("kind*-water"), TEXT("walkRight"), Caveman, TEXT("swim"), Right, true, 0 },
		{ TEXT("kind*"), TEXT("standing"), Caveman, TEXT("idle"), Camera, false, 0 },
		{ TEXT("kind*"), TEXT("waving"), Caveman, TEXT("wave"), Camera, false, 0 },
		{ TEXT("kind*"), TEXT("walkLeft"), Caveman, TEXT("walk"), Left, true, 0 },
		{ TEXT("kind*"), TEXT("walkRight"), Caveman, TEXT("walk"), Right, true, 0 },
		{ TEXT("kind*"), TEXT("comingOut"), Caveman, TEXT("walk"), Camera, true, 1 },
		{ TEXT("kind*"), TEXT("goingIn"), Caveman, TEXT("walk"), Away, true, -1 },
		// the standing passenger: the stone with eyes, waiting, or falling after a copter let it go or it bounced
		{ TEXT(""), TEXT("standingPassenger"), Stone, TEXT("stand"), Camera, false, 0 },
		{ TEXT(""), TEXT("droppedPassenger"), Stone, TEXT("fall"), Camera, false, 0 },
		{ TEXT(""), TEXT("bouncedPassenger"), Stone, TEXT("fall"), Camera, false, 0 },
		// the enemies
		{ TEXT("flyer"), TEXT("left"), Flyer, TEXT("fly"), Left, true, 0 },
		{ TEXT("flyer"), TEXT("right"), Flyer, TEXT("fly"), Right, true, 0 },
		{ TEXT("walker"), TEXT("left"), Walker, TEXT("walk"), Left, true, 0 },
		{ TEXT("walker"), TEXT("right"), Walker, TEXT("walk"), Right, true, 0 },
		{ TEXT("walker"), TEXT("watchLeft"), Walker, TEXT("watch"), Left, false, 0 },
		{ TEXT("walker"), TEXT("watchRight"), Walker, TEXT("watch"), Right, false, 0 },
		{ TEXT("walker"), TEXT("chargeLeft"), Walker, TEXT("charge"), Left, true, 0 },
		{ TEXT("walker"), TEXT("chargeRight"), Walker, TEXT("charge"), Right, true, 0 },
		{ TEXT("walker"), TEXT("recoverLeft"), Walker, TEXT("recover"), Left, false, 0 },
		{ TEXT("walker"), TEXT("recoverRight"), Walker, TEXT("recover"), Right, false, 0 },
		{ TEXT("walker"), TEXT("stunnedLeft"), Walker, TEXT("stunned"), Left, false, 0 },
		{ TEXT("walker"), TEXT("stunnedRight"), Walker, TEXT("stunned"), Right, false, 0 },
		{ TEXT("blower"), TEXT("blowing"), Blower, TEXT("blow"), Left, true, 0 },   // it blows to the left
		{ TEXT("tree"), TEXT("swaying"), Tree, TEXT("sway"), Camera, true, 0 },
		{ TEXT(""), TEXT("shakenTree"), Tree, TEXT("shaken"), Camera, false, 0 },
		// the bonus items: a mesh each, named as their kind
		{ TEXT(""), TEXT("energy*"), BonusItem, TEXT("lie"), Camera, false, 0 },
		{ TEXT(""), TEXT("multiplier"), BonusItem, TEXT("lie"), Camera, false, 0 },
	};

	/** What each model does when a passenger knocked it out (none: nothing knocks it out). */
	const TCHAR* KnockedOut(EUghModel Model)
	{
		switch (Model)
		{
		case Flyer: return TEXT("fall");
		case Walker: case Blower: return TEXT("stunned");
		default: return nullptr;
		}
	}

	const TCHAR* const FlyerActions[] = { TEXT("fly"), TEXT("fall") };
	const TCHAR* const WalkerActions[] = { TEXT("walk"), TEXT("watch"), TEXT("charge"), TEXT("recover"),
		TEXT("stunned") };
	const TCHAR* const BlowerActions[] = { TEXT("blow"), TEXT("stunned") };
	const TCHAR* const TreeActions[] = { TEXT("sway"), TEXT("shaken") };
	const TCHAR* const StoneActions[] = { TEXT("stand"), TEXT("fall") };
	const TCHAR* const ItemActions[] = { TEXT("lie") };
}

void FUghFigureActions::Load(const ugh_logic* Logic, int32 SpriteCount)
{
	BySprite.Reset();
	for (int32 Sprite = 0; Sprite < SpriteCount; ++Sprite)
	{
		ugh_logic_sprite Info;
		if (!ugh_logic_get_sprite(Logic, Sprite, &Info))
		{
			continue;
		}
		if (TOptional<FUghFigureAction> Action = Describe(Info))
		{
			BySprite.Add(Sprite, *Action);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("UGH no rule for sprite %d (%hs): it shows as clay"), Sprite, Info.name);
		}
	}
}

TOptional<FUghFigureAction> FUghFigureActions::Of(const ugh_logic_entity& Entity, const ugh_logic_entity* Before) const
{
	const FUghFigureAction* Found = BySprite.Find(Entity.sprite);
	if (!Found)
	{
		return {};
	}
	FUghFigureAction Action = *Found;
	if (Entity.stunned && KnockedOut(Action.Model))
	{
		Action.Action = KnockedOut(Action.Model);
		Action.bFollowsFrames = false;
	}
	const bool bDown = Before && Before->sprite >= 0 &&
		double(Entity.y - Before->y) / UghShapes::Subpixels >= FallingPixelsPerStep;
	if (Action.bInWater && bDown)
	{
		Action.Action = TEXT("fall");
		Action.Facing = EUghFacing::Camera;
		Action.bFollowsFrames = false;
	}
	return Action;
}

TOptional<FUghFigureAction> FUghFigureActions::Describe(const ugh_logic_sprite& Info)
{
	FString Kind, Animation = UTF8_TO_TCHAR(Info.name);
	if (!Animation.Split(TEXT("."), &Kind, &Animation))
	{
		Kind.Reset();
	}
	for (const FRule& Rule : Rules)
	{
		const bool bKind = *Rule.Kind ? Kind.MatchesWildcard(Rule.Kind) : Kind.IsEmpty();
		if (!bKind || !Animation.MatchesWildcard(Rule.Animation))
		{
			continue;
		}
		FUghFigureAction Action;
		Action.Model = Rule.Model;
		Action.Action = Rule.Action;
		Action.Facing = Rule.Facing;
		Action.bFollowsFrames = Rule.bFollowsFrames;
		Action.bInWater = Kind.EndsWith(TEXT("-water"));
		Action.Door = Rule.Door;
		Action.Frame = Info.frame;
		Action.Frames = FMath::Max(Info.frames, 1);
		if (Rule.Model == BonusItem)
		{
			Action.Item = Animation;
		}
		return Action;
	}
	return {};
}

TConstArrayView<const TCHAR*> FUghFigureActions::ActionsOf(EUghModel Model)
{
	switch (Model)
	{
	case Caveman: return FUghCaveman::ActionNames();
	case Stone: return StoneActions;
	case Flyer: return FlyerActions;
	case Walker: return WalkerActions;
	case Blower: return BlowerActions;
	case Tree: return TreeActions;
	case BonusItem: return ItemActions;
	}
	return {};
}
