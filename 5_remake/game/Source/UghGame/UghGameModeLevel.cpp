// The level's diorama built (at once, or a step a frame in the flight between two levels) and that flight.
#include "UghAssets.h"
#include "UghBackground.h"
#include "UghArchipelago.h"
#include "UghCampfire.h"
#include "UghCaveAir.h"
#include "UghCliffDressing.h"
#include "UghCopters.h"
#include "UghDecorations.h"
#include "UghEffects.h"
#include "UghFalls.h"
#include "UghFigures.h"
#include "UghGameMode.h"
#include "UghGround.h"
#include "UghMenuView.h"
#include "UghMood.h"
#include "UghPadSigns.h"
#include "UghRain.h"
#include "UghRockDressing.h"
#include "UghRockField.h"
#include "UghRockMesh.h"
#include "UghScenery.h"
#include "UghShapes.h"
#include "UghSigns.h"
#include "UghStage.h"
#include "UghStreams.h"
#include "UghTorches.h"
#include "UghTurf.h"
#include "UghWater.h"
#include "Engine/Engine.h"
#include "MeshDescription.h"

/** What a level's diorama is made of (AUghGameMode::PlanLevel), before it is put into the scene. */
struct FUghLevelPlan
{
	int32 LevelId = -1, Level = 0, Wind = 0, WaterRow = 0;
	TArray<FColor> Art;
	FUghRockField Field;
	TArray<FUghPadSign> PadSigns;
	TArray<FUghStream> Streams;
	FUghRockMesh Rock;
	TArray<FMeshDescription> RockPieces;   // its pieces described (FUghRockMesh::Describe)
	FUghTurf Turf;
	TArray<FUghDecoration> Decorations;
	TArray<FUghRockPiece> Pieces;
	double PlanSeconds = 0;
};

namespace
{
	/**
	 * The steps of putting a plan into the scene (AUghGameMode::ApplyLevel), each short: the drawing, a step a piece of
	 * the rock (PiecesOfRock of them), then these.
	 */
	enum EStep : int32 { StepBlades, StepShown, StepSigns, StepScenery, StepDressing, StepMood, StepAir, StepPeople,
		StepCopters, StepBursts, Steps };
	const TCHAR* const StepNames[] = { TEXT("turf"), TEXT("shown"), TEXT("signs"), TEXT("scenery"), TEXT("dressing"),
		TEXT("mood"), TEXT("air"), TEXT("people"), TEXT("copters"), TEXT("bursts") };
	/** The rock in so many pieces (each built in a frame of its own in a flight between two levels). */
	constexpr int32 PiecesOfRock = 16;
	/** In the mist of a flight between two levels the steps that change the scene take about this long a frame (s). */
	constexpr double MistStepSeconds = 0.012;

	/** A step's time in the log (a look at the frames of a flight between two levels). */
	void LogStep(const TCHAR* Name, double Started)
	{
		UE_LOG(LogTemp, Display, TEXT("UGH level step %s: %.1f ms"), Name, (FPlatformTime::Seconds() - Started) * 1000);
	}

	/** The flight's mist by the mood (the colour the picture fades into: never black, a cloud lit as the sky). */
	FLinearColor MistOf(const FUghMood& Mood)
	{
		switch (Mood.Kind)
		{
		case FUghMood::EKind::Evening: return FLinearColor(0.78f, 0.68f, 0.58f);
		case FUghMood::EKind::Dusk: return FLinearColor(0.46f, 0.43f, 0.52f);
		case FUghMood::EKind::Night: return FLinearColor(0.08f, 0.10f, 0.14f);
		case FUghMood::EKind::Storm: return FLinearColor(0.40f, 0.43f, 0.46f);
		default: return FLinearColor(0.84f, 0.87f, 0.91f);
		}
	}
}

