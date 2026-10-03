#include "UghFigures.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UghMaterials.h"
#include "UghShapes.h"
#include "UghSprites.h"
#include "UghTexture.h"

namespace
{
	using UghShapes::EShape;

	const FLinearColor CopterColors[] = { FLinearColor(0.9f, 0.35f, 0.03f), FLinearColor(0.03f, 0.5f, 0.6f) };
	const FLinearColor RotorColor(0.35f, 0.22f, 0.1f);
	const FLinearColor PassengerColor(0.75f, 0.45f, 0.3f);
	const FLinearColor EnemyColor(0.45f, 0.5f, 0.12f);
	const FLinearColor BonusColor(0.9f, 0.75f, 0.1f);
	const FLinearColor RainColor(0.55f, 0.65f, 0.85f);

	/** The figures fill the slab of the play; a bubble is a card in front of it. */
	constexpr double FigureDepth = 0, FigureThickness = UghShapes::PlaneThickness;
	constexpr double CardDepth = -FigureThickness / 2 - 2, CardThickness = 1;

	/** A figure that moves further in one step jumped (a new attempt, a passenger getting in): not interpolated. */
	constexpr double MaxStepPixels = 8;

	/** The rotor above the body, the passenger in the cabin or hanging below, in pixels from the copter's corner. */
	constexpr double CopterMiddle = (UghShapes::CopterBodyLeft + UghShapes::CopterBodyRight + 1) / 2.0;
	constexpr double RotorWidth = 28, RotorHeight = 1.5;
	constexpr double RiderSize = 8;
	constexpr double HangingTop = UghShapes::CopterBodyHeight + 1;
	/** A bubble's card stands this far above its passenger, pixels. */
	constexpr double BubbleGap = 2;
	/** A raindrop: a stroke of clay this long and thick (pixels) trailing behind it, along its way (with the wind). */
	constexpr double DropLength = 4, DropWidth = 1;

	/** A position (1/32 px) in pixels. */
	FVector2D Pixels(int32 X, int32 Y)
	{
		return FVector2D(X, Y) / UghShapes::Subpixels;
	}

	/** A position (1/32 px) between two steps, in pixels. */
	FVector2D Between(int32 X0, int32 Y0, int32 X1, int32 Y1, double Alpha)
	{
		const FVector2D From = Pixels(X0, Y0), To = Pixels(X1, Y1);
		if (FMath::Abs(To.X - From.X) > MaxStepPixels || FMath::Abs(To.Y - From.Y) > MaxStepPixels)
		{
			return To;
		}
		return FMath::Lerp(From, To, Alpha);
	}

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
	for (const FLinearColor& Color : CopterColors)
	{
		CopterBodies.Add(Clay(EShape::Cube, Color));
	}
	Rotors = Clay(EShape::Cube, RotorColor);
	Passengers = Clay(EShape::Cylinder, PassengerColor);
	Enemies = Clay(EShape::Sphere, EnemyColor);
	BonusItems = Clay(EShape::Cone, BonusColor);
	Raindrops = Clay(EShape::Cylinder, RainColor);
}

void AUghFigures::Show(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha,
	const FUghSprites& Sprites)
{
	if (Current.phase != UGH_LOGIC_PHASE_PLAY || Current.level_id < 0)
	{
		Clear();
		return;
	}
	// a new level or attempt: nothing to interpolate from
	const ugh_logic_view& From = Previous.phase == UGH_LOGIC_PHASE_PLAY && Previous.level_id == Current.level_id
		? Previous : Current;
	TArray<FTransform> Riders;
	ShowCopters(From, Current, Alpha, Riders);
	ShowEntities(From, Current, Alpha, Sprites, Riders);
	ShowRain(Current);
}

void AUghFigures::ShowCopters(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha,
	TArray<FTransform>& OutRiders)
{
	TArray<FTransform> RotorBoxes;
	for (int32 Player = 0; Player < CopterBodies.Num(); ++Player)
	{
		TArray<FTransform> Body;
		if (Player < Current.copter_count)
		{
			const ugh_logic_copter& C = Current.copters[Player];
			const ugh_logic_copter& P = Player < Previous.copter_count ? Previous.copters[Player] : C;
			const FVector2D At = Between(P.x, P.y, C.x, C.y, Alpha);
			Body.Add(UghShapes::Box(At.X + UghShapes::CopterBodyLeft, At.Y + RotorHeight,
				UghShapes::CopterBodyRight - UghShapes::CopterBodyLeft + 1, UghShapes::CopterBodyHeight - RotorHeight,
				FigureDepth, FigureThickness));
			// the rotor's sprites turn it: a blade that gets shorter and longer
			const double Blade = RotorWidth * (1 + C.rotor_sprite % 3) / 3;
			RotorBoxes.Add(UghShapes::Box(At.X + CopterMiddle - Blade / 2, At.Y, Blade, RotorHeight, FigureDepth,
				FigureThickness / 2));
			if (C.cargo_look != 0)
			{
				const double Top = C.destination < 0 ? HangingTop : (UghShapes::CopterBodyHeight - RiderSize) / 2;
				OutRiders.Add(UghShapes::Box(At.X + CopterMiddle - RiderSize / 2, At.Y + Top, RiderSize, RiderSize,
					FigureDepth - FigureThickness / 2, FigureThickness / 2));
			}
		}
		UghShapes::SetShapes(CopterBodies[Player], Body);
	}
	UghShapes::SetShapes(Rotors, RotorBoxes);
}

