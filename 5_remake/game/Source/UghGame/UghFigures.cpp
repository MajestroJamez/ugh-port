#include "UghFigures.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Texture2D.h"
#include "UghBetween.h"
#include "UghBubbles.h"
#include "UghFigureActions.h"
#include "UghFigureLook.h"
#include "UghFling.h"
#include "UghRockMesh.h"
#include "UghShapes.h"
#include "UghSprites.h"
#include "UghTexture.h"
#include "UghWater.h"

namespace
{
	using UghShapes::EShape;

	const FLinearColor PassengerColor(0.75f, 0.45f, 0.3f);
	const FLinearColor EnemyColor(0.45f, 0.5f, 0.12f);
	const FLinearColor BonusColor(0.9f, 0.75f, 0.1f);

	/** The figures fill the slab of the play; a bubble is a card in front of it and of the rock's face. */
	constexpr double FigureDepth = 0, FigureThickness = UghShapes::PlaneThickness;
	constexpr double CardDepth = FUghRockMesh::FrontDepth - 3;

	const ugh_logic_entity* FindEntity(const ugh_logic_view& View, int32 Kind, int32 Index)
	{
		for (int32 I = 0; I < View.entity_count; ++I)
		{
			if (View.entities[I].kind == Kind && View.entities[I].index == Index)
			{
				return &View.entities[I];
			}
		}
		return nullptr;
	}
}

AUghFigures::AUghFigures()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AUghFigures::BeginPlay()
{
	Super::BeginPlay();
	auto Clay = [this](EShape Shape, const FLinearColor& Color)
	{
		return UghShapes::AddShapes(this, Shape, UghShapes::Clay(this, Color));
	};
	Models.Load();
	Passengers = Clay(EShape::Cylinder, PassengerColor);
	Enemies = Clay(EShape::Sphere, EnemyColor);
	BonusItems = Clay(EShape::Cone, BonusColor);
	for (USceneComponent* Shapes : { Passengers, Enemies, BonusItems })
	{
		UghFigureLook::Mark(Shapes);
	}
}

void AUghFigures::Show(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha, double Seconds,
	const FUghSprites& Sprites, const FUghFigureActions& Actions, const TArray<FTransform>& ClayRiders,
	const FUghFlings* Flings)
{
	if (Current.phase != UGH_LOGIC_PHASE_PLAY || Current.level_id < 0)
	{
		Clear();
		return;
	}
	ShowEntities(UghBetween::From(Previous, Current), Current, Alpha, Seconds, Sprites, Actions, ClayRiders, Flings);
}