TSharedPtr<FUghLevelPlan> AUghGameMode::PlanLevel(const ugh_logic* Logic, const ugh_logic_view& View,
	TArray<FColor> Art, TArray<FUghArtTile> Doors, TArray<FUghArtTile> Signs)
{
	const double Started = FPlatformTime::Seconds();
	TSharedPtr<FUghLevelPlan> Plan = MakeShared<FUghLevelPlan>();
	Plan->LevelId = View.level_id;
	Plan->Level = View.level;
	Plan->Wind = View.wind;
	Plan->WaterRow = View.water_level / UghShapes::Subpixels;
	Plan->Art = MoveTemp(Art);
	Plan->Field.Build(Logic, Plan->Art, Doors);
	if (View.level_id >= 0)
	{
		Plan->PadSigns = UghPadSigns::Plan(Logic, FUghGround(Logic, Plan->Field), Signs);
		Plan->Streams = UghStreams::Plan(Logic, Plan->Field, Plan->WaterRow, Plan->PadSigns);
	}
	Plan->Field.CarveChannels(Plan->Streams);
	Plan->Rock.Build(Plan->Field);
	Plan->Rock.Describe(PiecesOfRock, Plan->RockPieces);
	if (View.level_id >= 0)
	{
		Plan->Turf.Build(Logic, Plan->Field, Plan->WaterRow, Plan->Streams);
		const double Planned = FPlatformTime::Seconds();
		Plan->Decorations =
			UghDecorations::Plan(Logic, Plan->Field, View.level_id, Plan->WaterRow, Plan->PadSigns, Plan->Streams);
		UE_LOG(LogTemp, Display, TEXT("UGH decorations: %d in %.0f ms (%s), streams %d"), Plan->Decorations.Num(),
			(FPlatformTime::Seconds() - Planned) * 1000, *UghDecorations::Summary(Plan->Decorations), Plan->Streams.Num());
		Plan->Pieces = UghRockDressing::Plan(Logic, Plan->Field, View.level_id, Plan->Decorations, Plan->Streams);
	}
	Plan->PlanSeconds = FPlatformTime::Seconds() - Started;
	return Plan;
}

void AUghGameMode::BuildLevel(const ugh_logic_view& View)
{
	const TSharedPtr<FUghLevelPlan> Built = PlanLevel(Simulation.GetLogic(), View, LevelArt.Draw(View.level_id, Sprites),
		LevelArt.Doors(View.level_id), LevelArt.Signs(View.level_id));
	for (int32 Step = 0; ApplyLevel(*Built, Step); ++Step)
	{
	}
}

/**
 * The diorama of the level planned: the rock coloured by its drawing, the pads' boards, its springs' streams, the
 * campfires and torches, the decorations, the scanned rock dressing the cliff; the light and the air of its mood
 * (UghMood), its rain; the people, the copters' riders and the bursts the play may show made now (not in the play).
 */
int32 AUghGameMode::FirstUnseenStep(const FUghLevelPlan& Built)
{
	return Built.RockPieces.Num() + 1 + StepShown;
}

