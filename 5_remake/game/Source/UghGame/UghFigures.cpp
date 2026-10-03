#include "UghFigures.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "UghBackground.h"
#include "UghShapes.h"
#include "UghSpriteSizes.h"

namespace
{
	const FLinearColor CopterColors[] = { FLinearColor(0.9f, 0.45f, 0.05f), FLinearColor(0.05f, 0.6f, 0.8f) };
	const FLinearColor RotorColor(0.6f, 0.6f, 0.6f);
	const FLinearColor PassengerColor(0.2f, 0.7f, 0.2f);
	const FLinearColor BubbleColor(0.95f, 0.95f, 0.95f);
	const FLinearColor EnemyColor(0.75f, 0.1f, 0.1f);
	const FLinearColor BonusColor(0.95f, 0.8f, 0.1f);
	const FLinearColor RainColor(0.6f, 0.7f, 0.9f);

	/** The figures stand in front of the background's face, this deep (units). */
	constexpr double Depth = -AUghBackground::Thickness / 2 - 40;
	constexpr double FigureThickness = 60;

	/** A figure that moves further in one step jumped (a new attempt, a passenger getting in): not interpolated. */
	constexpr double MaxStepPixels = 8;

	/** The rotor above the body, the passenger in the cabin or hanging below, in pixels from the copter's corner. */
	constexpr double CopterMiddle = (UghShapes::CopterBodyLeft + UghShapes::CopterBodyRight + 1) / 2.0;
	constexpr double RotorWidth = 28, RotorHeight = 1.5;
	constexpr double RiderSize = 8;
	constexpr double HangingTop = UghShapes::CopterBodyHeight + 1;
	/** A bubble above a passenger, pixels. */
	constexpr double BubbleWidth = 8, BubbleHeight = 6;
	/** A raindrop, pixels. */
	constexpr double DropWidth = 1, DropHeight = 3;

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
	for (const FLinearColor& Color : CopterColors)
	{
		CopterBodies.Add(UghShapes::AddBoxes(this, Color));
	}
	Rotors = UghShapes::AddBoxes(this, RotorColor);
	Passengers = UghShapes::AddBoxes(this, PassengerColor);
	Bubbles = UghShapes::AddBoxes(this, BubbleColor);
	Enemies = UghShapes::AddBoxes(this, EnemyColor);
	BonusItems = UghShapes::AddBoxes(this, BonusColor);
	Raindrops = UghShapes::AddBoxes(this, RainColor);
}

void AUghFigures::Show(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha,
	const FUghSpriteSizes& Sizes)
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
	ShowEntities(From, Current, Alpha, Sizes, Riders);
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
				Depth, FigureThickness));
			// the rotor's sprites turn it: a blade that gets shorter and longer
			const double Blade = RotorWidth * (1 + C.rotor_sprite % 3) / 3;
			RotorBoxes.Add(UghShapes::Box(At.X + CopterMiddle - Blade / 2, At.Y, Blade, RotorHeight, Depth,
				FigureThickness / 2));
			if (C.cargo_look != 0)
			{
				const double Top = C.destination < 0 ? HangingTop : (UghShapes::CopterBodyHeight - RiderSize) / 2;
				OutRiders.Add(UghShapes::Box(At.X + CopterMiddle - RiderSize / 2, At.Y + Top, RiderSize, RiderSize,
					Depth - FigureThickness / 2, FigureThickness / 2));
			}
		}
		UghShapes::SetBoxes(CopterBodies[Player], Body);
	}
	UghShapes::SetBoxes(Rotors, RotorBoxes);
}

void AUghFigures::ShowEntities(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha,
	const FUghSpriteSizes& Sizes, const TArray<FTransform>& Riders)
{
	TArray<FTransform> PassengerBoxes = Riders, BubbleBoxes, EnemyBoxes, BonusBoxes;
	for (int32 I = 0; I < Current.entity_count; ++I)
	{
		const ugh_logic_entity& E = Current.entities[I];
		if (E.sprite < 0)
		{
			continue;   // hidden
		}
		const ugh_logic_entity* P = FindEntity(Previous, E.kind, E.index);
		const FVector2D At = P && P->sprite >= 0 ? Between(P->x, P->y, E.x, E.y, Alpha) : Pixels(E.x, E.y);
		const FIntPoint Size = Sizes.Size(E.sprite);
		const FTransform Box = UghShapes::Box(At.X, At.Y, Size.X, Size.Y, Depth, FigureThickness);
		switch (E.kind)
		{
		case UGH_LOGIC_ENTITY_PASSENGER:
			PassengerBoxes.Add(Box);
			if (E.bubble >= 0)
			{
				BubbleBoxes.Add(UghShapes::Box(At.X + (Size.X - BubbleWidth) / 2, At.Y - BubbleHeight - 2, BubbleWidth,
					BubbleHeight, Depth, FigureThickness / 2));
			}
			break;
		case UGH_LOGIC_ENTITY_ENEMY:
			EnemyBoxes.Add(Box);
			break;
		case UGH_LOGIC_ENTITY_BONUS_ITEM:
			BonusBoxes.Add(Box);
			break;
		default:
			break;
		}
	}
	UghShapes::SetBoxes(Passengers, PassengerBoxes);
	UghShapes::SetBoxes(Bubbles, BubbleBoxes);
	UghShapes::SetBoxes(Enemies, EnemyBoxes);
	UghShapes::SetBoxes(BonusItems, BonusBoxes);
}

void AUghFigures::ShowRain(const ugh_logic_view& Current)
{
	TArray<FTransform> Drops;
	for (int32 I = 0; I < Current.raindrop_count; ++I)
	{
		Drops.Add(UghShapes::Box(Current.raindrops[I][0], Current.raindrops[I][1], DropWidth, DropHeight, Depth,
			FigureThickness / 4));
	}
	UghShapes::SetBoxes(Raindrops, Drops);
}

void AUghFigures::Clear()
{
	for (UInstancedStaticMeshComponent* Body : CopterBodies)
	{
		UghShapes::SetBoxes(Body, {});
	}
	for (UInstancedStaticMeshComponent* Boxes : { Rotors, Passengers, Bubbles, Enemies, BonusItems, Raindrops })
	{
		UghShapes::SetBoxes(Boxes, {});
	}
}
