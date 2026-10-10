#include "UghCaveAir.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UghMaterials.h"
#include "UghMood.h"
#include "UghRockField.h"
#include "UghShapes.h"
#include "UghTexture.h"

namespace
{
	using EKind = FUghMood::EKind;

	/** The card: this far behind the plane of the play, pixels (the figures' room ends 9 px behind it). */
	constexpr double CardDepth = 12;
	/** A campfire's glow and a torch's: where it is above its foot (pixels), how far it reaches, how bright. */
	constexpr double CampfireUp = 3, CampfireReach = 70, TorchUp = 9, TorchReach = 40;
	constexpr float CampfireStrength = 1.f, TorchStrength = 0.7f;
	/**
	 * The colours as the exposure shows them: the haze the shade's light (FUghMood::Shade) times HazeShare; the shafts
	 * the sun's (the moon's) colour, the fires' glow a warm one times the mood's fire light, as bright as each mood
	 * wants them (a storm's sun hardly makes shafts, the fires glow most at night).
	 */
	constexpr float HazeShare = 0.05f;   // 0.08 lifted the caves' black (32b)
	const FLinearColor FireColor(1.f, 0.42f, 0.13f);
	float ShaftShare(EKind Kind)
	{
		switch (Kind)
		{
		case EKind::Day: return 0.06f;
		case EKind::Evening: return 0.075f;
		case EKind::Dusk: return 0.055f;
		case EKind::Night: return 0.04f;
		default: return 0.02f;
		}
	}
	float GlowShare(EKind Kind)
	{
		switch (Kind)
		{
		case EKind::Day: return 0.08f;
		case EKind::Evening: return 0.1f;
		case EKind::Dusk: return 0.13f;
		case EKind::Night: return 0.16f;
		default: return 0.1f;
		}
	}

	constexpr int32 Width = UghCaveAir::Width, Height = UghCaveAir::Height;

	/** Each cell's channels softened by its neighbours (3 x 3). */
	TArray<FLinearColor> Soften(const TArray<FLinearColor>& Cells)
	{
		TArray<FLinearColor> Soft;
		Soft.SetNumZeroed(Cells.Num());
		for (int32 J = 0; J < Height; ++J)
		{
			for (int32 I = 0; I < Width; ++I)
			{
				FLinearColor Sum(0, 0, 0, 0);
				int32 Count = 0;
				for (int32 Y = FMath::Max(J - 1, 0); Y <= FMath::Min(J + 1, Height - 1); ++Y)
				{
					for (int32 X = FMath::Max(I - 1, 0); X <= FMath::Min(I + 1, Width - 1); ++X)
					{
						Sum += Cells[Y * Width + X];
						++Count;
					}
				}
				Soft[J * Width + I] = Sum / float(Count);
			}
		}
		return Soft;
	}
}

TArray<FUghAirFire> UghCaveAir::Fires(TConstArrayView<FUghDecoration> Decorations, double WaterRow)
{
	TArray<FUghAirFire> Fires;
	for (const FUghDecoration& Decoration : Decorations)
	{
		const bool bCampfire = Decoration.Kind == FUghDecoration::EKind::Campfire;
		if ((bCampfire || Decoration.Kind == FUghDecoration::EKind::Torch) && Decoration.Y < WaterRow)
		{
			Fires.Add({ FVector2D(Decoration.X, Decoration.Y - (bCampfire ? CampfireUp : TorchUp)),
				bCampfire ? CampfireReach : TorchReach, bCampfire ? CampfireStrength : TorchStrength });
		}
	}
	return Fires;
}

