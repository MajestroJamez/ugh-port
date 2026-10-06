#include "UghEffectPlayer.h"

#include "Algo/Find.h"
#include "UghBetween.h"
#include "UghFigureActions.h"
#include "UghFling.h"
#include "UghShapes.h"
#include "UghSprites.h"

namespace
{
	using EAction = EUghEffectAction;
	using EAnchor = EUghAnchor;

	/** The burst of every event (the sounds' are FUghSounds::Cues: an event plays both). */
	const FUghEffectCue CueTable[] = {
		{ UGH_LOGIC_EVENT_LEVEL_CAPTION, EUghBurst::Surf, EAction::Play, EAnchor::Shore },
		{ UGH_LOGIC_EVENT_COPTER_CRASHED, EUghBurst::Explosion, EAction::Play, EAnchor::Copter },
		{ UGH_LOGIC_EVENT_LEVEL_DONE, EUghBurst::Celebration, EAction::Play, EAnchor::CopterTop },
		{ UGH_LOGIC_EVENT_PASSENGER_BOARDED, EUghBurst::Dust, EAction::Play, EAnchor::EntityFeet, 0.35 },
		{ UGH_LOGIC_EVENT_PASSENGER_PAID, EUghBurst::Shells, EAction::Play, EAnchor::Copter, 1, true },
		{ UGH_LOGIC_EVENT_QUICK_DELIVERY, EUghBurst::Glints, EAction::Play, EAnchor::CopterSkids },
		{ UGH_LOGIC_EVENT_PASSENGER_DROPPED, EUghBurst::Dust, EAction::Play, EAnchor::CopterSkids, 0.3 },
		{ UGH_LOGIC_EVENT_PASSENGER_IN_WATER, EUghBurst::Splash, EAction::Play, EAnchor::EntityWater },
		{ UGH_LOGIC_EVENT_FLYER_SCREECH, EUghBurst::Feathers, EAction::Play, EAnchor::Entity, 0.6 },
		{ UGH_LOGIC_EVENT_FLYER_FLAP_START, EUghBurst::Downdraft, EAction::Loop, EAnchor::Entity },
		{ UGH_LOGIC_EVENT_FLYER_FLAP_STOP, EUghBurst::Downdraft, EAction::Stop, EAnchor::Entity },
		{ UGH_LOGIC_EVENT_BLOWER_BLOW, EUghBurst::Gust, EAction::Play, EAnchor::EntityMouth },
		{ UGH_LOGIC_EVENT_ENEMY_STUNNED, EUghBurst::Thud, EAction::Play, EAnchor::EntityFeet, 1, true,
			EUghBurst::Feathers },
		{ UGH_LOGIC_EVENT_TREE_DROP, EUghBurst::Leaves, EAction::Play, EAnchor::EntityTop },
		{ UGH_LOGIC_EVENT_BONUS_COLLECTED, EUghBurst::Glints, EAction::Play, EAnchor::Entity },
	};

	/** A copter landing raises its dust this much bigger for each pixel a step it came down at (at least, at most). */
	constexpr double DustPerDescent = 0.6, LeastDust = 0.5, MostDust = 1.5;
	/** A bonus item landing raises a little dust. */
	constexpr double ItemDust = 0.3;
	/** A crash into the water splashes this big (with its explosion). */
	constexpr double CrashSplash = 2;
	/** A shot shows a burst in the air this far beside the copter (towards the middle of the screen) and above it. */
	constexpr double ShotAside = 40, ShotAbove = 10, CopterTopRoom = 8;

	/** The kind of entity an event names (UGH_LOGIC_ENTITY_...), 0 none. */
	int32 EntityKindOf(int32 Event)
	{
		switch (Event)
		{
		case UGH_LOGIC_EVENT_PASSENGER_BOARDED: case UGH_LOGIC_EVENT_PASSENGER_PAID:
		case UGH_LOGIC_EVENT_QUICK_DELIVERY: case UGH_LOGIC_EVENT_PASSENGER_DROPPED:
		case UGH_LOGIC_EVENT_PASSENGER_IN_WATER:
			return UGH_LOGIC_ENTITY_PASSENGER;
		case UGH_LOGIC_EVENT_FLYER_SCREECH: case UGH_LOGIC_EVENT_FLYER_FLAP_START: case UGH_LOGIC_EVENT_FLYER_FLAP_STOP:
		case UGH_LOGIC_EVENT_BLOWER_BLOW: case UGH_LOGIC_EVENT_ENEMY_STUNNED: case UGH_LOGIC_EVENT_TREE_DROP:
			return UGH_LOGIC_ENTITY_ENEMY;
		case UGH_LOGIC_EVENT_BONUS_COLLECTED:
			return UGH_LOGIC_ENTITY_BONUS_ITEM;
		default:
			return 0;
		}
	}

