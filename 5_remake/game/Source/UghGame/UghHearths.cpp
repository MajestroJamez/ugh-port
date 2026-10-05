#include "UghHearths.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UghAssets.h"
#include "UghElectricDreams.h"
#include "UghMaterials.h"
#include "UghShapes.h"

namespace
{
	/**
	 * A scanned hearth, parts of its width: its ring of stones (how many, how far out, how big), its logs (how many,
	 * how long, where their feet are, how steeply they lean together), its coals (how many, how far out, how big).
	 */
	constexpr int32 RingStones = 10, LogCount = 4, CoalCount = 9;
	constexpr double RingRadius = 0.42, StoneSize = 0.2, StoneSunk = 0.3;
	constexpr double LogLength = 0.62, LogFoot = 0.34, LogLean = 26;
	constexpr double CoalRadius = 0.2, CoalSize = 0.09;
	/** How bright the embers glow in the logs and the coals, how much more the logs' undersides. */
	constexpr float LogGlow = 0.8f, LogUnderside = 0.85f, CoalGlow = 2.f;

	/** Kenney's campfire in clay of these colours by its material slots: stone, wood, woodDark; clay logs. */
	const FLinearColor StoneColor(0.1f, 0.095f, 0.09f), WoodColor(0.3f, 0.17f, 0.08f), CharredColor(0.1f, 0.05f, 0.03f);
	const FLinearColor LogColor(0.2f, 0.1f, 0.04f);
	/** Two clay logs this long and thick, of the width. */
	constexpr double ClayLogLength = 0.9, ClayLogThick = 0.18;

	UMaterialInstanceDynamic* Embers(UObject* Outer, float Glow, float Underside)
	{
		UMaterialInstanceDynamic* Material = UghShapes::Material(Outer, UghMaterials::Embers);
		Material->SetScalarParameterValue(UghMaterials::GlowParameter, Glow);
		Material->SetScalarParameterValue(UghMaterials::UndersideParameter, Underside);
		return Material;
	}
}

void FUghHearths::Make(AActor* Owner)
{
	const TArray<UStaticMesh*> ScannedStones = UghElectricDreams::Meshes(UghElectricDreams::FireStones);
	const TArray<UStaticMesh*> ScannedLogs = UghElectricDreams::Meshes(UghElectricDreams::FireLogs);
	if (!ScannedStones.IsEmpty() && !ScannedLogs.IsEmpty())
	{
		UMaterialInterface* Charcoal = Embers(Owner, LogGlow, LogUnderside), * Coal = Embers(Owner, CoalGlow, 0);
		for (UStaticMesh* Mesh : ScannedStones)
		{
			Stones.Add(NewPart(Owner, Mesh, nullptr));
			Coals.Add(NewPart(Owner, Mesh, Coal));
		}
		for (UStaticMesh* Mesh : ScannedLogs)
		{
			Logs.Add(NewPart(Owner, Mesh, Charcoal));
		}
		return;
	}
	for (UStaticMesh* Mesh : UghAssets::Meshes({ UghAssets::Campfire }))
	{
		const int32 Part = NewPart(Owner, Mesh, nullptr);
		for (int32 Slot = 0; Slot < Mesh->GetStaticMaterials().Num(); ++Slot)
		{
			const FString Name = Mesh->GetStaticMaterials()[Slot].MaterialSlotName.ToString();
			Parts[Part].Component->SetMaterial(Slot, UghShapes::Clay(Owner, Name.Contains(TEXT("stone")) ? StoneColor
				: Name.Contains(TEXT("Dark")) ? CharredColor : WoodColor));
		}
		Kenney.Add(Part);
	}
	if (Kenney.IsEmpty())
	{
		ClayLogs = Parts.Add({ UghShapes::AddShapes(Owner, UghShapes::EShape::Cylinder, UghShapes::Clay(Owner, LogColor)),
			FBox(ForceInit), {} });
	}
}

int32 FUghHearths::NewPart(AActor* Owner, UStaticMesh* Mesh, UMaterialInterface* Material)
{
	UInstancedStaticMeshComponent* Component = UghShapes::AddShapes(Owner, UghShapes::EShape::Cube, nullptr);
	Component->SetStaticMesh(Mesh);
	for (int32 Slot = 0; Material && Slot < Mesh->GetStaticMaterials().Num(); ++Slot)
	{
		Component->SetMaterial(Slot, Material);
	}
	return Parts.Add({ Component, Mesh->GetBoundingBox(), {} });
}

void FUghHearths::Put(int32 Part, const FVector& At, const FQuat& Rotation, double Size)
{
	FPart& Into = Parts[Part];
	// its longest side along X first
	const FVector Extent = Into.Bounds.GetSize();
	const FQuat Along = Extent.X >= Extent.Y && Extent.X >= Extent.Z ? FQuat::Identity
		: Extent.Y >= Extent.Z ? FQuat(FVector::UpVector, UE_HALF_PI) : FQuat(FVector::YAxisVector, UE_HALF_PI);
	const double Scale = Size / FMath::Max(Extent.GetMax(), 1.0);
	const FQuat Turn = Rotation * Along;
	Into.Boxes.Add(FTransform(Turn, At - Turn.RotateVector(Into.Bounds.GetCenter() * Scale), FVector(Scale)));
}