void AUghFigures::ShowEntities(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha,
	double Seconds, const FUghSprites& Sprites, const FUghFigureActions& Actions, const TArray<FTransform>& Riders,
	const FUghFlings* Flings)
{
	TArray<FTransform> PassengerShapes = Riders, EnemyShapes, BonusShapes;
	TArray<FBubble> Bubbles;
	const double Surface = UghWater::Surface(Previous, Current, Alpha);
	Models.Begin();
	StoneDrops.Begin();
	Lively.Begin();
	for (int32 I = 0; I < Current.entity_count; ++I)
	{
		const ugh_logic_entity& E = Current.entities[I];
		if (E.sprite < 0)
		{
			continue;   // hidden
		}
		const ugh_logic_entity* P = FindEntity(Previous, E.kind, E.index);
		const FVector2D At = P && P->sprite >= 0 ? UghBetween::Position(P->x, P->y, E.x, E.y, Alpha)
			: UghBetween::Pixels(E.x, E.y);
		const FIntPoint Size = Sprites.Size(E.sprite);
		if (E.kind == UGH_LOGIC_ENTITY_PASSENGER && BubbleLooks.IsValidIndex(E.bubble) && BubbleLooks[E.bubble])
		{
			Bubbles.Add({ *BubbleLooks[E.bubble], UghBubbles::Place(At, Size.X) });
		}
		TOptional<FUghFigureAction> Action = Actions.Of(E, P);
		const double Velocity = P && P->sprite >= 0 ? double(E.x - P->x) / UghShapes::Subpixels : 0;
		FVector Offset = FVector::ZeroVector;
		const TOptional<FUghFlight> Flight = E.kind == UGH_LOGIC_ENTITY_PASSENGER && Flings
			? Flings->Of(E.index, At.Y, Surface) : TOptional<FUghFlight>();
		if (Flight)
		{
			// flung towards the camera, flailing in the air (placed as in the water from the start)
			Offset = UghShapes::ToWorld(0, -Flight->Lift, Flight->Depth) - UghShapes::ToWorld(0, 0, 0);
			if (Action && Flight->bInAir)
			{
				Action->Action = TEXT("flail");
				Action->Facing = EUghFacing::Camera;
				Action->bFollowsFrames = false;
				Action->bInWater = true;
				Action->Door = 0;
			}
		}
		else if (Action && E.kind == UGH_LOGIC_ENTITY_PASSENGER)
		{
			Lively.Apply(Previous, Current, Alpha, Seconds, E, P, At, Size, *Action);   // (the action only)
		}
		if (Action && Action->Model == EUghModel::Stone)
		{
			// let go, it falls out of the sling
			const bool bFalling = FCString::Strcmp(Action->Action, TEXT("fall")) == 0;
			const double Below = StoneDrops.Below(E.index, At.Y, bFalling);
			Offset += UghShapes::ToWorld(0, Below, 0) - UghShapes::ToWorld(0, 0, 0);
		}
		if (Action && Models.Show(this, E, *Action, At, Size, Velocity, Seconds, Offset))
		{
			continue;
		}
		FTransform Shape = UghShapes::Box(At.X, At.Y, Size.X, Size.Y, FigureDepth, FigureThickness);
		Shape.AddToTranslation(Offset);
		switch (E.kind)
		{
		case UGH_LOGIC_ENTITY_PASSENGER: PassengerShapes.Add(Shape); break;
		case UGH_LOGIC_ENTITY_ENEMY: EnemyShapes.Add(Shape); break;
		case UGH_LOGIC_ENTITY_BONUS_ITEM: BonusShapes.Add(Shape); break;
		default: break;
		}
	}
	Models.End();
	StoneDrops.End();
	Lively.End();
	UghShapes::SetShapes(Passengers, PassengerShapes);
	UghShapes::SetShapes(Enemies, EnemyShapes);
	UghShapes::SetShapes(BonusItems, BonusShapes);
	ShowBubbles(Bubbles);
}

void AUghFigures::LoadBubbles(const ugh_logic* Logic, int32 SpriteCount)
{
	BubbleLooks.Reset();
	for (int32 Sprite = 0; Sprite < SpriteCount; ++Sprite)
	{
		BubbleLooks.Add(UghBubbles::Look(Logic, Sprite));
	}
}

/** Each bubble on a card of its own (the cards differ in their picture), the cards made as they are needed. */
void AUghFigures::ShowBubbles(const TArray<FBubble>& Bubbles)
{
	for (int32 I = 0; I < FMath::Max(Bubbles.Num(), BubbleCards.Num()); ++I)
	{
		if (I >= Bubbles.Num())
		{
			BubbleCards[I]->SetVisibility(false);
			continue;
		}
		if (I >= BubbleCards.Num())
		{
			BubbleCards.Add(UghShapes::AddCard(this));
		}
		const FBubble& Bubble = Bubbles[I];
		const int32 Key = (Bubble.Look.Marks * 2 + Bubble.Look.bQuestion) * 2 + Bubble.Place.bTailLeft;
		TObjectPtr<UTexture2D>& Texture = BubbleTextures.FindOrAdd(Key);
		if (!Texture)
		{
			Texture = UghTexture::Create(this, UghBubbles::PictureWidth, UghBubbles::PictureHeight,
				UghBubbles::Draw(Bubble.Look, Bubble.Place.bTailLeft), false);
		}
		UghShapes::ShowCard(BubbleCards[I], Texture, Bubble.Place.At.X, Bubble.Place.At.Y, UghBubbles::Width,
			UghBubbles::Height, CardDepth);
	}
}

void AUghFigures::Clear()
{
	Models.Begin();
	Models.End();
	Lively.Reset();
	for (UInstancedStaticMeshComponent* Shapes : { Passengers, Enemies, BonusItems })
	{
		UghShapes::SetShapes(Shapes, {});
	}
	for (UStaticMeshComponent* Card : BubbleCards)
	{
		Card->SetVisibility(false);
	}
}

void AUghFigures::Stock()
{
	Models.Stock(this);
}
