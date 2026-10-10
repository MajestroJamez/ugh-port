// The figures as automation tests of the editor: what every sprite of the data means (Ugh.Figures.Actions), the
// clock of the actions that follow the frames (Ugh.Figures.Clock), the imported models (Ugh.Figures.Models).
#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Misc/AutomationTest.h"
#include "UghAssets.h"
#include "UghBubbles.h"
#include "UghFigureActions.h"
#include "UghFigureModels.h"
#include "UghFigurePlace.h"
#include "UghFrameClock.h"
#include "UghShapes.h"
#include "UghSimulation.h"
#include "UghSprites.h"

namespace
{
	const FString Assets() { return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../../assets")); }

	FString Describe(EUghModel Model, const FUghFigureAction& Action)
	{
		return FString::Printf(TEXT("model %d action %s"), int32(Model), Action.Action);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghFigureActionsTest, "Ugh.Figures.Actions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUghFigureActionsTest::RunTest(const FString& Parameters)
{
	FUghSimulation Simulation;
	FUghSprites Sprites;
	FString Error;
	if (!TestTrue(TEXT("the sprites: ") + Error, Sprites.Load(Assets(), Error)) ||
		!TestTrue(TEXT("the game data: ") + Error, Simulation.Load(Assets() / TEXT("logic/ugh-data.ugd"), Error)))
	{
		return false;
	}
	// every sprite an entity of the data shows is an action its model has (a speech bubble is UghBubbles's)
	TSet<EUghModel> Models;
	int32 Named = 0;
	for (int32 Sprite = 0; Sprite < Sprites.Count(); ++Sprite)
	{
		ugh_logic_sprite Info;
		if (!ugh_logic_get_sprite(Simulation.GetLogic(), Sprite, &Info) ||
			UghBubbles::Look(Simulation.GetLogic(), Sprite).IsSet())
		{
			continue;
		}
		++Named;
		const TOptional<FUghFigureAction> Action = FUghFigureActions::Describe(Info);
		if (!TestTrue(FString::Printf(TEXT("sprite %d (%hs) has a rule"), Sprite, Info.name), Action.IsSet()))
		{
			continue;
		}
		Models.Add(Action->Model);
		TestTrue(FString::Printf(TEXT("sprite %d (%hs): %s is an action of its model"), Sprite, Info.name,
			*Describe(Action->Model, *Action)), FUghFigureActions::ActionsOf(Action->Model).Contains(Action->Action));
		TestTrue(FString::Printf(TEXT("sprite %d (%hs): a bonus item names its mesh"), Sprite, Info.name),
			(Action->Model == EUghModel::BonusItem) == !Action->Item.IsEmpty());
	}
	TestTrue(FString::Printf(TEXT("the data names sprites (%d)"), Named), Named > 250);
	TestEqual(TEXT("every model shows some sprite"), Models.Num(), int32(EUghModel::BonusItem) + 1);

	// what the sprite does not say: a knocked out enemy, a passenger going down in the water
	FUghFigureActions Actions;
	Actions.Load(Simulation.GetLogic(), Sprites.Count());
	const ugh_logic_entity Flyer{ UGH_LOGIC_ENTITY_ENEMY, 0, 0, 0, 234, -1, 0, 0 };
	TestEqual(TEXT("a flyer flies"), FString(Actions.Of(Flyer, &Flyer)->Action), FString(TEXT("fly")));
	ugh_logic_entity Hit = Flyer;
	Hit.stunned = 1;
	TestEqual(TEXT("a flyer a passenger hit falls"), FString(Actions.Of(Hit, &Flyer)->Action), FString(TEXT("fall")));
	const ugh_logic_entity Afloat{ UGH_LOGIC_ENTITY_PASSENGER, 0, 0, 0, 379, -1, 1, 0 };
	TestEqual(TEXT("a passenger treads water"), FString(Actions.Of(Afloat, &Afloat)->Action), FString(TEXT("tread")));
	ugh_logic_entity Sinking = Afloat;
	Sinking.y += UGH_LOGIC_SUBPIXELS;
	TestEqual(TEXT("a passenger going down falls"), FString(Actions.Of(Sinking, &Afloat)->Action),
		FString(TEXT("fall")));
	TestFalse(TEXT("a rotor's sprite is no figure"), Actions.Of({ UGH_LOGIC_ENTITY_ENEMY, 0, 0, 0, 218, -1, 0, 0 },
		nullptr).IsSet());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghFrameClockTest, "Ugh.Figures.Clock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUghFrameClockTest::RunTest(const FString& Parameters)
{
	// 60 frames a second over a walk of 9 sprites, each 7 steps of the logic
	constexpr int32 Frames = 9, StepsPerFrame = 7;
	FUghFrameClock Clock;
	double Last = 0, Jump = 0;
	for (int32 Frame = 0; Frame < 600; ++Frame)
	{
		const int32 Step = int32(Frame / 60.0 * FUghSimulation::TickRate);
		const int32 Sprite = (Step / StepsPerFrame) % Frames;
		Clock.Update(Sprite, Frames, 1 / 60.0);
		const double Phase = Clock.Phase();
		TestTrue(TEXT("within the frame shown"), Phase >= double(Sprite) / Frames &&
			Phase < double(Sprite + 1) / Frames);
		if (Frame > 120)
		{
			Jump = FMath::Max(Jump, FMath::Abs(FMath::Frac(Phase - Last + 0.5) - 0.5));
		}
		Last = Phase;
	}
	// a smooth loop moves about 1 / 60 of a loop of 0.9 s a frame; a stepped one 1 / 9 at a time
	TestTrue(FString::Printf(TEXT("goes on smoothly (at most %.3f of the loop a frame)"), Jump), Jump < 0.06);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghFigureModelsTest, "Ugh.Figures.Models",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUghFigureModelsTest::RunTest(const FString& Parameters)
{
	FUghFigureModels Models;
	Models.Load();
	struct FSized
	{
		EUghModel Model;
		const TCHAR* Id;   // the figure
		FIntPoint Sprite;   // px
	};
	const FSized Rigged[] = { { EUghModel::Flyer, TEXT("the flyer"), { 32, 23 } },
		{ EUghModel::Walker, TEXT("the walker"), { 32, 22 } }, { EUghModel::Blower, TEXT("the blower"), { 32, 22 } },
		{ EUghModel::Tree, TEXT("the tree"), { 32, 24 } } };
	for (const FSized& Each : Rigged)
	{
		if (!Models.Has(Each.Model))
		{
			AddInfo(FString::Printf(TEXT("%s is not imported (fetch-assets.ps1, build.ps1): not checked"), Each.Id));
			continue;
		}
		// about as long as its sprite is wide (it may reach a little further: wings, a tail), not higher than it; the
		// T-rex lying flat shown bigger (its tail reaching far behind), about as much of the screen
		const USkeletalMesh* Mesh = Models.SkeletalMeshOf(Each.Model);
		AddInfo(FString::Printf(TEXT("%s: %s"), Each.Id, *Mesh->GetPathName()));
		const FBox Box = Mesh->GetBounds().GetBox();
		const FVector2D Wanted = FVector2D(Each.Sprite) * UghShapes::UnitsPerPixel;
		const bool bTrex = Mesh == UghAssets::SkeletalMesh(UghAssets::BlowerTrex);
		const FVector Scale = Each.Model == EUghModel::Flyer ? FVector(UghFigurePlace::FlyerScale)
			: bTrex ? UghFigurePlace::TrexScale : FVector::OneVector;
		const double Longest = bTrex ? 3 : 1.4;   // (its tail curls away far behind, into the rock)
		const FVector2D Got(FMath::Max(Box.GetSize().X * Scale.X, Box.GetSize().Y * Scale.Y), Box.GetSize().Z * Scale.Z);
		TestTrue(FString::Printf(TEXT("%s is as big as its sprite (%.0f x %.0f, its sprite %.0f x %.0f)"), Each.Id,
			Got.X, Got.Y, Wanted.X, Wanted.Y),
			Got.X > Wanted.X * 0.7 && Got.X < Wanted.X * Longest && Got.Y < Wanted.Y * 1.4 &&
			(!bTrex || Got.Y > Wanted.Y * 0.5));
	}
	// the light shines through the flyer's wings: their slot and maps are there
	if (const USkeletalMesh* Flyer = Models.SkeletalMeshOf(EUghModel::Flyer))
	{
		TestTrue(TEXT("the flyer has wings"), Flyer->GetMaterials().ContainsByPredicate([](const FSkeletalMaterial& Slot)
			{ return Slot.MaterialSlotName == FUghFigureModels::WingSlot; }));
		TestTrue(TEXT("the wings' maps"), UghAssets::Texture(UghAssets::Pterodactyl, FUghFigureModels::WingColor) &&
			UghAssets::Texture(UghAssets::Pterodactyl, FUghFigureModels::WingNormal));
	}
	// the T-rex snorts its dust out of its nostrils, along their way from its head
	if (const USkeletalMesh* Trex = UghAssets::SkeletalMesh(UghAssets::BlowerTrex))
	{
		const FReferenceSkeleton& Bones = Trex->GetRefSkeleton();
		TestTrue(TEXT("the T-rex has nostrils and a head"), Bones.FindBoneIndex(FUghSnort::Nostrils) !=
			INDEX_NONE && Bones.FindBoneIndex(FUghSnort::Head) != INDEX_NONE);
	}
	if (Models.Has(EUghModel::BonusItem))
	{
		for (const TCHAR* Kind : { TEXT("energy1"), TEXT("energy5"), TEXT("energy9"), TEXT("multiplier") })
		{
			TestNotNull(FString::Printf(TEXT("a bonus item %s"), Kind), UghAssets::Mesh(UghAssets::BonusItems, Kind));
		}
	}
	return true;
}

#endif