double FUghHearths::Add(const FVector& Foot, double Width, double Yaw, int32 Seed)
{
	FRandomStream Random(Seed);
	if (!Logs.IsEmpty())
	{
		return AddScanned(Foot, Width, Yaw, Random);
	}
	return AddKenney(Foot, Width, Yaw);
}

double FUghHearths::AddScanned(const FVector& Foot, double Width, double Yaw, FRandomStream& Random)
{
	for (int32 Stone = 0; Stone < RingStones; ++Stone)
	{
		const double Angle = Yaw + 360.0 * (Stone + Random.FRandRange(-0.2, 0.2)) / RingStones;
		const FVector Out = FRotator(0, Angle, 0).Vector() * RingRadius * Width;
		const double Size = StoneSize * Width * Random.FRandRange(0.75, 1.2);
		Put(Stones[Random.RandHelper(Stones.Num())], Foot + Out + FVector(0, 0, Size * (0.5 - StoneSunk)),
			FQuat(FRotator(Random.FRandRange(-15, 15), Random.FRandRange(0, 360), Random.FRandRange(-15, 15))), Size);
	}
	for (int32 Coal = 0; Coal < CoalCount; ++Coal)
	{
		const FVector Out = FRotator(0, Random.FRandRange(0, 360), 0).Vector() * Random.FRandRange(0, CoalRadius) * Width;
		const double Size = CoalSize * Width * Random.FRandRange(0.6, 1.3);
		Put(Coals[Random.RandHelper(Coals.Num())], Foot + Out + FVector(0, 0, Size * 0.25), FQuat(FRotator(
			Random.FRandRange(0, 360), Random.FRandRange(0, 360), Random.FRandRange(0, 360))), Size);
	}
	// the logs lean together over the coals, their feet out at the ring
	double Top = 0;
	for (int32 Log = 0; Log < LogCount; ++Log)
	{
		const double Angle = Yaw + 45 + 360.0 * (Log + Random.FRandRange(-0.15, 0.15)) / LogCount;
		const double Length = LogLength * Width * Random.FRandRange(0.85, 1.1), Lean = LogLean + Random.FRandRange(-6, 6);
		const FVector In = -FRotator(0, Angle, 0).Vector();
		const FVector FootOfLog = Foot - In * LogFoot * Width;
		const FVector Way = (In * FMath::Cos(FMath::DegreesToRadians(Lean)) +
			FVector(0, 0, FMath::Sin(FMath::DegreesToRadians(Lean)))).GetSafeNormal();
		const FQuat Rotation = FRotationMatrix::MakeFromXZ(Way, FVector::UpVector).ToQuat() *
			FQuat(FVector::XAxisVector, Random.FRandRange(0, UE_TWO_PI));
		Put(Logs[Random.RandHelper(Logs.Num())], FootOfLog + Way * Length / 2, Rotation, Length);
		Top = FMath::Max(Top, Way.Z * Length);
	}
	return Top;
}

double FUghHearths::AddKenney(const FVector& Foot, double Width, double Yaw)
{
	const FQuat Turn(FVector::UpVector, FMath::DegreesToRadians(Yaw));
	if (ClayLogs != INDEX_NONE)
	{
		// two clay logs crossed: cylinders (along Z) laid down and turned both ways
		const double Thick = Width * ClayLogThick;
		const FVector Scale(Thick / UghShapes::ShapeSize, Thick / UghShapes::ShapeSize,
			Width * ClayLogLength / UghShapes::ShapeSize);
		const FVector Centre = Foot + FVector(0, 0, Thick / 2);
		Parts[ClayLogs].Boxes.Append({ FTransform(FRotator(90, Yaw + 30, 0), Centre, Scale),
			FTransform(FRotator(90, Yaw - 30, 0), Centre, Scale) });
		return Thick;
	}
	// the stones and logs filling the box's width, standing on its foot
	FBox Bounds(ForceInit);
	for (const int32 Part : Kenney)
	{
		Bounds += Parts[Part].Bounds;
	}
	const FVector Size = Bounds.GetSize();
	const double Scale = Width / FMath::Max(Size.X, Size.Y);
	const FVector Anchor(Bounds.GetCenter().X, Bounds.GetCenter().Y, Bounds.Min.Z);
	for (const int32 Part : Kenney)
	{
		Parts[Part].Boxes.Add(FTransform(Turn, Foot - Turn.RotateVector(Anchor * Scale), FVector(Scale)));
	}
	return Size.Z * Scale;
}

void FUghHearths::Show()
{
	for (FPart& Part : Parts)
	{
		UghShapes::SetShapes(Part.Component, Part.Boxes);
		Part.Boxes.Reset();
	}
}
