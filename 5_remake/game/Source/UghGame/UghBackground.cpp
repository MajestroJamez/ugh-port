#include "UghBackground.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UghAssets.h"
#include "UghMaterials.h"
#include "UghRockMesh.h"
#include "UghShapes.h"
#include "UghSprites.h"
#include "UghTexture.h"

namespace
{
	constexpr int32 Width = UghShapes::ScreenWidth, Height = UghShapes::ScreenHeight;

	/** The water reaches from just in front of the rock's face to its back wall, units. */
	constexpr double WaterFront = FUghRockMesh::FrontDepth - 1;
	/** The cliff takes the colours of the drawing blurred this much (pixels): its areas, not its pixels. */
	constexpr int32 ArtBlur = 2;
	/**
	 * The cliff around the rock's grid (above it and at its sides), which keeps the sun off the cave as the rock in the
	 * grid does: unseen, only its shadow (else the sun would light bands of the back wall past the grid's edges); this
	 * many pixels thick, reaching this far into the grid (the rock there is closed, its surface shadows only from inside
	 * the cave).
	 */
	constexpr double ShroudThickness = 200, ShroudOverlap = 12;

	/** The drawing blurred (a box of ArtBlur pixels around each). */
	TArray<FColor> Soften(const TArray<FColor>& Art)
	{
		TArray<FColor> Soft;
		Soft.SetNumUninitialized(Art.Num());
		for (int32 Y = 0; Y < Height; ++Y)
		{
			for (int32 X = 0; X < Width; ++X)
			{
				FLinearColor Sum = FLinearColor::Transparent;
				int32 Count = 0;
				for (int32 SY = FMath::Max(Y - ArtBlur, 0); SY <= FMath::Min(Y + ArtBlur, Height - 1); ++SY)
				{
					for (int32 SX = FMath::Max(X - ArtBlur, 0); SX <= FMath::Min(X + ArtBlur, Width - 1); ++SX)
					{
						Sum += FLinearColor(Art[SY * Width + SX]);
						++Count;
					}
				}
				Soft[Y * Width + X] = (Sum / Count).ToFColor(true);
			}
		}
		return Soft;
	}
}

AUghBackground::AUghBackground()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Static);   // the rock's (its shadows and ray tracing are cached)
}

void AUghBackground::BeginPlay()
{
	Super::BeginPlay();
	RockMaterial = MakeCliffMaterial();
	bCliff = RockMaterial != nullptr;
	if (!bCliff)
	{
		RockMaterial = UghShapes::Material(this, UghMaterials::Rock);
	}
	Water = UghShapes::AddShapes(this, UghShapes::EShape::Cube, UghShapes::Material(this, UghMaterials::Water));
	AddShroud();
}

void AUghBackground::AddShroud()
{
	UInstancedStaticMeshComponent* Shroud = UghShapes::AddShapes(this, UghShapes::EShape::Cube, nullptr);
	Shroud->SetHiddenInGame(true);
	Shroud->SetCastHiddenShadow(true);
	constexpr double Front = FUghRockMesh::FrontDepth, Back = FUghRockMesh::BackDepth, Thick = ShroudThickness;
	constexpr double Left = ShroudOverlap - FUghRockMesh::MarginX;
	constexpr double Right = Width + FUghRockMesh::MarginX - ShroudOverlap;
	constexpr double Top = ShroudOverlap - FUghRockMesh::MarginY, Bottom = Height + FUghRockMesh::MarginY;
	auto Box = [&](double X, double Y, double W, double H)
	{
		return UghShapes::Box(X, Y, W, H, (Front + Back) / 2, Back - Front);
	};
	UghShapes::SetShapes(Shroud, { Box(Left - Thick, Top - Thick, Right - Left + 2 * Thick, Thick),
		Box(Left - Thick, Top, Thick, Bottom - Top), Box(Right, Top, Thick, Bottom - Top) });
}

