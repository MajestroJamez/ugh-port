#include "UghSigns.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "UghAssets.h"
#include "UghShapes.h"
#include "UghSprites.h"
#include "UghTexture.h"

AUghSigns::AUghSigns()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Static);
}

void AUghSigns::BeginPlay()
{
	Super::BeginPlay();
	for (int32 Marks = 0; Marks <= UghPadSigns::MostMarks; ++Marks)
	{
		Models.Add(UghAssets::Mesh(UghAssets::Signs, *FString::Printf(TEXT("sign_%d"), Marks)));
	}
	if (Models.Contains(nullptr))
	{
		UE_LOG(LogTemp, Display, TEXT("UGH the boards are not imported (fetch-assets.ps1, build.ps1): the original's"));
		Models.Reset();
	}
}

UStaticMeshComponent* AUghSigns::AddModel(UStaticMesh* Model, const FVector& Location)
{
	UStaticMeshComponent* Board = NewObject<UStaticMeshComponent>(this);
	Board->SetMobility(EComponentMobility::Static);
	Board->SetStaticMesh(Model);
	Board->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Board->SetupAttachment(RootComponent);
	Board->SetWorldLocation(Location);
	Board->RegisterComponent();
	AddInstanceComponent(Board);
	return Board;
}

void AUghSigns::Show(const TArray<FUghPadSign>& Signs, const FUghSprites& Sprites)
{
	for (UStaticMeshComponent* Board : Boards)
	{
		Board->DestroyComponent();   // static ones: new ones where the new level has its boards
	}
	Boards.Reset();
	for (const FUghPadSign& Sign : Signs)
	{
		if (!Models.IsEmpty())
		{
			Boards.Add(AddModel(Models[Sign.Marks], UghShapes::ToWorld(Sign.X, Sign.Y, UghPadSigns::Depth)));
			continue;
		}
		UStaticMeshComponent* Card = Boards.Add_GetRef(UghShapes::AddCard(this));
		const int32 Sprite = Sign.Sprite.Get(UghPadSigns::SpriteOf(Sign.Marks));
		TObjectPtr<UTexture2D>& Texture = SpriteTextures.FindOrAdd(Sprite);
		const FIntPoint Size = Sprites.Size(Sprite);
		if (!Texture)
		{
			Texture = UghTexture::Create(this, Size.X, Size.Y, Sprites.Pixels(Sprite), true);
		}
		const FBox2D Box = Sign.Box();
		UghShapes::ShowCard(Card, Texture, Box.Min.X, Box.Min.Y, Size.X, Size.Y, UghPadSigns::Front);
	}
}
