#include "UghWater.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UghBetween.h"
#include "UghMaterials.h"
#include "UghShapes.h"
#include "UghSprites.h"

namespace
{
	constexpr int32 Width = UghShapes::ScreenWidth, Height = UghShapes::ScreenHeight;

	/** How much a swimmer and a copter on the water stir it. */
	constexpr float SwimmerStir = 1.f, CopterStir = 1.5f;
	static_assert(UghWater::MaxRings == UE_ARRAY_COUNT(UghMaterials::RingParameters));
}

double UghWater::Surface(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha)
{
	const ugh_logic_view& From = UghBetween::From(Previous, Current);
	return FMath::Lerp(double(From.water_level), double(Current.water_level), Alpha) / UghShapes::Subpixels;
}

TOptional<FTransform> UghWater::Box(double Surface, bool bOpenSea)
{
	if (Surface >= Height)
	{
		return {};
	}
	const double Top = FMath::Max(Surface, double(-FUghRockMesh::MarginY));
	if (bOpenSea)
	{
		constexpr double Around = OpenSea / UghShapes::UnitsPerPixel;
		return UghShapes::Box(Width / 2.0 - Around, Top, 2 * Around, Height + OpenSeaDepth - Top, 0, 2 * OpenSea);
	}
	constexpr double Back = FUghRockMesh::BackDepth;
	return UghShapes::Box(-Reach, Top, Width + 2 * Reach,
		Height + FUghRockMesh::MarginY - Top, (Back + Front) / 2, Back - Front);
}

TArray<FVector4> UghWater::Rings(const ugh_logic_view& View, const FUghSprites& Sprites, double Surface)
{
	TArray<FVector4> Rings;
	auto Stir = [&](double Left, double Right, double Top, double Bottom, float Strength)
	{
		if (Rings.Num() < MaxRings && Top < Surface && Surface <= Bottom + 1)
		{
			Rings.Add(FVector4(UghShapes::ToWorld((Left + Right) / 2, Surface, 0), Strength));
		}
	};
	for (int32 I = 0; I < View.copter_count; ++I)
	{
		const FVector2D At = UghBetween::Pixels(View.copters[I].x, View.copters[I].y);
		Stir(At.X + UghShapes::CopterBodyLeft, At.X + UghShapes::CopterBodyRight + 1, At.Y,
			At.Y + UghShapes::CopterBodyHeight, CopterStir);
	}
	for (int32 I = 0; I < View.entity_count; ++I)
	{
		const ugh_logic_entity& Entity = View.entities[I];
		if (Entity.kind == UGH_LOGIC_ENTITY_PASSENGER && Entity.sprite >= 0)
		{
			const FVector2D At = UghBetween::Pixels(Entity.x, Entity.y);
			const FIntPoint Size = Sprites.Size(Entity.sprite);
			Stir(At.X, At.X + Size.X, At.Y, At.Y + Size.Y, SwimmerStir);
		}
	}
	return Rings;
}

AUghWater::AUghWater()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AUghWater::BeginPlay()
{
	Super::BeginPlay();
	Material = UghShapes::Material(this, UghMaterials::Water);
	Water = UghShapes::AddShapes(this, UghShapes::EShape::Cube, Material);
	Water->SetCastShadow(false);   // the light under it is the water's to dim
}

void AUghWater::Show(double Surface, const TArray<FVector4>& Rings, bool bOpenSea)
{
	const TOptional<FTransform> Box = UghWater::Box(Surface, bOpenSea);
	UghShapes::SetShapes(Water, Box ? TArray<FTransform>{ *Box } : TArray<FTransform>());
	Material->SetScalarParameterValue(UghMaterials::WaterLevelParameter, UghShapes::ToWorld(0, Surface, 0).Z);
	for (int32 I = 0; I < UghWater::MaxRings; ++I)
	{
		const FVector4 Ring = I < Rings.Num() ? Rings[I] : FVector4(0, 0, 0, 0);
		Material->SetVectorParameterValue(UghMaterials::RingParameters[I], FLinearColor(Ring.X, Ring.Y, Ring.Z, Ring.W));
	}
}

void AUghWater::SetWeather(int32 Wind, const FVector& Sun, float Caustics)
{
	Material->SetScalarParameterValue(UghMaterials::RainParameter, Wind != 0 ? 1.f : 0.f);
	Material->SetScalarParameterValue(UghMaterials::WindParameter, Wind);
	Material->SetVectorParameterValue(UghMaterials::SunParameter, FLinearColor(Sun.X, Sun.Y, Sun.Z));
	Material->SetScalarParameterValue(UghMaterials::CausticsParameter, Caustics);
}

double AUghWater::SurfaceZ() const
{
	if (Water->GetInstanceCount() == 0)
	{
		return -UE_BIG_NUMBER;
	}
	FTransform Box;
	Water->GetInstanceTransform(0, Box, true);
	return Box.GetLocation().Z + Box.GetScale3D().Z * UghShapes::ShapeSize / 2;
}