	int32 EntityKey(int32 Kind, int32 Index) { return Kind * UGH_LOGIC_MAX_ENTITIES + Index; }

	const ugh_logic_entity* EntityIn(const ugh_logic_view& View, int32 Kind, int32 Index)
	{
		for (int32 I = 0; I < View.entity_count; ++I)
		{
			const ugh_logic_entity& Entity = View.entities[I];
			if (Entity.kind == Kind && Entity.index == Index && Entity.sprite >= 0)
			{
				return &Entity;
			}
		}
		return nullptr;
	}

	/** The body of a copter (pixels). */
	FBox2D BodyOf(const ugh_logic_copter& Copter)
	{
		const FVector2D Corner = UghBetween::Pixels(Copter.x, Copter.y);
		return FBox2D(Corner + FVector2D(UghShapes::CopterBodyLeft, 0),
			Corner + FVector2D(UghShapes::CopterBodyRight + 1, UghShapes::CopterBodyHeight));
	}

	double WaterRow(const ugh_logic_view& View)
	{
		return double(View.water_level) / UghShapes::Subpixels;
	}
}

const FLinearColor FUghEffectPlayer::BonusTints[3] = {
	FLinearColor(1.f, 0.8f, 0.35f), FLinearColor(1.f, 0.4f, 0.35f), FLinearColor(1.f, 0.9f, 0.55f) };

TConstArrayView<FUghEffectCue> FUghEffectPlayer::Cues()
{
	return CueTable;
}

const FUghEffectCue* FUghEffectPlayer::CueOf(int32 Event)
{
	return Algo::FindBy(CueTable, Event, &FUghEffectCue::Event);
}

void FUghEffectPlayer::Load(const ugh_logic* InLogic, const FUghSprites* InSprites, const FUghFigureActions* InActions,
	const FUghFlings* InFlings)
{
	Logic = InLogic;
	Sprites = InSprites;
	Actions = InActions;
	Flings = InFlings;
	Forget();
}

void FUghEffectPlayer::OnEvent(const ugh_logic_event& Event, const ugh_logic_view& View)
{
	const FUghEffectCue* Cue = CueOf(Event.kind);
	if (!Cue || (Event.kind == UGH_LOGIC_EVENT_PASSENGER_IN_WATER && Flings && Flings->IsFlung(Event.entity)))
	{
		return;   // (a flung passenger splashes where its feet reach the water: FUghFlings)
	}
	const ugh_logic_entity* Entity =
		Event.entity >= 0 ? EntityIn(View, EntityKindOf(Event.kind), Event.entity) : nullptr;
	const TOptional<FUghFigureAction> Action =
		Entity && Actions ? Actions->Of(*Entity, nullptr) : TOptional<FUghFigureAction>();
	const EUghBurst Burst = Cue->ForFlyer && Action && Action->Model == EUghModel::Flyer ? *Cue->ForFlyer : Cue->Burst;
	// the copters it happens at: the player's, else every one (the level done)
	const bool bEveryCopter = Event.player < 0 &&
		(Cue->Anchor == EAnchor::Copter || Cue->Anchor == EAnchor::CopterTop || Cue->Anchor == EAnchor::CopterSkids);
	const int32 First = Event.player >= 0 ? Event.player : 0;
	const int32 Last = bEveryCopter ? View.copter_count - 1 : First;
	for (int32 Copter = First; Copter <= Last; ++Copter)
	{
		const TOptional<FVector2D> At = Cue->Action == EAction::Stop ? FVector2D::ZeroVector
			: Place(Cue->Anchor, Event, View, Copter);
		if (!At)
		{
			continue;
		}
		FUghEffectOrder Order = OrderAt(Burst, *At, View);
		Order.Event = Event.kind;
		Order.Action = Cue->Action;
		Order.Scale = Cue->Scale;
		Order.Owner = Event.entity;
		Order.Points = Cue->bPoints ? Event.value : 0;
		Order.bFacingLeft = Action && Action->Facing == EUghFacing::Left;
		if (Event.kind == UGH_LOGIC_EVENT_BONUS_COLLECTED)
		{
			Order.Tint = BonusTints[FMath::Clamp(Event.value, 0, int32(UE_ARRAY_COUNT(BonusTints)) - 1)];
		}
		Orders.Add(Order);
	}
	// a crash into the water splashes too
	if (Event.kind == UGH_LOGIC_EVENT_COPTER_CRASHED && Event.player >= 0 && Event.player < View.copter_count)
	{
		const FBox2D Body = BodyOf(View.copters[Event.player]);
		if (Body.Max.Y >= WaterRow(View) - 1)
		{
			FUghEffectOrder Splash = OrderAt(EUghBurst::Splash, FVector2D(Body.GetCenter().X, WaterRow(View)), View);
			Splash.Event = Event.kind;
			Splash.Scale = CrashSplash;
			Orders.Add(Splash);
		}
	}
}

