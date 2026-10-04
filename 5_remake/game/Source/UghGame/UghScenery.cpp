#include "UghScenery.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UghAssets.h"
#include "UghShapes.h"

namespace
{
	const FLinearColor TrunkColor(0.25f, 0.13f, 0.06f);
	const FLinearColor CrownColor(0.1f, 0.3f, 0.06f);
	const FLinearColor StoneColor(0.3f, 0.29f, 0.27f);

	/** The clay palm: its trunk this thick (pixels), its crown this part of its height. */
	constexpr double TrunkWidth = 2, CrownPart = 0.35;
	/** A clay stone is a ball sunk this part of its height into its ledge. */
	constexpr double StoneSunk = 0.3;
	/** A model's foot sinks this deep into its ledge (pixels), so that no gap shows under it. */
	constexpr double ModelSunk = 0.5;

	/** The box of pixels Left, Top, Width x Height, as deep as wide, its middle `Depth` units deep. */
	FTransform Box(double Left, double Top, double Width, double Height, double Depth)
	{
		return UghShapes::Box(Left, Top, Width, Height, Depth, Width * UghShapes::UnitsPerPixel);
	}
}

AUghScenery::AUghScenery()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AUghScenery::BeginPlay()
{
	Super::BeginPlay();
	Palms.Append(UghAssets::Meshes({ UghAssets::Palm }));
	Rocks.Append(UghAssets::Meshes(UghAssets::Rocks));
	Trunks = UghShapes::AddShapes(this, UghShapes::EShape::Cylinder, UghShapes::Clay(this, TrunkColor));
	Crowns = UghShapes::AddShapes(this, UghShapes::EShape::Sphere, UghShapes::Clay(this, CrownColor));
	Stones = UghShapes::AddShapes(this, UghShapes::EShape::Sphere, UghShapes::Clay(this, StoneColor));
}

void AUghScenery::Show(const TArray<FUghDecoration>& Decorations)
{
	for (UStaticMeshComponent* Model : Models)
	{
		Model->DestroyComponent();
	}
	Models.Reset();
	TArray<FTransform> TrunkBoxes, CrownBoxes, StoneBoxes;
	for (const FUghDecoration& Decoration : Decorations)
	{
		const bool bPalm = Decoration.Kind == FUghDecoration::EKind::Palm;
		const TArray<TObjectPtr<UStaticMesh>>& Meshes = bPalm ? Palms : Rocks;
		const double X = Decoration.X, Y = Decoration.Y, Width = Decoration.Width, Height = Decoration.Height;
		if (!Meshes.IsEmpty())
		{
			AddModel(Decoration, Meshes);
		}
		else if (bPalm)
		{
			const double Trunk = Height * (1 - CrownPart);
			TrunkBoxes.Add(Box(X - TrunkWidth / 2, Y - Trunk, TrunkWidth, Trunk, Decoration.Depth));
			CrownBoxes.Add(Box(X - Width / 2, Y - Height, Width, Height * CrownPart, Decoration.Depth));
		}
		else
		{
			StoneBoxes.Add(Box(X - Width / 2, Y - Height * (1 - StoneSunk), Width, Height, Decoration.Depth));
		}
	}
	UghShapes::SetShapes(Trunks, TrunkBoxes);
	UghShapes::SetShapes(Crowns, CrownBoxes);
	UghShapes::SetShapes(Stones, StoneBoxes);
}

void AUghScenery::AddModel(const FUghDecoration& Decoration, const TArray<TObjectPtr<UStaticMesh>>& Meshes)
{
	UStaticMesh* Mesh = Meshes[Decoration.Variant % Meshes.Num()];
	const FBox Bounds = Mesh->GetBoundingBox();
	const FVector Size = Bounds.GetSize();
	// turned, its bounding box reaches this far across the screen and into the depth
	const double Turn = FMath::DegreesToRadians(Decoration.Yaw);
	const double Cos = FMath::Abs(FMath::Cos(Turn)), Sin = FMath::Abs(FMath::Sin(Turn));
	const double Across = FMath::Max(Cos * Size.X + Sin * Size.Y, Sin * Size.X + Cos * Size.Y);
	const double Scale = UghShapes::UnitsPerPixel *
		FMath::Min(Decoration.Height / FMath::Max(Size.Z, 1.0), Decoration.Width / FMath::Max(Across, 1.0));
	const FQuat Rotation(FVector::UpVector, Turn);
	const FVector Foot(Bounds.GetCenter().X, Bounds.GetCenter().Y, Bounds.Min.Z);
	const FVector Location = UghShapes::ToWorld(Decoration.X, Decoration.Y + ModelSunk, Decoration.Depth) -
		Rotation.RotateVector(Foot * Scale);

	UStaticMeshComponent* Model = NewObject<UStaticMeshComponent>(this);
	Model->SetMobility(EComponentMobility::Movable);
	Model->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Model->SetStaticMesh(Mesh);
	Model->SetupAttachment(RootComponent);
	Model->SetWorldTransform(FTransform(Rotation, Location, FVector(Scale)));
	Model->RegisterComponent();
	AddInstanceComponent(Model);
	Models.Add(Model);
}
