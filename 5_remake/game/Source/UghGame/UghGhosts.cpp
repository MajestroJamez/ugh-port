#include "UghGhosts.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UghAssets.h"
#include "UghBetween.h"
#include "UghCopterModel.h"
#include "UghGhost.h"
#include "UghMaterials.h"
#include "UghShapes.h"

namespace
{
	/** The middle of the body across, in pixels from the copter's corner (as AUghCopters). */
	constexpr double CopterMiddle = (UghShapes::CopterBodyLeft + UghShapes::CopterBodyRight + 1) / 2.0;
	/** Pale blue, see-through. */
	const FLinearColor GhostColor(0.5f, 0.82f, 1.f);
	constexpr float GhostOpacity = 0.5f;

	/**
	 * A part: movable, no collision, no shadow, nothing of it in Lumen, the distance fields or the ray traced scene. Not
	 * drawn by Nanite (its fallback mesh instead): Nanite draws no translucency, the material would be invalid there.
	 */
	UStaticMeshComponent* AddPart(AActor* Owner, UStaticMesh* Mesh, USceneComponent* Parent, const FVector& At,
		UMaterialInterface* Material)
	{
		UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(Owner);
		Part->bDisallowNanite = true;
		Part->SetStaticMesh(Mesh);
		Part->SetMobility(EComponentMobility::Movable);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCastShadow(false);
		Part->bAffectDistanceFieldLighting = false;
		Part->bVisibleInRayTracing = false;
		Part->bVisibleInReflectionCaptures = false;
		Part->bReceivesDecals = false;
		Part->SetAffectDynamicIndirectLighting(false);
		Part->SetAffectIndirectLightingWhileHidden(false);
		for (int32 Slot = 0; Slot < Part->GetNumMaterials(); ++Slot)
		{
			Part->SetMaterial(Slot, Material);
		}
		Part->SetupAttachment(Parent);
		Part->SetRelativeLocation(At);
		Part->SetVisibility(false);
		Part->RegisterComponent();
		Owner->AddInstanceComponent(Part);
		return Part;
	}
}

AUghGhosts::AUghGhosts()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AUghGhosts::BeginPlay()
{
	Super::BeginPlay();
	Material = UghShapes::Material(this, UghMaterials::Ghost);
	Material->SetVectorParameterValue(UghMaterials::ColorParameter, GhostColor);
	Material->SetScalarParameterValue(UghMaterials::OpacityParameter, GhostOpacity);
	for (int32 Player = 0; Player < UE_ARRAY_COUNT(UghCopterModel::Bodies); ++Player)
	{
		UStaticMesh* Body = UghAssets::Mesh(UghAssets::Copter, UghCopterModel::Bodies[Player]);
		UStaticMesh* Rotor = UghAssets::Mesh(UghAssets::Copter, UghCopterModel::Rotors[Player]);
		if (!Body || !Rotor)
		{
			UE_LOG(LogTemp, Display, TEXT("UGH no ghost: the copter's models are not imported"));
			Bodies.Reset();
			Rotors.Reset();
			return;
		}
		Bodies.Add(AddPart(this, Body, RootComponent, FVector::ZeroVector, Material));
		Rotors.Add(AddPart(this, Rotor, Bodies.Last(), UghCopterModel::RotorHub, Material));
		Spins.AddDefaulted();
	}
}

void AUghGhosts::Show(const FUghGhost* Ghost, double Alpha, double Seconds)
{
	const bool bShown = Ghost && Ghost->IsShown();
	for (int32 Player = 0; Player < Bodies.Num(); ++Player)
	{
		const bool bThere = bShown && Player < Ghost->GetCurrent().copter_count;
		Bodies[Player]->SetVisibility(bThere, true);
		if (!bThere)
		{
			continue;
		}
		const ugh_logic_view& From = UghBetween::From(Ghost->GetPrevious(), Ghost->GetCurrent());
		const ugh_logic_copter& To = Ghost->GetCurrent().copters[Player];
		const ugh_logic_copter& Was = Player < From.copter_count ? From.copters[Player] : To;
		const FVector2D At = UghBetween::Position(Was.x, Was.y, To.x, To.y, Alpha);
		Bodies[Player]->SetWorldLocationAndRotation(
			UghShapes::ToWorld(At.X + CopterMiddle, At.Y + UghShapes::CopterBodyHeight, 0), FQuat::Identity);
		Spins[Player].Update(To.rotor_sprite, Seconds);
		Rotors[Player]->SetRelativeRotation(FRotator(0, 360 * Spins[Player].RotorTurn(), 0));
	}
}

TOptional<FVector> AUghGhosts::GetShown(int32 Player) const
{
	return Bodies.IsValidIndex(Player) && Bodies[Player]->IsVisible() ? Bodies[Player]->GetComponentLocation()
		: TOptional<FVector>();
}
