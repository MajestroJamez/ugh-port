#include "UghShapes.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Actor.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/PackageName.h"
#include "UghMaterials.h"

FVector UghShapes::ToWorld(double X, double Y, double Depth)
{
	return FVector(X * UnitsPerPixel, -Depth, (ScreenHeight - Y) * UnitsPerPixel);
}

FTransform UghShapes::Box(double Left, double Top, double Width, double Height, double Depth, double Thickness)
{
	const FVector Centre = ToWorld(Left + Width / 2, Top + Height / 2, Depth);
	const FVector Scale(Width * UnitsPerPixel / ShapeSize, Thickness / ShapeSize, Height * UnitsPerPixel / ShapeSize);
	return FTransform(FQuat::Identity, Centre, Scale);
}

UMaterialInstanceDynamic* UghShapes::Material(UObject* Outer, const TCHAR* Path)
{
	const FString Object = FString::Printf(TEXT("%s.%s"), Path, *FPackageName::GetShortName(Path));
	UMaterialInterface* Parent = LoadObject<UMaterialInterface>(nullptr, *Object);
	if (!Parent)
	{
		UE_LOG(LogTemp, Error, TEXT("UGH no material %s: run build.ps1 (it makes them)"), Path);
		Parent = UMaterial::GetDefaultMaterial(MD_Surface);
	}
	return UMaterialInstanceDynamic::Create(Parent, Outer);
}

UMaterialInstanceDynamic* UghShapes::Clay(UObject* Outer, const FLinearColor& Color)
{
	UMaterialInstanceDynamic* Clay = Material(Outer, UghMaterials::Clay);
	Clay->SetVectorParameterValue(UghMaterials::ColorParameter, Color);
	return Clay;
}

UInstancedStaticMeshComponent* UghShapes::AddShapes(AActor* Owner, EShape Shape, UMaterialInterface* Material)
{
	static const TCHAR* const Meshes[] = { TEXT("/Engine/BasicShapes/Cube.Cube"),
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), TEXT("/Engine/BasicShapes/Sphere.Sphere"),
		TEXT("/Engine/BasicShapes/Cone.Cone") };
	UInstancedStaticMeshComponent* Shapes = NewObject<UInstancedStaticMeshComponent>(Owner);
	Shapes->SetMobility(EComponentMobility::Movable);
	Shapes->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Shapes->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, Meshes[static_cast<int32>(Shape)]));
	Shapes->SetMaterial(0, Material);
	Shapes->SetupAttachment(Owner->GetRootComponent());
	Shapes->RegisterComponent();
	Owner->AddInstanceComponent(Shapes);
	return Shapes;
}

void UghShapes::SetShapes(UInstancedStaticMeshComponent* Component, const TArray<FTransform>& Boxes)
{
	if (Component->GetInstanceCount() == Boxes.Num())
	{
		if (!Boxes.IsEmpty())
		{
			Component->BatchUpdateInstancesTransforms(0, Boxes, true, true, true);
		}
		return;
	}
	Component->ClearInstances();
	Component->AddInstances(Boxes, false, true, false);
}

UStaticMeshComponent* UghShapes::AddCard(AActor* Owner)
{
	UStaticMeshComponent* Card = NewObject<UStaticMeshComponent>(Owner);
	Card->SetMobility(EComponentMobility::Movable);
	Card->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
	Card->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Card->SetCastShadow(false);
	Card->SetMaterial(0, Material(Card, UghMaterials::Sprite));
	Card->SetupAttachment(Owner->GetRootComponent());
	Card->RegisterComponent();
	Owner->AddInstanceComponent(Card);
	return Card;
}

void UghShapes::ShowCard(UStaticMeshComponent* Card, UTexture2D* Sprite, double Left, double Top, double Width,
	double Height, double Depth)
{
	Cast<UMaterialInstanceDynamic>(Card->GetMaterial(0))->SetTextureParameterValue(UghMaterials::ArtParameter, Sprite);
	// a thin box as big as the sprite: its front face shows the sprite
	constexpr double Thickness = 1;
	Card->SetWorldTransform(Box(Left, Top, Width, Height, Depth, Thickness));
	Card->SetVisibility(true);
}