UMaterialInstanceDynamic* AUghBackground::MakeCliffMaterial()
{
	static_assert(UE_ARRAY_COUNT(UghMaterials::CliffLayers) == UE_ARRAY_COUNT(UghAssets::CliffSets));
	UMaterialInstanceDynamic* Cliff = nullptr;
	for (int32 Layer = 0; Layer < UE_ARRAY_COUNT(UghAssets::CliffSets); ++Layer)
	{
		const UMaterialInterface* Set = UghAssets::Material(UghAssets::CliffSets[Layer]);
		if (!Set)
		{
			return nullptr;
		}
		Cliff = Cliff ? Cliff : UghShapes::Material(this, UghMaterials::Cliff);
		for (const TCHAR* Map : UghMaterials::CliffMaps)
		{
			UTexture* Texture = nullptr;
			if (!Set->GetTextureParameterValue(FHashedMaterialParameterInfo(Map), Texture) || !Texture)
			{
				UE_LOG(LogTemp, Display, TEXT("UGH no %s in %s (build.ps1 -ForceImport): the drawing's colours instead"),
					Map, *Set->GetName());
				return nullptr;
			}
			Cliff->SetTextureParameterValue(FName(FString(UghMaterials::CliffLayers[Layer]) + Map), Texture);
		}
	}
	return Cliff;
}

void AUghBackground::Build(const FUghRockMesh& Mesh, const TArray<FColor>& Art, const TArray<FUghArtTile>& Signs,
	const FUghSprites& Sprites)
{
	if (Rock)
	{
		Rock->DestroyComponent();   // a static one: a new one for the new mesh
		Rock = nullptr;
	}
	UStaticMesh* RockMesh = Mesh.ToStaticMesh(this);
	ShowSigns(RockMesh && bCliff ? Signs : TArray<FUghArtTile>(), Sprites);   // the drawing on its own shows them
	if (!RockMesh)
	{
		return;
	}
	// the cliff takes the drawing's areas, the drawing's colours on their own its pixels
	RockArt = UghTexture::Create(this, Width, Height, bCliff ? Soften(Art) : Art, false);
	RockMaterial->SetTextureParameterValue(UghMaterials::ArtParameter, RockArt);
	Rock = NewObject<UStaticMeshComponent>(this);
	Rock->SetMobility(EComponentMobility::Static);
	Rock->SetStaticMesh(RockMesh);
	Rock->SetMaterial(0, RockMaterial);
	Rock->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Rock->SetupAttachment(RootComponent);
	Rock->RegisterComponent();
	AddInstanceComponent(Rock);
}

void AUghBackground::ShowSigns(const TArray<FUghArtTile>& Signs, const FUghSprites& Sprites)
{
	for (int32 I = 0; I < FMath::Max(Signs.Num(), SignCards.Num()); ++I)
	{
		if (I >= Signs.Num())
		{
			SignCards[I]->SetVisibility(false);
			continue;
		}
		if (I >= SignCards.Num())
		{
			SignCards.Add(UghShapes::AddCard(this));
		}
		const FUghArtTile& Sign = Signs[I];
		const FIntPoint Size = Sprites.Size(Sign.Sprite);
		TObjectPtr<UTexture2D>& Texture = SignTextures.FindOrAdd(Sign.Sprite);
		if (!Texture)
		{
			Texture = UghTexture::Create(this, Size.X, Size.Y, Sprites.Pixels(Sign.Sprite), true);
		}
		UghShapes::ShowCard(SignCards[I], Texture, Sign.At.X, Sign.At.Y, Size.X, Size.Y, SignDepth);
	}
}

void AUghBackground::SetWater(double Surface)
{
	TArray<FTransform> Boxes;
	if (Surface < Height)
	{
		// across the whole cliff, beyond the screen too
		const double Top = FMath::Max(Surface, double(-FUghRockMesh::MarginY));
		constexpr double Depth = FUghRockMesh::BackDepth - WaterFront;
		Boxes.Add(UghShapes::Box(-FUghRockMesh::MarginX, Top, Width + 2 * FUghRockMesh::MarginX,
			Height + FUghRockMesh::MarginY - Top, (FUghRockMesh::BackDepth + WaterFront) / 2, Depth));
	}
	UghShapes::SetShapes(Water, Boxes);
}