bool AUghGameMode::ApplyLevel(const FUghLevelPlan& Built, int32 Step)
{
	const double Started = FPlatformTime::Seconds();
	const ugh_logic* Logic = Simulation.GetLogic();
	const bool bLevel = Built.LevelId >= 0;
	const int32 Pieces = Built.RockPieces.Num();
	if (Step <= Pieces)
	{
		// the drawing, then the rock a piece at a time
		if (Step == 0)
		{
			Background->BeginBuild(Built.Art, bLevel ? &Built.Turf : nullptr);
		}
		else
		{
			Background->AddRock(Built.RockPieces[Step - 1]);
		}
		UE_CLOG(Step == Pieces, LogTemp, Display, TEXT("UGH level: the rock in %d pieces, planned in %.0f ms"), Pieces,
			Built.PlanSeconds * 1000);
		LogStep(Step == 0 ? TEXT("drawing") : TEXT("rock"), Started);
		return true;
	}
	const int32 Later = Step - Pieces - 1;
	switch (Later)
	{
	case StepBlades:
		if (bLevel)
		{
			Background->AddBlades(Built.Turf);
		}
		break;
	case StepShown:
		Background->FinishBuild();   // (the old rock gone: in the mist of a flight)
		break;
	case StepSigns:
	{
		// (else the drawing shows them)
		Signs->Show(Background->ShowsArt() ? TArray<FUghPadSign>() : Built.PadSigns, Sprites);
		Falls->Show(Built.Streams);
		const FUghMood& Mood = UghMood::Of(Built.Level, Built.Wind);
		// what a shot wants to look at
		const FUghDecoration* Looked = Built.Decorations.FindByPredicate([this](const FUghDecoration& Decoration)
		{
			return Shot.GetLook() == UghDecorations::Name(Decoration.Kind);
		});
		ShotLook = Looked ? FVector2D(Looked->X, Looked->Y - Looked->Height / 2) : TOptional<FVector2D>();
		ShotAround = FUghShot::LookAround;
		Campfire->Place(Built.Decorations, Built.Wind, Mood.FireLight);
		Torches->Place(Built.Decorations, Built.Wind, Mood.FireLight);
		HearFires(Built.Decorations);
		break;
	}
	case StepScenery:
		Scenery->Show(Built.Decorations);
		break;
	case StepDressing:
	{
		const int32 Roots = Built.Pieces.FilterByPredicate([](const FUghRockPiece& Piece)
		{
			return Piece.Kind == FUghRockPiece::EKind::Root;
		}).Num();
		UE_LOG(LogTemp, Display, TEXT("UGH rock dressing: %d cliffs, %d roots"), Built.Pieces.Num() - Roots, Roots);
		Dressing->Show(Built.Pieces);
		break;
	}
	case StepMood:
	{
		const FUghMood& Shown = UghMood::Of(Built.Level, Built.Wind);
		UE_LOG(LogTemp, Display, TEXT("UGH mood: %s"), Shown.Name);
		Stage->SetMood(Shown, Built.Wind);
		Effects->SetShade(Shown.Shade);
		Water->SetWeather(Built.Wind, Stage->SunDirection(), Shown.Caustics);
		Water->SetSky(UghAssets::Texture(Shown.Sky), Shown.SkySeen);
		break;
	}
	case StepAir:
		CaveAir->Build(bLevel ? Logic : nullptr, Built.Decorations, UghMood::Of(Built.Level, Built.Wind),
			Stage->SunDirection(), Built.WaterRow);
		Rain->Build(bLevel ? Logic : nullptr, Built.LevelId, Built.Wind);
		BackgroundLevel = Built.LevelId;
		MoodLevel = Built.Level;
		break;
	// the people the play may show, made now (a MetaHuman made in the play was a hitch)
	case StepPeople:
		if (bLevel && !bInMenu)
		{
			Figures->Stock();
		}
		break;
	case StepCopters:
		if (bLevel && !bInMenu)
		{
			Copters->Stock();
		}
		break;
	case StepBursts:
		if (bLevel && !bInMenu)
		{
			// (a flung passenger's splash, FUghFlings; a copter falling into the sea and coming up, FUghDunks)
			Effects->Stock({ EUghBurst::Plunge, EUghBurst::Dunk, EUghBurst::Boil });
		}
		break;
	default:
		return false;
	}
	LogStep(StepNames[Later], Started);
	return Later + 1 < Steps;
}

bool AUghGameMode::StartVoyage(const FUghCameraPose& From, int32 Next, bool bFromPlay)
{
	const int32 Count = Passwords.LevelCount(Playing.Players);
	if (!bIntro || !Profile.Settings.bIntro || Next < 0 || Next >= Count || (bFromPlay && Next < 1))
	{
		return false;
	}
	// the archipelago of the mode around the stone: before, the stone stands for the level left (from the play), else
	// the archipelago is where the level selection showed it; after the switch, it stands for the next level
	const FVector Home = UghMenuView::StoneMiddle(), Stone(Home.X, Home.Y, 0);
	VoyagePlaces = FUghIsles::Layout(Count, Home);
	VoyageStates.Reset();
	for (int32 Level = 0; Level < Count; ++Level)
	{
		VoyageStates.Add(FUghIsles::StateOf(Profile.Scores, Playing.Players, Level, Count));
	}
	VoyageLeft = bFromPlay ? Next - 1 : INDEX_NONE;
	VoyageNext = Next;
	VoyageBefore = bFromPlay ? Stone - VoyagePlaces[VoyageLeft].Foot : FVector::ZeroVector;
	VoyageAfter = Stone - VoyagePlaces[Next].Foot;
	VoyageBefore.Z = VoyageAfter.Z = 0;
	Voyage.Start(From, AUghStage::Play(AUghStage::ViewportAspect()), VoyageBefore - VoyageAfter, Stone, bFromPlay, SeaZ);
	if (bFromPlay)
	{
		FrozenPrevious = Simulation.GetPrevious();   // (from the selection: the title's level, taken before the game began)
		FrozenCurrent = Simulation.GetCurrent();
	}
	MistBefore = MistAfter = MistOf(UghMood::Of(FrozenCurrent.level, FrozenCurrent.wind));
	bIntroScene = bVoyageScene = true;
	IntroLevel = Next;   // (no flight from black at its caption)
	SurfaceAge = 1e9;
	UE_LOG(LogTemp, Display, TEXT("UGH voyage to level %d (%s), %.1f s, the mist at %.2f s"), Next + 1,
		bFromPlay ? TEXT("from the play") : TEXT("from the level selection"), Voyage.GetDuration(), Voyage.GetSwitchTime());
	CameraLog.Note(TEXT("voyage"));
	return true;
}

