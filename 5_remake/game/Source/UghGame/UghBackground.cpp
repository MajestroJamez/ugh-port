#include "UghBackground.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"
#include "UghMaterials.h"
#include "UghRockMesh.h"
#include "UghShapes.h"

namespace
{
	const FLinearColor WoodColor(0.25f, 0.13f, 0.06f);

	/** The wooden box around the screen: its boards are this wide (pixels) and reach from this depth (units). */
	constexpr double FrameWidth = 24, FrameFront = -60;
	/** The water reaches from just behind the rock's cut face to the back of the rock, units. */
	constexpr double WaterFront = FUghRockMesh::CutDepth + 1;
}

AUghBackground::AUghBackground()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AUghBackground::BeginPlay()
{
	Super::BeginPlay();
	Rock = NewObject<UProceduralMeshComponent>(this);
	Rock->SetupAttachment(RootComponent);
	Rock->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Rock->RegisterComponent();
	AddInstanceComponent(Rock);
	RockMaterial = UghShapes::Material(this, UghMaterials::Rock);

	Frame = UghShapes::AddShapes(this, UghShapes::EShape::Cube, UghShapes::Clay(this, WoodColor));
	Water = UghShapes::AddShapes(this, UghShapes::EShape::Cube, UghShapes::Material(this, UghMaterials::Water));

	// the boards around the screen, from in front of the play to the back of the rock
	constexpr double Width = UghShapes::ScreenWidth, Height = UghShapes::ScreenHeight;
	constexpr double Depth = FUghRockMesh::BackDepth - FrameFront, Middle = (FUghRockMesh::BackDepth + FrameFront) / 2;
	UghShapes::SetShapes(Frame, {
		UghShapes::Box(-FrameWidth, -FrameWidth, Width + 2 * FrameWidth, FrameWidth, Middle, Depth),
		UghShapes::Box(-FrameWidth, Height, Width + 2 * FrameWidth, FrameWidth, Middle, Depth),
		UghShapes::Box(-FrameWidth, 0, FrameWidth, Height, Middle, Depth),
		UghShapes::Box(Width, 0, FrameWidth, Height, Middle, Depth) });
}

void AUghBackground::Build(const FUghRockMesh& Mesh, UTexture2D* Art)
{
	Rock->ClearAllMeshSections();
	if (Mesh.Vertices.IsEmpty())
	{
		return;
	}
	Rock->CreateMeshSection(0, Mesh.Vertices, Mesh.Triangles, Mesh.Normals, Mesh.UVs, {}, {}, false);
	RockArt = Art;
	RockMaterial->SetTextureParameterValue(UghMaterials::ArtParameter, Art);
	Rock->SetMaterial(0, RockMaterial);
}

void AUghBackground::SetWater(double Surface)
{
	TArray<FTransform> Boxes;
	if (Surface < UghShapes::ScreenHeight)
	{
		const double Top = FMath::Max(Surface, 0.0);
		constexpr double Depth = FUghRockMesh::BackDepth - WaterFront;
		Boxes.Add(UghShapes::Box(0, Top, UghShapes::ScreenWidth, UghShapes::ScreenHeight - Top,
			(FUghRockMesh::BackDepth + WaterFront) / 2, Depth));
	}
	UghShapes::SetShapes(Water, Boxes);
}