void FUghEffectPlayer::OnView(const ugh_logic_view& Previous, const ugh_logic_view& Current)
{
	if (Current.phase != UGH_LOGIC_PHASE_PLAY || &UghBetween::From(Previous, Current) == &Current)
	{
		Forget();   // between attempts: nothing lands, nothing is remembered
		return;
	}
	for (int32 Copter = 0; Copter < FMath::Min(Current.copter_count, int32(UE_ARRAY_COUNT(bAirborne))); ++Copter)
	{
		const ugh_logic_copter& Now = Current.copters[Copter];
		if (const TOptional<ugh_logic_pad> Pad = PadUnder(Now))
		{
			if (bAirborne[Copter] && Descent[Copter] >= LandingPixelsPerStep)
			{
				FUghEffectOrder Dust = OrderAt(EUghBurst::Dust, FVector2D(BodyOf(Now).GetCenter().X, Pad->y), Current);
				Dust.Scale = FMath::Clamp(Descent[Copter] * DustPerDescent, LeastDust, MostDust);
				Orders.Add(Dust);
			}
			bAirborne[Copter] = false;
		}
		else
		{
			bAirborne[Copter] = true;
			Descent[Copter] = double(Now.y - Previous.copters[Copter].y) / UghShapes::Subpixels;
		}
	}
	TSet<int32> FallingNow;
	for (int32 I = 0; I < Current.entity_count; ++I)
	{
		const ugh_logic_entity& Entity = Current.entities[I];
		if (Entity.sprite < 0)
		{
			continue;
		}
		const TOptional<FBox2D> Box = Find(Current, Entity.kind, Entity.index);
		LastSeen.Add(EntityKey(Entity.kind, Entity.index), *Box);
		const ugh_logic_entity* Before = Entity.kind == UGH_LOGIC_ENTITY_BONUS_ITEM
			? EntityIn(Previous, Entity.kind, Entity.index) : nullptr;
		if (Before && Entity.y > Before->y)
		{
			FallingNow.Add(Entity.index);
		}
		else if (Before && Entity.y == Before->y && Falling.Contains(Entity.index))
		{
			FUghEffectOrder Dust = OrderAt(EUghBurst::Dust, FVector2D(Box->GetCenter().X, Box->Max.Y), Current);
			Dust.Scale = ItemDust;
			Orders.Add(Dust);
		}
	}
	Falling = MoveTemp(FallingNow);
}

TArray<FUghEffectOrder> FUghEffectPlayer::TakeOrders()
{
	return MoveTemp(Orders);
}

TOptional<FBox2D> FUghEffectPlayer::Find(const ugh_logic_view& View, int32 Kind, int32 Index) const
{
	const ugh_logic_entity* Entity = EntityIn(View, Kind, Index);
	if (!Entity)
	{
		return {};
	}
	const FVector2D Corner = UghBetween::Pixels(Entity->x, Entity->y);
	static const FUghSprites NoSprites;   // (their default size)
	const FIntPoint Size = (Sprites ? *Sprites : NoSprites).Size(Entity->sprite);
	return FBox2D(Corner, Corner + FVector2D(Size));
}

FUghEffectOrder FUghEffectPlayer::OrderAt(EUghBurst Burst, const FVector2D& Place, const ugh_logic_view& View) const
{
	FUghEffectOrder Order;
	Order.Burst = Burst;
	Order.Place = Place;
	Order.Floor = FloorUnder(Place, View);
	return Order;
}

FVector2D FUghEffectPlayer::ShotPlace(EUghBurst Burst, const ugh_logic_view& View) const
{
	const FBox2D Body = View.copter_count > 0 ? BodyOf(View.copters[0])
		: FBox2D(UghShapes::Screen().GetCenter(), UghShapes::Screen().GetCenter());
	const FVector2D Middle = Body.GetCenter();
	switch (UghBursts::Get(Burst).Rest)
	{
	case UghBursts::ERest::Water:
		return FVector2D(Middle.X, FMath::Min(WaterRow(View), double(UghShapes::ScreenHeight)));
	case UghBursts::ERest::Ground:
		return FVector2D(Middle.X, FloorUnder(FVector2D(Middle.X, Body.Max.Y), View));
	default:
		return Middle + FVector2D(Middle.X < UghShapes::ScreenWidth / 2 ? ShotAside : -ShotAside, -ShotAbove);
	}
}

