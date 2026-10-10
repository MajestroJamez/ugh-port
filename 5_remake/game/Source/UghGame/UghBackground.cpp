#include "UghBackground.h"

#include "Algo/Find.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MeshDescription.h"
#include "Misc/PackageName.h"
#include "UghAssets.h"
#include "UghElectricDreams.h"
#include "UghMaterials.h"
#include "UghRockMesh.h"
#include "UghShapes.h"
#include "UghTexture.h"
#include "UghTreeRoots.h"
#include "UghTurf.h"

namespace
{
	constexpr int32 Width = UghShapes::ScreenWidth, Height = UghShapes::ScreenHeight;

	/** The cliff takes the colours of the drawing blurred this much (pixels): its areas, not its pixels. */
	constexpr int32 ArtBlur = 8;
	/**
	 * The cliff around the rock's grid (above it and at its sides), which keeps the sun off the cave as the rock in the
	 * grid does: unseen, only its shadow (else the sun would light bands of the back wall past the grid's edges); this
	 * many pixels thick, reaching this far into the grid (the rock there is closed, its surface shadows only from inside
	 * the cave).
	 */
	constexpr double ShroudThickness = 200, ShroudOverlap = 12;
	/** The cliff's textures up to this size keep all their mips (larger ones would cost too much memory). */
	constexpr float ResidentMaxSize = 4096;

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
	// two of it: the shown rock's and the next level's (built while the shown one is seen, FinishBuild)
	for (TObjectPtr<UMaterialInstanceDynamic>& Material : RockMaterials)
	{
		Material = MakeCliffMaterial(this);
		bCliff = Material != nullptr;
		if (!bCliff)
		{
			Material = UghShapes::Material(this, UghMaterials::Rock);
		}
	}
	// the turf's blades (UghMakeAssets makes it; none before build.ps1)
	const FString Turf = UghMaterials::Turf;
	BladesMaterial = LoadObject<UMaterialInterface>(nullptr, *(Turf + TEXT(".") + FPackageName::GetShortName(Turf)));
	AddShroud();
}

void AUghBackground::AddShroud()
{
	UInstancedStaticMeshComponent* Shroud = UghShapes::AddShapes(this, UghShapes::EShape::Cube, nullptr);
	Shroud->SetHiddenInGame(true);
	Shroud->SetCastHiddenShadow(true);
	UghShapes::SetShapes(Shroud, ShroudBoxes());
}

TArray<FTransform> AUghBackground::ShroudBoxes()
{
	constexpr double Front = FUghRockMesh::FrontDepth, Back = FUghRockMesh::BackDepth, Thick = ShroudThickness;
	constexpr double Left = ShroudOverlap - FUghRockMesh::MarginX;
	constexpr double Right = Width + FUghRockMesh::MarginX - ShroudOverlap;
	constexpr double Top = ShroudOverlap - FUghRockMesh::MarginY, Bottom = Height + FUghRockMesh::MarginY;
	auto Box = [&](double X, double Y, double W, double H)
	{
		return UghShapes::Box(X, Y, W, H, (Front + Back) / 2, Back - Front);
	};
	return { Box(Left - Thick, Top - Thick, Right - Left + 2 * Thick, Thick), Box(Left - Thick, Top, Thick, Bottom - Top),
		Box(Right, Top, Thick, Bottom - Top) };
}

UMaterialInstanceDynamic* AUghBackground::MakeCliffMaterial(UObject* Outer)
{
	UMaterialInstanceDynamic* Cliff = UghShapes::Material(Outer, UghMaterials::Cliff);
	for (int32 Layer = 0; Layer < UE_ARRAY_COUNT(UghMaterials::CliffLayers); ++Layer)
	{
		if (!SetScannedLayer(Cliff, Layer) && !SetImportedLayer(Cliff, Layer))
		{
			return nullptr;
		}
	}
	// all their mips: the streamer would go by the mesh's UVs (the screen, the whole level on a texture) and keep only
	// the smallest, the rock blurred - the code maps them from the world, metres to a texture
	for (const TCHAR* Layer : UghMaterials::CliffLayers)
	{
		for (const TCHAR* Map : UghMaterials::CliffMaps)
		{
			UTexture* Texture = nullptr;
			if (Cliff->GetTextureParameterValue(FHashedMaterialParameterInfo(FName(FString(Layer) + Map)), Texture) &&
				Texture && Texture->GetSurfaceWidth() <= ResidentMaxSize)
			{
				Texture->bForceMiplevelsToBeResident = true;
			}
		}
	}
	return Cliff;
}