void AUghFigures::ShowEntities(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha,
	const FUghSprites& Sprites, const TArray<FTransform>& Riders)
{
	TArray<FTransform> PassengerShapes = Riders, EnemyShapes, BonusShapes;
	TArray<FBubble> Bubbles;
	for (int32 I = 0; I < Current.entity_count; ++I)
	{
		const ugh_logic_entity& E = Current.entities[I];
		if (E.sprite < 0)
		{
			continue;   // hidden
		}
		const ugh_logic_entity* P = FindEntity(Previous, E.kind, E.index);
		const FVector2D At = P && P->sprite >= 0 ? Between(P->x, P->y, E.x, E.y, Alpha) : Pixels(E.x, E.y);
		const FIntPoint Size = Sprites.Size(E.sprite);
		const FTransform Shape = UghShapes::Box(At.X, At.Y, Size.X, Size.Y, FigureDepth, FigureThickness);
		switch (E.kind)
		{
		case UGH_LOGIC_ENTITY_PASSENGER:
			PassengerShapes.Add(Shape);
			if (E.bubble >= 0)
			{
				const FIntPoint Bubble = Sprites.Size(E.bubble);
				Bubbles.Add({ E.bubble, FVector2D(At.X + (Size.X - Bubble.X) / 2.0, At.Y - Bubble.Y - BubbleGap) });
			}
			break;
		case UGH_LOGIC_ENTITY_ENEMY:
			EnemyShapes.Add(Shape);
			break;
		case UGH_LOGIC_ENTITY_BONUS_ITEM:
			BonusShapes.Add(Shape);
			break;
		default:
			break;
		}
	}
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
			UStaticMeshComponent* Card = NewObject<UStaticMeshComponent>(this);
			Card->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
			Card->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Card->SetCastShadow(false);
			Card->SetMaterial(0, UghShapes::Material(Card, UghMaterials::Sprite));
			Card->SetupAttachment(RootComponent);
			Card->RegisterComponent();
			AddInstanceComponent(Card);
			BubbleCards.Add(Card);
		}
		const FBubble& Bubble = Bubbles[I];
		TObjectPtr<UTexture2D>& Texture = SpriteTextures.FindOrAdd(Bubble.Sprite);
		const FIntPoint Size = Sprites.Size(Bubble.Sprite);
		if (!Texture)
		{
			Texture = UghTexture::Create(this, Size.X, Size.Y, Sprites.Pixels(Bubble.Sprite), true);
		}
		UStaticMeshComponent* Card = BubbleCards[I];
		Cast<UMaterialInstanceDynamic>(Card->GetMaterial(0))->SetTextureParameterValue(UghMaterials::ArtParameter, Texture);
		// a thin box as big as the sprite: its front face shows the sprite
		Card->SetWorldTransform(UghShapes::Box(Bubble.At.X, Bubble.At.Y, Size.X, Size.Y, CardDepth, CardThickness));
		Card->SetVisibility(true);
	}
}

void AUghFigures::ShowRain(const ugh_logic_view& Current)
{
	// a drop falls as many pixels down as with the wind each step (45 degrees): a cylinder (along Z) laid that way
	const FVector2D Way = FVector2D(Current.wind, 1).GetSafeNormal();
	const FVector Along = (UghShapes::ToWorld(Way.X, Way.Y, 0) - UghShapes::ToWorld(0, 0, 0)).GetSafeNormal();
	const FQuat Turn = FRotationMatrix::MakeFromZY(Along, FVector::YAxisVector).ToQuat();
	const double Across = DropWidth * UghShapes::UnitsPerPixel / UghShapes::ShapeSize;
	const FVector Scale(Across, Across, DropLength * UghShapes::UnitsPerPixel / UghShapes::ShapeSize);
	TArray<FTransform> Drops;
	for (int32 I = 0; I < Current.raindrop_count; ++I)
	{
		const FVector2D Middle = FVector2D(Current.raindrops[I][0] + 0.5, Current.raindrops[I][1] + 0.5) -
			Way * DropLength / 2;
		Drops.Add(FTransform(Turn, UghShapes::ToWorld(Middle.X, Middle.Y, FigureDepth), Scale));
	}
	UghShapes::SetShapes(Raindrops, Drops);
}

void AUghFigures::Clear()
{
	for (UInstancedStaticMeshComponent* Body : CopterBodies)
	{
		UghShapes::SetShapes(Body, {});
	}
	for (UInstancedStaticMeshComponent* Shapes : { Rotors, Passengers, Enemies, BonusItems, Raindrops })
	{
		UghShapes::SetShapes(Shapes, {});
	}
	for (UStaticMeshComponent* Card : BubbleCards)
	{
		Card->SetVisibility(false);
	}
}
