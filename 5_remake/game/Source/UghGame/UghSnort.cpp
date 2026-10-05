#include "UghSnort.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/RandomStream.h"
#include "UghMaterials.h"
#include "UghMeshes.h"
#include "UghShapes.h"

namespace
{
	/** The quads are moved by the material, far from where they are. */
	constexpr float PuffReach = 400.f;
	/** How much the snort rises over the ground it blows along. */
	constexpr double Lift = 0.12;
}

void FUghSnort::Show(AActor* Owner, USkeletalMeshComponent* Model, bool bBlowing, double Phase)
{
	if (Model->GetBoneIndex(Nostrils) == INDEX_NONE)
	{
		Hide();
		return;
	}
	if (!Puff)
	{
		TArray<UghMeshes::FQuad> Quads;
		FRandomStream Random(Puffs);
		for (int32 Each = 0; Each < Puffs; ++Each)
		{
			Quads.Add({ FVector::ZeroVector, FVector2D(1), FVector2D(Random.FRand(), Random.FRand()) });
		}
		Material = UghShapes::Material(Owner, UghMaterials::Puff);
		Puff = UghMeshes::NewPart<UStaticMeshComponent>(Owner, Material);
		Puff->SetStaticMesh(UghMeshes::Quads(Owner, Quads));
		Puff->SetBoundsScale(PuffReach);
		Puff->SetVisibleInRayTracing(false);
		Puff->AttachToComponent(Model, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Nostrils);
	}
	const double Blow = bBlowing && Phase >= BreathIn ? (Phase - BreathIn) / (1 - BreathIn) : 0;
	// the nostrils' way from the head, along the ground (its head rests on it): a little up rather than into it
	FVector Way = (Model->GetSocketLocation(Nostrils) - Model->GetSocketLocation(Head)).GetSafeNormal();
	Way.Z = FMath::Max(Way.Z, 0.0) + Lift;
	Way.Normalize();
	Material->SetScalarParameterValue(UghMaterials::BlowParameter, float(Blow));
	Material->SetVectorParameterValue(UghMaterials::DirectionParameter, FLinearColor(Way));
	Puff->SetVisibility(Blow > 0);
}

void FUghSnort::Hide()
{
	if (Puff)
	{
		Puff->SetVisibility(false);
	}
}
