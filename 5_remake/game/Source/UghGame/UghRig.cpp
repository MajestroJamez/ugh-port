#include "UghRig.h"

#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Actor.h"
#include "UghAssets.h"

bool FUghRig::Load(const TCHAR* Id, TConstArrayView<const TCHAR*> ActionNames)
{
	TArray<UAnimSequence*> Found;
	for (const TCHAR* Name : ActionNames)
	{
		Found.Add(UghAssets::Animation(Id, Name));
	}
	return Load(UghAssets::SkeletalMesh(Id), Found);
}

bool FUghRig::Load(USkeletalMesh* InMesh, TConstArrayView<UAnimSequence*> InActions)
{
	Actions.Reset();
	for (UAnimSequence* Action : InActions)
	{
		Actions.Add(Action);
	}
	Mesh = InActions.Contains(nullptr) ? nullptr : InMesh;
	return IsLoaded();
}

USkeletalMeshComponent* FUghRig::Add(AActor* Owner) const
{
	USkeletalMeshComponent* Model = NewObject<USkeletalMeshComponent>(Owner);
	Model->SetSkeletalMesh(Mesh);
	Model->SetMobility(EComponentMobility::Movable);
	Model->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Model->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	Model->SetVisibility(false);
	Model->SetupAttachment(Owner->GetRootComponent());
	Model->RegisterComponent();
	Owner->AddInstanceComponent(Model);
	return Model;
}

void FUghRig::Play(USkeletalMeshComponent* Model, int32 Action) const
{
	UAnimSequence* Sequence = Actions[Action];
	if (Model->GetSingleNodeInstance() == nullptr || Model->GetSingleNodeInstance()->GetAnimationAsset() != Sequence)
	{
		Model->PlayAnimation(Sequence, true);
	}
	Model->SetPlayRate(1.f);
}

void FUghRig::Hold(USkeletalMeshComponent* Model, int32 Action, double Fraction) const
{
	Play(Model, Action);
	Model->SetPlayRate(0.f);
	Model->SetPosition(float(Fraction * Actions[Action]->GetPlayLength()), false);
}
