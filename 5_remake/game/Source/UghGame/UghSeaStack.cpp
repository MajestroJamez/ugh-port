#include "UghSeaStack.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UghBackground.h"
#include "UghMaterials.h"
#include "UghRockMesh.h"
#include "UghScenery.h"
#include "UghShapes.h"
#include "UghStackField.h"
#include "UghTexture.h"

namespace
{
	/**
	 * What the cliff's material takes from the drawing for the whole stone (its warm rock: the limestone; neither
	 * green nor dark), and the colour of the plain rock without it.
	 */
	const FColor StoneColor(170, 150, 125);
	/**
	 * Its rock's surfaces (the first RockLayers of UghMaterials::CliffLayers: the limestone, the grey stone) this many
	 * times larger than the level's: no tiling seen far off.
	 */
	constexpr int32 RockLayers = 2;
	constexpr float RockScale = 2.5f;
}

AUghSeaStack::AUghSeaStack()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Static);   // its shadows and ray tracing are cached
	SetActorHiddenInGame(true);
}

void AUghSeaStack::Show(bool bShow)
{
	if (bShow == bShown)
	{
		return;
	}
	if (bShow && !Stone)
	{
		Make();
	}
	bShown = bShow;
	// hidden it leaves the scene (the shadows, the ray tracing, Lumen's cards): the play pays nothing for it
	for (AActor* Part : { static_cast<AActor*>(this), static_cast<AActor*>(Jungle.Get()) })
	{
		Part->SetActorHiddenInGame(!bShow);
		TInlineComponentArray<UPrimitiveComponent*> Components(Part);
		for (UPrimitiveComponent* Component : Components)
		{
			if (bShow && !Component->IsRegistered())
			{
				Component->RegisterComponent();
			}
			else if (!bShow && Component->IsRegistered())
			{
				Component->UnregisterComponent();
			}
		}
	}
}

void AUghSeaStack::SetWater(double Surface)
{
	if (bCliff && bShown)
	{
		StoneMaterial->SetScalarParameterValue(UghMaterials::WaterLevelParameter, UghShapes::ToWorld(0, Surface, 0).Z);
	}
}

void AUghSeaStack::Make()
{
	const double Started = FPlatformTime::Seconds();
	FUghStackField Field;
	Field.Build();
	TArray<FVector> Points;
	TArray<FUghNetQuad> Quads;
	UghSurfaceNets::Build(Field, Points, Quads);
	FUghRockMesh Mesh;
	Mesh.Build(Points, Quads, [](const FVector& Point) { return -FUghStackField::Gradient(Point).GetSafeNormal(); },
		&FUghStackField::Shade);

	StoneMaterial = AUghBackground::MakeCliffMaterial(this);
	bCliff = StoneMaterial != nullptr;
	if (!bCliff)
	{
		StoneMaterial = UghShapes::Material(this, UghMaterials::Rock);
	}
	for (int32 Layer = 0; Layer < RockLayers; ++Layer)
	{
		const FName Size(FString(UghMaterials::CliffLayers[Layer]) + UghMaterials::SizeParameter);
		float Metres = 0;
		if (bCliff && StoneMaterial->GetScalarParameterValue(Size, Metres))
		{
			StoneMaterial->SetScalarParameterValue(Size, Metres * RockScale);
		}
	}
	StoneArt = UghTexture::Create(this, 1, 1, { StoneColor }, false);
	StoneMaterial->SetTextureParameterValue(UghMaterials::ArtParameter, StoneArt);
	Stone = NewObject<UStaticMeshComponent>(this);
	Stone->SetMobility(EComponentMobility::Static);
	Stone->SetStaticMesh(Mesh.ToStaticMesh(this));
	Stone->SetMaterial(0, StoneMaterial);
	Stone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Stone->SetupAttachment(RootComponent);
	Stone->RegisterComponent();
	AddInstanceComponent(Stone);

	Jungle = GetWorld()->SpawnActor<AUghScenery>();
	const TArray<FUghDecoration> Plants = Field.Plants(Points);
	Jungle->Show(Plants);
	UE_LOG(LogTemp, Display, TEXT("UGH sea stack: %d vertices, %d triangles, %d plants in %.0f ms"), Mesh.Vertices.Num(),
		Mesh.Triangles.Num() / 3, Plants.Num(), (FPlatformTime::Seconds() - Started) * 1000);
}