bool AUghGameMode::ShotLevelDone()
{
	if (bInMenu || Voyage.IsFlying() || !StartVoyage(CameraPose, Simulation.GetCurrent().level + 1, true))
	{
		return false;
	}
	ShotNextIn = ShotNextAfter;
	return true;
}

void AUghGameMode::SwitchVoyage(const ugh_logic_view& Live)
{
	const bool bNext = Live.level_id >= 0 && (Live.level_id != BackgroundLevel || Live.level != MoodLevel);
	if (!bNext && !Plan)
	{
		return;   // the logic is still at the level left
	}
	if (!Plan && !PlanTask.IsValid())
	{
		// the next level planned on a worker meanwhile (the logic only waits for its caption's key: its level stays)
		const ugh_logic* Logic = Simulation.GetLogic();
		TArray<FColor> Art = LevelArt.Draw(Live.level_id, Sprites);   // (the sprites load here, on this thread)
		PlanTask = UE::Tasks::Launch(TEXT("UghLevelPlan"),
			[Logic, Live, Art = MoveTemp(Art), Doors = LevelArt.Doors(Live.level_id), Signs = LevelArt.Signs(Live.level_id)]
			{
				return PlanLevel(Logic, Live, Art, Doors, Signs);
			});
		CameraLog.Note(TEXT("planning"));
	}
	// the play about to begin (hurried late): all of it at once
	const bool bLate = Live.phase == UGH_LOGIC_PHASE_SETUP || Live.phase == UGH_LOGIC_PHASE_PLAY;
	if (!Plan)
	{
		if (!PlanTask.IsCompleted() && !bLate)
		{
			return;   // (in the mist: held there)
		}
		Plan = PlanTask.GetResult();   // (waits when late)
		PlanTask = {};
		PlanStep = 0;
	}
	// the new rock made hidden while the level left is still seen, a piece a frame; what changes the scene only in the
	// mist, as many steps a frame as fit in MistStepSeconds
	const int32 Unseen = FirstUnseenStep(*Plan);
	if (PlanStep < Unseen && !bLate)
	{
		ApplyLevel(*Plan, PlanStep++);
		return;
	}
	if (!Voyage.WantsSwitch() && !bLate)
	{
		return;
	}
	if (PlanStep == Unseen)
	{
		SurfaceFrom = LastSurface;
	}
	const double Until = FPlatformTime::Seconds() + MistStepSeconds;
	bool bMore = true;
	do
	{
		bMore = ApplyLevel(*Plan, PlanStep++);
	}
	while (bMore && (bLate || FPlatformTime::Seconds() < Until));
	if (bMore)
	{
		return;
	}
	Plan.Reset();
	Voyage.Switched();
	MistAfter = MistOf(UghMood::Of(Live.level, Live.wind));
	Effects->Clear();   // (the fireworks of the level left)
	SurfaceAge = 0;
	CameraLog.Note(TEXT("voyage switched"));
	UE_LOG(LogTemp, Display, TEXT("UGH voyage: the world switched at %.2f s"), Voyage.GetTime());
	if (bLate)
	{
		Voyage.Hurry();
	}
	// the garbage of the level left collected now, while the mist thins (in the play it was a hitch)
	if (GEngine)
	{
		CollectedAt = FPlatformTime::Seconds();
		GEngine->ForceGarbageCollection(true);
	}
}

void AUghGameMode::DropPlan()
{
	if (PlanTask.IsValid())
	{
		PlanTask.Wait();   // (it reads the logic: not while that changes)
		PlanTask = {};
	}
	Plan.Reset();
}