TArray<FLinearColor> UghCaveAir::Map(const ugh_logic* Logic, const FVector2D& Way, TConstArrayView<FUghAirFire> Fires)
{
	if (!Logic)
	{
		return {};
	}
	// how much of each cell is air
	TArray<float> Air;
	Air.SetNumZeroed(Width * Height);
	for (int32 J = 0; J < Height; ++J)
	{
		for (int32 I = 0; I < Width; ++I)
		{
			int32 Open = 0;
			for (int32 Y = 0; Y < Scale; ++Y)
			{
				for (int32 X = 0; X < Scale; ++X)
				{
					Open += ugh_logic_solid(Logic, I * Scale + X, J * Scale + Y) ? 0 : 1;
				}
			}
			Air[J * Width + I] = float(Open) / (Scale * Scale);
		}
	}
	// rock: a cell mostly rock; off the screen nothing (the light comes in there)
	auto Rock = [&](int32 I, int32 J)
	{
		return I >= 0 && I < Width && J >= 0 && J < Height && Air[J * Width + I] < 0.5f;
	};
	auto Off = [](int32 I, int32 J) { return I < 0 || I >= Width || J < 0 || J >= Height; };
	// how far a march from the cell's middle along `Step` (cells) goes before rock (or Reach); bOut: it left the screen
	auto March = [&](int32 I, int32 J, const FVector2D& Step, double Reach, bool& bOut)
	{
		FVector2D At(I + 0.5, J + 0.5);
		for (double Gone = 1; Gone <= Reach; ++Gone)
		{
			At += Step;
			const int32 X = FMath::FloorToInt32(At.X), Y = FMath::FloorToInt32(At.Y);
			if (Off(X, Y))
			{
				bOut = true;
				return Gone;
			}
			if (Rock(X, Y))
			{
				return Gone;
			}
		}
		return Reach + 1;
	};
	const FVector2D Back = -Way.GetSafeNormal();
	const FVector2D Ups[] = { FVector2D(0, -1), FVector2D(-0.7071, -0.7071), FVector2D(0.7071, -0.7071) };
	const double ShelterCells = ShelterReach / Scale, ShaftCells = ShaftReach / Scale;
	TArray<FLinearColor> Cells;
	Cells.SetNumZeroed(Width * Height);
	for (int32 J = 0; J < Height; ++J)
	{
		for (int32 I = 0; I < Width; ++I)
		{
			FLinearColor& Cell = Cells[J * Width + I];
			Cell.A = Air[J * Width + I];
			if (Rock(I, J))
			{
				continue;
			}
			// sheltered: rock above it and beside above, the nearer the more
			double Shelter = 0;
			for (const FVector2D& Up : Ups)
			{
				bool bOut = false;
				const double Gone = March(I, J, Up, ShelterCells, bOut);
				Shelter += !bOut && Gone <= ShelterCells ? 1 - 0.5 * Gone / ShelterCells : 0;
			}
			Cell.R = float(Shelter / UE_ARRAY_COUNT(Ups));
			// lit by a shaft: its way back against the light leaves the screen through air, the nearer the brighter
			bool bOut = false;
			const double Gone = March(I, J, Back, ShaftCells, bOut);
			Cell.G = bOut ? float(1 - 0.7 * Gone / ShaftCells) : 0.f;
		}
	}
	// the fires' glow: along the shortest way through the air (8 neighbours), fading with it
	TArray<float> Distance;
	for (const FUghAirFire& Fire : Fires)
	{
		const double Reach = Fire.Reach / Scale;
		// (a torch wedged into a wall: from the nearest air)
		const int32 FireI = FMath::FloorToInt32(Fire.At.X / Scale), FireJ = FMath::FloorToInt32(Fire.At.Y / Scale);
		TOptional<FIntPoint> From;
		for (int32 Ring = 0; Ring <= 3 && !From; ++Ring)
		{
			for (int32 Y = -Ring; Y <= Ring && !From; ++Y)
			{
				for (int32 X = -Ring; X <= Ring && !From; ++X)
				{
					if (!Off(FireI + X, FireJ + Y) && !Rock(FireI + X, FireJ + Y))
					{
						From = FIntPoint(FireI + X, FireJ + Y);
					}
				}
			}
		}
		if (!From)
		{
			continue;
		}
		Distance.Init(UE_BIG_NUMBER, Width * Height);
		TArray<TPair<float, int32>> Open;   // a heap: the way so far, the cell
		auto Visit = [&](int32 I, int32 J, float Gone)
		{
			if (!Off(I, J) && !Rock(I, J) && Gone <= Reach && Gone < Distance[J * Width + I])
			{
				Distance[J * Width + I] = Gone;
				Open.HeapPush(TPair<float, int32>(Gone, J * Width + I),
					[](const TPair<float, int32>& A, const TPair<float, int32>& B) { return A.Key < B.Key; });
			}
		};
		Visit(From->X, From->Y, 0);
		while (Open.Num() > 0)
		{
			TPair<float, int32> Next;
			Open.HeapPop(Next, [](const TPair<float, int32>& A, const TPair<float, int32>& B) { return A.Key < B.Key; },
				EAllowShrinking::No);
			if (Next.Key > Distance[Next.Value])
			{
				continue;
			}
			const int32 I = Next.Value % Width, J = Next.Value / Width;
			for (int32 Y = -1; Y <= 1; ++Y)
			{
				for (int32 X = -1; X <= 1; ++X)
				{
					if (X != 0 || Y != 0)
					{
						Visit(I + X, J + Y, Next.Key + (X != 0 && Y != 0 ? 1.4142f : 1.f));
					}
				}
			}
		}
		for (int32 Index = 0; Index < Cells.Num(); ++Index)
		{
			if (Distance[Index] <= Reach)
			{
				const float Near = 1.f - float(Distance[Index] / Reach);
				Cells[Index].B = FMath::Min(1.f, Cells[Index].B + Fire.Strength * Near * Near);
			}
		}
	}
	return Soften(Cells);
}

