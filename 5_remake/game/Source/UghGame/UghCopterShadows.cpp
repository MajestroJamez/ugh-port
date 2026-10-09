#include "UghCopterShadows.h"

#include "Components/DecalComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UghBetween.h"
#include "UghMaterials.h"
#include "UghShapes.h"

namespace
{
	/** The shadow's half width: the body's half at the surface, this much wider at FadeHeight. */
	constexpr double Spread = 0.6;
}

FBox2D UghCopterShadow::BodyAt(const FVector2D& Corner)
{
	return FBox2D(Corner + FVector2D(UghShapes::CopterBodyLeft, 0),
		Corner + FVector2D(UghShapes::CopterBodyRight + 1, UghShapes::CopterBodyHeight));
}

TOptional<FUghCopterShadow> UghCopterShadow::Under(const ugh_logic* Logic, const FBox2D& Body, double WaterRow)
{
	if (!Logic)
	{
		return {};
	}
	const double Quarter = (Body.Max.X - Body.Min.X) / 4;
	const int32 Left = FMath::Max(FMath::FloorToInt32(Body.Min.X + Quarter), 0);
	const int32 Right = FMath::Min(FMath::FloorToInt32(Body.Max.X - Quarter - 1e-6), UghShapes::ScreenWidth - 1);
	const int32 Top = FMath::Max(FMath::FloorToInt32(Body.Max.Y), 0);
	TOptional<int32> Surface;
	for (int32 Column = Left; Column <= Right; ++Column)
	{
		for (int32 Row = Top; Row < UghShapes::ScreenHeight && (!Surface || Row < *Surface); ++Row)
		{
			if (ugh_logic_solid(Logic, Column, Row))
			{
				Surface = Row;
				break;
			}
		}
	}
	if (!Surface || *Surface > WaterRow)
	{
		return {};
	}
	FUghCopterShadow Shadow;
	Shadow.X = Body.GetCenter().X;
	Shadow.Surface = *Surface;
	Shadow.Height = FMath::Max(*Surface - Body.Max.Y, 0.0);
	if (Shadow.Height >= FadeHeight)
	{
		return {};
	}
	const double Up = Shadow.Height / FadeHeight;
	Shadow.Opacity = Darkest * FMath::Pow(1 - Up, 1.5);
	Shadow.HalfWidth = (Body.Max.X - Body.Min.X) / 2 * (1 + Spread * Up);
	return Shadow;
}

FTransform UghCopterShadow::Place(const FUghCopterShadow& Shadow)
{
	// the decal projects along its x: down; its y the depths, its z across the screen
	const FVector Middle = UghShapes::ToWorld(Shadow.X, Shadow.Surface - (Above - Below) / 2, 0);
	return FTransform(FRotator(-90, 0, 0), Middle, FVector((Above + Below) / 2, HalfDepth, Shadow.HalfWidth) *
		UghShapes::UnitsPerPixel);
}

AUghCopterShadows::AUghCopterShadows()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AUghCopterShadows::Show(const ugh_logic* Logic, const ugh_logic_view& Previous, const ugh_logic_view& Current,
	double Alpha)
{
	const bool bPlay = Current.phase == UGH_LOGIC_PHASE_PLAY && Current.level_id >= 0;
	const ugh_logic_view& From = UghBetween::From(Previous, Current);
	const double WaterRow = double(Current.water_level) / UghShapes::Subpixels;
	for (int32 Player = 0; Player < int32(UE_ARRAY_COUNT(Current.copters)); ++Player)
	{
		TOptional<FUghCopterShadow> Shadow;
		if (bPlay && Player < Current.copter_count)
		{
			const ugh_logic_copter& To = Current.copters[Player];
			const ugh_logic_copter& Was = Player < From.copter_count ? From.copters[Player] : To;
			Shadow = UghCopterShadow::Under(Logic,
				UghCopterShadow::BodyAt(UghBetween::Position(Was.x, Was.y, To.x, To.y, Alpha)), WaterRow);
		}
		if (Shadow && (!Decals.IsValidIndex(Player) || !Decals[Player]))
		{
			UMaterialInstanceDynamic* Material = UghShapes::Material(this, UghMaterials::CopterShadow);
			UDecalComponent* Decal = NewObject<UDecalComponent>(this);
			Decal->DecalSize = FVector::OneVector;   // the transform's scale is its size
			Decal->SetDecalMaterial(Material);
			Decal->SetupAttachment(RootComponent);
			Decal->RegisterComponent();
			AddInstanceComponent(Decal);
			Decals.SetNum(FMath::Max(Decals.Num(), Player + 1));
			Materials.SetNum(Decals.Num());
			Decals[Player] = Decal;
			Materials[Player] = Material;
		}
		UDecalComponent* Decal = Decals.IsValidIndex(Player) ? Decals[Player].Get() : nullptr;
		if (!Decal)
		{
			continue;
		}
		Decal->SetVisibility(Shadow.IsSet());
		if (Shadow)
		{
			Decal->SetWorldTransform(UghCopterShadow::Place(*Shadow));
			Materials[Player]->SetScalarParameterValue(UghMaterials::OpacityParameter, Shadow->Opacity);
		}
	}
}