bool AUghBackground::SetScannedLayer(UMaterialInstanceDynamic* Cliff, int32 Layer)
{
	const FString Name = UghMaterials::CliffLayers[Layer];
	const UghElectricDreams::FCliffLayer* Scanned = Algo::FindByPredicate(UghElectricDreams::CliffLayers,
		[&](const UghElectricDreams::FCliffLayer& Each) { return Name == Each.Layer; });
	if (!Scanned)
	{
		return false;
	}
	// all of its maps or none: a missing one leaves the layer to its imported texture set
	TArray<TPair<FName, UTexture*>> Textures;
	const TPair<const TCHAR*, const TCHAR*> Maps[] = { { UghMaterials::BaseColorParameter, Scanned->BaseColor },
		{ UghMaterials::NormalParameter, Scanned->Normal }, { UghMaterials::RoughnessParameter, Scanned->Packed },
		{ UghMaterials::HeightParameter, Scanned->Packed } };
	for (const TPair<const TCHAR*, const TCHAR*>& Map : Maps)
	{
		UTexture* Texture = UghElectricDreams::Texture(Map.Value);
		if (!Texture)
		{
			return false;
		}
		Textures.Add({ FName(Name + Map.Key), Texture });
	}
	for (const TPair<FName, UTexture*>& Texture : Textures)
	{
		Cliff->SetTextureParameterValue(Texture.Key, Texture.Value);
	}
	Cliff->SetScalarParameterValue(FName(Name + UghMaterials::SizeParameter), Scanned->Size);
	Cliff->SetVectorParameterValue(FName(Name + UghMaterials::HeightMaskParameter), Scanned->HeightMask);
	return true;
}

bool AUghBackground::SetImportedLayer(UMaterialInstanceDynamic* Cliff, int32 Layer)
{
	static_assert(UE_ARRAY_COUNT(UghMaterials::CliffLayers) == UE_ARRAY_COUNT(UghAssets::CliffSets));
	const UMaterialInterface* Set = UghAssets::Material(UghAssets::CliffSets[Layer]);
	if (!Set)
	{
		return false;
	}
	for (const TCHAR* Map : UghMaterials::CliffMaps)
	{
		UTexture* Texture = nullptr;
		if (!Set->GetTextureParameterValue(FHashedMaterialParameterInfo(Map), Texture) || !Texture)
		{
			UE_LOG(LogTemp, Display, TEXT("UGH no %s in %s (build.ps1 -ForceImport): the drawing's colours instead"),
				Map, *Set->GetName());
			return false;
		}
		Cliff->SetTextureParameterValue(FName(FString(UghMaterials::CliffLayers[Layer]) + Map), Texture);
	}
	return true;
}

void AUghBackground::Build(const FUghRockMesh& Mesh, const TArray<FColor>& Art, const FUghTurf* Turf)
{
	BeginBuild(Art, Turf);
	TArray<FMeshDescription> Pieces;
	Mesh.Describe(1, Pieces);
	for (const FMeshDescription& Piece : Pieces)
	{
		AddRock(Piece);
	}
	if (Turf)
	{
		AddBlades(*Turf);
	}
	FinishBuild();
}

namespace
{
	/** Each of `Parts` destroyed (a static one: a new one for a new mesh), none left. */
	void DestroyParts(TArray<TObjectPtr<UStaticMeshComponent>>& Parts)
	{
		for (UStaticMeshComponent* Part : Parts)
		{
			if (Part)
			{
				Part->DestroyComponent();
			}
		}
		Parts.Reset();
	}
}

void AUghBackground::BeginBuild(const TArray<FColor>& Art, const FUghTurf* Turf)
{
	// (what an unfinished build left)
	DestroyParts(NewRocks);
	if (NewBlades)
	{
		NewBlades->DestroyComponent();
		NewBlades = nullptr;
	}
	if (NewRoots)
	{
		NewRoots->DestroyComponent();
		NewRoots = nullptr;
	}
	// the cliff takes the drawing's areas, the drawing's colours on their own its pixels
	TArray<FColor> Shown = bCliff ? Soften(Art) : Art;
	if (bCliff)
	{
		// the cliff's alpha: 255 - the paths trodden to the cave entrances (bare soil there)
		const bool bPaths = Turf && Turf->GetPaths().Num() == Shown.Num();
		for (int32 Index = 0; Index < Shown.Num(); ++Index)
		{
			Shown[Index].A = bPaths ? 255 - Turf->GetPaths()[Index] : 255;
		}
	}
	// into the material the shown rock does not have
	const int32 Built = 1 - ShownMaterial;
	RockArts[Built] = UghTexture::Create(this, Width, Height, Shown, false);
	RockMaterials[Built]->SetTextureParameterValue(UghMaterials::ArtParameter, RockArts[Built]);
}

