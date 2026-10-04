#include "UghMetaHumans.h"

#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Actor.h"
#include "GroomComponent.h"
#include "Misc/PackageName.h"

namespace
{
	/** The blueprint's components of the body and the face (by their names). */
	const FName BodyName(TEXT("Body")), FaceName(TEXT("Face"));
	/**
	 * The grooms' level of detail: their helmets (the quality Low assembles strands, too slow, and helmets; the game's
	 * figures are small).
	 */
	constexpr int32 GroomLOD = 5;

	/** The asset of type TAsset at `Path` (a package path, its object named as the package or `Object`). */
	template <typename TAsset>
	TAsset* Find(const FString& Path, const FString& Object = FString())
	{
		const FString Name = Object.IsEmpty() ? FPackageName::GetShortName(Path) : Object;
		return FPackageName::DoesPackageExist(Path) ? LoadObject<TAsset>(nullptr, *(Path + TEXT(".") + Name)) : nullptr;
	}

	/** The skeletal mesh of the blueprint's component `Name`; none without it. */
	USkeletalMesh* MeshOf(const UBlueprintGeneratedClass* Blueprint, FName Name)
	{
		const USCS_Node* Node = Blueprint->SimpleConstructionScript->FindSCSNode(Name);
		const USkeletalMeshComponent* Template = Node ? Cast<USkeletalMeshComponent>(Node->ComponentTemplate) : nullptr;
		return Template ? Template->GetSkeletalMeshAsset() : nullptr;
	}
}

bool FUghMetaHuman::Load(const TCHAR* Name, TConstArrayView<const TCHAR*> ActionNames)
{
	const FString Folder = FString(UghMetaHumans::Root) / Name;
	Blueprint = Find<UBlueprintGeneratedClass>(Folder / FString::Printf(TEXT("BP_%s"), Name),
		FString::Printf(TEXT("BP_%s_C"), Name));
	USkeletalMesh* Face = Blueprint ? MeshOf(Blueprint, FaceName) : nullptr;
	TArray<UAnimSequence*> Actions;
	for (const TCHAR* Action : ActionNames)
	{
		Actions.Add(Find<UAnimSequence>(Folder / FString::Printf(TEXT("Actions/AS_%s"), Action)));
	}
	if (!Face || !Body.Load(MeshOf(Blueprint, BodyName), Actions))
	{
		UE_LOG(LogTemp, Display, TEXT("UGH no MetaHuman %s or its actions (metahumans.ps1)"), *Folder);
		Body = FUghRig();
		return false;
	}
	OwnHeight = Face->GetImportedBounds().GetBox().Max.Z;
	return true;
}

void FUghMetaHuman::Add(AActor* Owner, USceneComponent* Holder, double Height, UMaterialInterface* Garment) const
{
	FAdding Adding{ Owner, Height / OwnHeight, Garment, nullptr };
	for (const USCS_Node* Node : Blueprint->SimpleConstructionScript->GetRootNodes())
	{
		AddNode(Adding, Node, Holder);
	}
	TArray<USceneComponent*> Parts;
	Holder->GetChildrenComponents(true, Parts);
	for (USceneComponent* Part : Parts)
	{
		USkeletalMeshComponent* Follower = Cast<USkeletalMeshComponent>(Part);
		if (Follower && Follower != Adding.Leader)
		{
			Follower->SetLeaderPoseComponent(Adding.Leader);
		}
	}
}

/**
 * The blueprint's component of `Node` and its children under `Parent`: the skeletal meshes and the grooms (the others,
 * the root, LOD sync, MetaHuman component, are left out: their children go to `Parent`). The body is the leader; a
 * mesh but the body and the face is the outfit.
 */
void FUghMetaHuman::AddNode(FAdding& Adding, const USCS_Node* Node, USceneComponent* Parent) const
{
	AActor* Owner = Adding.Owner;
	USceneComponent* Template = Cast<USceneComponent>(Node->ComponentTemplate);
	USceneComponent* Part = Parent;
	if (Template && (Template->IsA<USkeletalMeshComponent>() || Template->IsA<UGroomComponent>()))
	{
		Part = NewObject<USceneComponent>(Owner, Template->GetClass(), NAME_None, RF_Transient, Template);
		Part->SetMobility(EComponentMobility::Movable);
		Part->SetVisibility(false);
		if (USkeletalMeshComponent* Mesh = Cast<USkeletalMeshComponent>(Part))
		{
			Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);   // the body plays the actions, the others follow
			if (Node->GetVariableName() == BodyName)
			{
				Adding.Leader = Mesh;
				Mesh->SetRelativeTransform(FTransform(FQuat::Identity, FVector::ZeroVector, FVector(Adding.Scale)));
			}
			else if (Node->GetVariableName() != FaceName && Adding.Garment)
			{
				for (int32 Slot = 0; Slot < Mesh->GetNumMaterials(); ++Slot)
				{
					Mesh->SetMaterial(Slot, Adding.Garment);
				}
			}
		}
		if (UGroomComponent* Groom = Cast<UGroomComponent>(Part))
		{
			Groom->SimulationSettings.bOverrideSettings = true;   // no hair physics on the small figures
			Groom->SimulationSettings.SolverSettings.bEnableSimulation = false;
			Groom->SetForcedLOD(GroomLOD);
		}
		Part->SetupAttachment(Parent, Node->AttachToName);
		Part->RegisterComponent();
		Owner->AddInstanceComponent(Part);
	}
	for (const USCS_Node* Child : Node->GetChildNodes())
	{
		AddNode(Adding, Child, Part);
	}
}
