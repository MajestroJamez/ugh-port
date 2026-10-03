#include "UghBackground.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "UghShapes.h"
#include "ugh_logic.h"

namespace
{
	const FLinearColor WallColor(0.05f, 0.05f, 0.06f);
	const FLinearColor RockColor(0.35f, 0.33f, 0.3f);
	const FLinearColor PadColor(0.55f, 0.35f, 0.15f);
	const FLinearColor WaterColor(0.05f, 0.2f, 0.5f);

	/** Pixels a pad's plate is high. */
	constexpr double PadHeight = 2.0;

	/** A run of solid pixels in a row: Left .. Right - 1. */
	struct FRun
	{
		int32 Left, Right;
		bool operator==(const FRun& Other) const { return Left == Other.Left && Right == Other.Right; }
	};

	TArray<FRun> SolidRuns(const ugh_logic* Logic, int32 Y)
	{
		TArray<FRun> Runs;
		for (int32 X = 0; X < UghShapes::ScreenWidth;)
		{
			if (!ugh_logic_solid(Logic, X, Y))
			{
				++X;
				continue;
			}
			const int32 Left = X;
			while (X < UghShapes::ScreenWidth && ugh_logic_solid(Logic, X, Y))
			{
				++X;
			}
			Runs.Add({ Left, X });
		}
		return Runs;
	}
}

AUghBackground::AUghBackground()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AUghBackground::BeginPlay()
{
	Super::BeginPlay();
	Wall = UghShapes::AddBoxes(this, WallColor);
	Rock = UghShapes::AddBoxes(this, RockColor);
	Pads = UghShapes::AddBoxes(this, PadColor);
	Water = UghShapes::AddBoxes(this, WaterColor);
	UghShapes::SetBoxes(Wall, { UghShapes::Box(-40, -40, UghShapes::ScreenWidth + 80, UghShapes::ScreenHeight + 80,
		Thickness, 100) });
}

void AUghBackground::Build(const ugh_logic* Logic)
{
	BuildRock(Logic);
	BuildPads(Logic);
}

/** The solid pixels as few boxes: runs of a row, merged with the same runs of the rows below. */
void AUghBackground::BuildRock(const ugh_logic* Logic)
{
	TArray<FTransform> Boxes;
	TArray<FRun> Open;      // the runs of the last row
	TArray<int32> OpenTop;  // the row where each of them started
	for (int32 Y = 0; Y <= UghShapes::ScreenHeight; ++Y)
	{
		const TArray<FRun> Runs = Y < UghShapes::ScreenHeight ? SolidRuns(Logic, Y) : TArray<FRun>();
		TArray<int32> Top;
		for (const FRun& Run : Runs)
		{
			const int32 Continued = Open.Find(Run);
			Top.Add(Continued == INDEX_NONE ? Y : OpenTop[Continued]);
		}
		for (int32 I = 0; I < Open.Num(); ++I)
		{
			if (!Runs.Contains(Open[I]))
			{
				const FRun& Run = Open[I];
				Boxes.Add(UghShapes::Box(Run.Left, OpenTop[I], Run.Right - Run.Left, Y - OpenTop[I], 0, Thickness));
			}
		}
		Open = Runs;
		OpenTop = Top;
	}
	UghShapes::SetBoxes(Rock, Boxes);
}

void AUghBackground::BuildPads(const ugh_logic* Logic)
{
	for (UTextRenderComponent* Number : PadNumbers)
	{
		Number->DestroyComponent();
	}
	PadNumbers.Reset();
	TArray<FTransform> Plates;
	const int32 Count = ugh_logic_pad_count(Logic);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		ugh_logic_pad Pad;
		ugh_logic_get_pad(Logic, Index, &Pad);
		Plates.Add(UghShapes::Box(Pad.left, Pad.y, Pad.right - Pad.left, PadHeight, -Thickness / 2, 40));

		UTextRenderComponent* Number = NewObject<UTextRenderComponent>(this);
		Number->SetText(FText::AsNumber(Pad.number));
		Number->SetHorizontalAlignment(EHTA_Center);
		Number->SetWorldSize(80);
		Number->SetTextRenderColor(FColor(40, 40, 40));
		Number->SetupAttachment(RootComponent);
		Number->RegisterComponent();
		Number->SetWorldLocationAndRotation(UghShapes::ToWorld((Pad.left + Pad.right) / 2.0, Pad.y + 3, -Thickness / 2 - 30),
			FRotator(0, 90, 0));   // facing the camera
		PadNumbers.Add(Number);
	}
	UghShapes::SetBoxes(Pads, Plates);
}

void AUghBackground::SetWater(double Surface)
{
	TArray<FTransform> Boxes;
	if (Surface < UghShapes::ScreenHeight)
	{
		const double Top = FMath::Max(Surface, 0.0);
		// behind the figures and the rock's face, from just behind that face to the plane of the play
		constexpr double Front = Thickness / 2 - 10;
		Boxes.Add(UghShapes::Box(0, Top, UghShapes::ScreenWidth, UghShapes::ScreenHeight - Top, -Front / 2, Front));
	}
	UghShapes::SetBoxes(Water, Boxes);
}