void AUghBackground::AddRock(const FMeshDescription& Piece)
{
	UStaticMesh* RockMesh = FUghRockMesh::ToStaticMesh(this, Piece);
	if (!RockMesh)
	{
		return;
	}
	UStaticMeshComponent* Rock = NewObject<UStaticMeshComponent>(this);
	Rock->SetMobility(EComponentMobility::Static);
	Rock->SetStaticMesh(RockMesh);
	Rock->SetMaterial(0, RockMaterials[1 - ShownMaterial]);
	Rock->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Rock->SetVisibility(false);   // (until FinishBuild)
	Rock->SetupAttachment(RootComponent);
	Rock->RegisterComponent();
	AddInstanceComponent(Rock);
	NewRocks.Add(Rock);
}

void AUghBackground::AddBlades(const FUghTurf& Turf)
{
	// the grass hanging over the edges: static, no shadows, no ray tracing (cheap at any quality)
	UStaticMesh* BladesMesh = BladesMaterial ? Turf.GetBlades().ToStaticMesh(this) : nullptr;
	if (BladesMesh)
	{
		NewBlades = NewObject<UStaticMeshComponent>(this);
		NewBlades->SetMobility(EComponentMobility::Static);
		NewBlades->SetStaticMesh(BladesMesh);
		NewBlades->SetMaterial(0, BladesMaterial);
		NewBlades->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		NewBlades->SetCastShadow(false);
		NewBlades->bVisibleInRayTracing = false;
		NewBlades->bAffectDistanceFieldLighting = false;
		NewBlades->SetVisibility(false);   // (until FinishBuild)
		NewBlades->SetupAttachment(RootComponent);
		NewBlades->RegisterComponent();
		AddInstanceComponent(NewBlades);
	}
	UE_LOG(LogTemp, Display, TEXT("UGH turf: %d cards of grass%s"), Turf.CardCount(),
		NewBlades ? TEXT("") : TEXT(" (not shown: no M_UghTurf, run build.ps1)"));
}

void AUghBackground::AddRoots(const FUghTreeRoots& TreeRoots)
{
	// the tiling bark of the jungle tree's model (its slot roots: its own aerial roots and lianas)
	UMaterialInterface* Bark = nullptr;
	if (const USkeletalMesh* Tree = UghAssets::SkeletalMesh(UghAssets::TreeJungle))
	{
		for (const FSkeletalMaterial& Slot : Tree->GetMaterials())
		{
			Bark = Slot.MaterialSlotName == TEXT("roots") ? Slot.MaterialInterface.Get() : Bark;
		}
	}
	UStaticMesh* RootsMesh = Bark ? TreeRoots.GetMesh().ToStaticMesh(this) : nullptr;
	if (RootsMesh)
	{
		NewRoots = NewObject<UStaticMeshComponent>(this);
		NewRoots->SetMobility(EComponentMobility::Static);
		NewRoots->SetStaticMesh(RootsMesh);
		NewRoots->SetMaterial(0, Bark);
		NewRoots->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		NewRoots->bVisibleInRayTracing = false;
		NewRoots->bAffectDistanceFieldLighting = false;
		NewRoots->SetVisibility(false);   // (until FinishBuild)
		NewRoots->SetupAttachment(RootComponent);
		NewRoots->RegisterComponent();
		AddInstanceComponent(NewRoots);
	}
	UE_LOG(LogTemp, Display, TEXT("UGH tree roots: %d (%d triangles)%s"), TreeRoots.GetStrands().Num(),
		TreeRoots.GetMesh().Triangles.Num() / 3, RootsMesh || TreeRoots.GetStrands().IsEmpty() ? TEXT("") :
		TEXT(" (not shown: no jungle tree, fetch-assets.ps1 and build.ps1)"));
}

void AUghBackground::FinishBuild()
{
	DestroyParts(Rocks);
	if (Blades)
	{
		Blades->DestroyComponent();
	}
	if (Roots)
	{
		Roots->DestroyComponent();
	}
	Roots = NewRoots;
	NewRoots = nullptr;
	if (Roots)
	{
		Roots->SetVisibility(true);
	}
	Rocks = MoveTemp(NewRocks);
	NewRocks.Reset();
	Blades = NewBlades;
	NewBlades = nullptr;
	for (UStaticMeshComponent* Part : Rocks)
	{
		Part->SetVisibility(true);
	}
	if (Blades)
	{
		Blades->SetVisibility(true);
	}
	ShownMaterial = 1 - ShownMaterial;
}

void AUghBackground::SetWater(double Surface)
{
	if (bCliff)
	{
		for (UMaterialInstanceDynamic* Material : RockMaterials)
		{
			Material->SetScalarParameterValue(UghMaterials::WaterLevelParameter, UghShapes::ToWorld(0, Surface, 0).Z);
		}
	}
}