TOptional<FVector2D> FUghEffectPlayer::Place(EUghAnchor Anchor, const ugh_logic_event& Event,
	const ugh_logic_view& View, int32 Copter) const
{
	const TOptional<FBox2D> Body = Copter < View.copter_count ? BodyOf(View.copters[Copter]) : TOptional<FBox2D>();
	switch (Anchor)
	{
	case EAnchor::Copter: return Body ? Body->GetCenter() : TOptional<FVector2D>();
	case EAnchor::CopterTop:
		return Body ? FVector2D(Body->GetCenter().X, Body->Min.Y - CopterTopRoom) : TOptional<FVector2D>();
	case EAnchor::CopterSkids: return Body ? FVector2D(Body->GetCenter().X, Body->Max.Y) : TOptional<FVector2D>();
	case EAnchor::Shore: return FVector2D(UghShapes::ScreenWidth / 2.0, WaterRow(View));
	default: break;
	}
	const TOptional<FBox2D> Box = Seen(View, EntityKindOf(Event.kind), Event.entity);
	if (!Box)
	{
		return Event.player >= 0 && Body ? Body->GetCenter() : TOptional<FVector2D>();
	}
	const FVector2D Middle = Box->GetCenter();
	switch (Anchor)
	{
	case EAnchor::EntityFeet: return FVector2D(Middle.X, Box->Max.Y);
	case EAnchor::EntityTop: return FVector2D(Middle.X, Box->Min.Y + Box->GetSize().Y / 4);
	case EAnchor::EntityWater: return FVector2D(Middle.X, WaterRow(View));
	case EAnchor::EntityMouth:
	{
		const ugh_logic_entity* Entity = EntityIn(View, EntityKindOf(Event.kind), Event.entity);
		const TOptional<FUghFigureAction> Action =
			Entity && Actions ? Actions->Of(*Entity, nullptr) : TOptional<FUghFigureAction>();
		const bool bLeft = !Action || Action->Facing != EUghFacing::Right;
		return FVector2D(bLeft ? Box->Min.X : Box->Max.X, Middle.Y);
	}
	default: return Middle;
	}
}

TOptional<FBox2D> FUghEffectPlayer::Seen(const ugh_logic_view& View, int32 Kind, int32 Index) const
{
	if (const TOptional<FBox2D> Box = Find(View, Kind, Index))
	{
		return Box;
	}
	const FBox2D* Last = LastSeen.Find(EntityKey(Kind, Index));
	return Last ? *Last : TOptional<FBox2D>();
}

double FUghEffectPlayer::FloorUnder(const FVector2D& Place, const ugh_logic_view& View) const
{
	const double Water = WaterRow(View);
	const int32 X = FMath::FloorToInt32(Place.X);
	const double Bottom = FMath::Min(Water, double(UghShapes::ScreenHeight));
	for (int32 Row = FMath::Max(0, FMath::CeilToInt32(Place.Y)); Logic && Row < Bottom; ++Row)
	{
		if (ugh_logic_solid(Logic, X, Row))
		{
			return Row;
		}
	}
	return Water;
}

TOptional<ugh_logic_pad> FUghEffectPlayer::PadUnder(const ugh_logic_copter& Copter) const
{
	const FBox2D Body = BodyOf(Copter);
	for (int32 Index = 0; Logic && Index < ugh_logic_pad_count(Logic); ++Index)
	{
		ugh_logic_pad Pad;
		if (ugh_logic_get_pad(Logic, Index, &Pad) && FMath::Abs(Body.Max.Y - Pad.y) <= 1 &&
			Body.Max.X > Pad.left && Body.Min.X < Pad.right + 1)
		{
			return Pad;
		}
	}
	return {};
}

void FUghEffectPlayer::Forget()
{
	LastSeen.Reset();
	Falling.Reset();
	bAirborne[0] = bAirborne[1] = false;
}

void FUghEffectPlayer::Order(EUghBurst Burst, const FVector2D& Place, const ugh_logic_view& View, double Scale,
	double Depth)
{
	FUghEffectOrder Order = OrderAt(Burst, Place, View);
	Order.Scale = Scale;
	Order.Depth = Depth;
	Orders.Add(Order);
}
