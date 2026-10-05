#include "UghFigures.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Texture2D.h"
#include "UghBetween.h"
#include "UghFigureActions.h"
#include "UghRockMesh.h"
#include "UghShapes.h"
#include "UghSprites.h"
#include "UghTexture.h"

namespace
{
	using UghShapes::EShape;

	const FLinearColor PassengerColor(0.75f, 0.45f, 0.3f);
	const FLinearColor EnemyColor(0.45f, 0.5f, 0.12f);
	const FLinearColor BonusColor(0.9f, 0.75f, 0.1f);

	/** The figures fill the slab of the play; a bubble is a card in front of it and of the rock's face. */
	constexpr double FigureDepth = 0, FigureThickness = UghShapes::PlaneThickness;
	constexpr double CardDepth = FUghRockMesh::FrontDepth - 3;

	/** A bubble's card stands this far above its passenger, pixels. */
	constexpr double BubbleGap = 2;

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
}

void AUghFigures::Show(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha, double Seconds,
	const FUghSprites& Sprites, const FUghFigureActions& Actions, const TArray<FTransform>& ClayRiders)
{
	if (Current.phase != UGH_LOGIC_PHASE_PLAY || Current.level_id < 0)
	{
		Clear();
		return;
	}
	ShowEntities(UghBetween::From(Previous, Current), Current, Alpha, Seconds, Sprites, Actions, ClayRiders);
}

void AUghFigures::ShowEntities(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha,
	double Seconds, const FUghSprites& Sprites, const FUghFigureActions& Actions, const TArray<FTransform>& Riders)
{
	TArray<FTransform> PassengerShapes = Riders, EnemyShapes, BonusShapes;
	TArray<FBubble> Bubbles;
	Models.Begin();
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
		if (E.kind == UGH_LOGIC_ENTITY_PASSENGER && E.bubble >= 0)
		{
			const FIntPoint Bubble = Sprites.Size(E.bubble);
			Bubbles.Add({ E.bubble, FVector2D(At.X + (Size.X - Bubble.X) / 2.0, At.Y - Bubble.Y - BubbleGap) });
		}
		const TOptional<FUghFigureAction> Action = Actions.Of(E, P);
		const double Velocity = P && P->sprite >= 0 ? double(E.x - P->x) / UghShapes::Subpixels : 0;
		if (Action && Models.Show(this, E, *Action, At, Size, Velocity, Seconds))
		{
			continue;
		}
		const FTransform Shape = UghShapes::Box(At.X, At.Y, Size.X, Size.Y, FigureDepth, FigureThickness);
		switch (E.kind)
		{
		case UGH_LOGIC_ENTITY_PASSENGER: PassengerShapes.Add(Shape); break;
		case UGH_LOGIC_ENTITY_ENEMY: EnemyShapes.Add(Shape); break;
		case UGH_LOGIC_ENTITY_BONUS_ITEM: BonusShapes.Add(Shape); break;
		default: break;
		}
	}
	Models.End();
	UghShapes::SetShapes(Passengers, PassengerShapes);
	UghShapes::SetShapes(Enemies, EnemyShapes);
	UghShapes::SetShapes(BonusItems, BonusShapes);
	ShowBubbles(Bubbles, Sprites);
}

/** Each bubble on a card of its own (the cards differ in their sprite), the cards made as they are needed. */
void AUghFigures::ShowBubbles(const TArray<FBubble>& Bubbles, const FUghSprites& Sprites)
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
		TObjectPtr<UTexture2D>& Texture = SpriteTextures.FindOrAdd(Bubble.Sprite);
		const FIntPoint Size = Sprites.Size(Bubble.Sprite);
		if (!Texture)
		{
			Texture = UghTexture::Create(this, Size.X, Size.Y, Sprites.Pixels(Bubble.Sprite), true);
		}
		UghShapes::ShowCard(BubbleCards[I], Texture, Bubble.At.X, Bubble.At.Y, Size.X, Size.Y, CardDepth);
	}
}

void AUghFigures::Clear()
{
	Models.Begin();
	Models.End();
	for (UInstancedStaticMeshComponent* Shapes : { Passengers, Enemies, BonusItems })
	{
		UghShapes::SetShapes(Shapes, {});
	}
	for (UStaticMeshComponent* Card : BubbleCards)
	{
		Card->SetVisibility(false);
	}
}