AUghCaveAir::AUghCaveAir()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AUghCaveAir::BeginPlay()
{
	Super::BeginPlay();
	Card = NewObject<UStaticMeshComponent>(this);
	Card->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
	Card->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Card->SetCastShadow(false);
	Card->SetVisibleInRayTracing(false);
	Card->bAffectDynamicIndirectLighting = false;
	Card->SetReceivesDecals(false);
	Card->SetTranslucentSortPriority(-1);   // before the flames, the rain and the bursts: never over them
	// the plane (100 units, facing up) stood across the screen, CardDepth behind the plane of the play
	Card->SetWorldLocationAndRotation(UghShapes::ToWorld(UghShapes::ScreenWidth / 2.0, UghShapes::ScreenHeight / 2.0,
		CardDepth * UghShapes::UnitsPerPixel), FRotator(0, 0, 90));
	Card->SetWorldScale3D(FVector(UghShapes::ScreenWidth * UghShapes::UnitsPerPixel / UghShapes::ShapeSize,
		UghShapes::ScreenHeight * UghShapes::UnitsPerPixel / UghShapes::ShapeSize, 1));
	Material = UghShapes::Material(this, UghMaterials::CaveAir);
	Card->SetMaterial(0, Material);
	Card->SetupAttachment(RootComponent);
	Card->SetVisibility(false);
	Card->RegisterComponent();
	AddInstanceComponent(Card);
}

void AUghCaveAir::Build(const ugh_logic* InLogic, TConstArrayView<FUghDecoration> InDecorations, const FUghMood& Mood,
	const FVector& Sun, double WaterRow)
{
	Logic = InLogic;
	Decorations = TArray<FUghDecoration>(InDecorations);
	// the light's way on the screen: x right, y down (the world's z up)
	Way = FVector2D(Sun.X, -Sun.Z).GetSafeNormal();
	if (Way.Y <= 0.1)
	{
		Way = FVector2D(0, 1);
	}
	if (Material)
	{
		Material->SetVectorParameterValue(UghMaterials::DirectionParameter, FLinearColor(Way.X, Way.Y, 0));
		Material->SetVectorParameterValue(UghMaterials::HazeParameter, Mood.Shade * HazeShare);
		Material->SetVectorParameterValue(UghMaterials::ShaftParameter, Mood.SunColor * ShaftShare(Mood.Kind));
		Material->SetVectorParameterValue(UghMaterials::FireGlowParameter,
			FireColor * GlowShare(Mood.Kind) * Mood.FireLight);
	}
	FiresLit = -1;
	SetWater(WaterRow);
}

void AUghCaveAir::SetWater(double InSurface)
{
	if (Material)
	{
		Material->SetScalarParameterValue(UghMaterials::WaterLevelParameter, UghShapes::ToWorld(0, InSurface, 0).Z);
	}
	Surface = InSurface;
	const int32 Lit = UghCaveAir::Fires(Decorations, Surface).Num();
	if (Lit != FiresLit)
	{
		FiresLit = Lit;
		ShowMap();
	}
}

void AUghCaveAir::ShowMap()
{
	const double Started = FPlatformTime::Seconds();
	const TArray<FLinearColor> Map =
		UghCaveAir::Map(Logic, Way, Logic ? UghCaveAir::Fires(Decorations, Surface) : TArray<FUghAirFire>());
	if (!Card || !Material)
	{
		return;
	}
	if (Map.IsEmpty())
	{
		Card->SetVisibility(false);
		return;
	}
	// (the texture is sRGB: the values stored so that it gives them back as they are)
	TArray<FColor> Pixels;
	Pixels.Reserve(Map.Num());
	for (const FLinearColor& Cell : Map)
	{
		Pixels.Add(Cell.ToFColor(true));
	}
	MapTexture = UghTexture::Create(this, UghCaveAir::Width, UghCaveAir::Height, Pixels, false);
	Material->SetTextureParameterValue(UghMaterials::AirParameter, MapTexture);
	Card->SetVisibility(true);
	UE_LOG(LogTemp, Display, TEXT("UGH cave air: %d fires glowing, the map in %.1f ms"), FiresLit,
		(FPlatformTime::Seconds() - Started) * 1000);
}
