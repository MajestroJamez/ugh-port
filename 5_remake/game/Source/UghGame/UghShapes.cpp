#include "UghShapes.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	/** The engine's shapes are 100 units across. */
	constexpr double ShapeSize = 100.0;
}

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

UInstancedStaticMeshComponent* UghShapes::AddBoxes(AActor* Owner, const FLinearColor& Color)
{
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UMaterialInterface* Material =
		LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	UInstancedStaticMeshComponent* Boxes = NewObject<UInstancedStaticMeshComponent>(Owner);
	Boxes->SetMobility(EComponentMobility::Movable);
	Boxes->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Boxes->SetStaticMesh(Cube);
	UMaterialInstanceDynamic* Colored = UMaterialInstanceDynamic::Create(Material, Boxes);
	Colored->SetVectorParameterValue(TEXT("Color"), Color);
	Boxes->SetMaterial(0, Colored);
	Boxes->SetupAttachment(Owner->GetRootComponent());
	Boxes->RegisterComponent();
	Owner->AddInstanceComponent(Boxes);
	return Boxes;
}

void UghShapes::SetBoxes(UInstancedStaticMeshComponent* Component, const TArray<FTransform>& Boxes)
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
