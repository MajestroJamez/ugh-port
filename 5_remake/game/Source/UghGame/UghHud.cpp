#include "UghHud.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "UghBetween.h"
#include "UghEffects.h"
#include "UghGameMode.h"
#include "UghShapes.h"
#include "UghUi.h"
#include "UghUiState.h"
#include "UghUiStyle.h"
#include "UghWarning.h"

namespace
{
	constexpr float FullEnergy = UGH_LOGIC_FULL_ENERGY;
	/** How fast the help comes and goes (its share a second). */
	constexpr double HelpRate = 4;
	/** A stone's number shows this far (units) over its top (its flag), whole up to this far from the camera. */
	constexpr double NumberAbove = 1800, NumberNear = 60000;
}

void AUghHud::DrawHUD()
{
	Super::DrawHUD();
	const AUghGameMode* Mode = GetWorld()->GetAuthGameMode<AUghGameMode>();
	if (!Mode || !Canvas || Mode->GetAssets().IsEmpty())   // before StartPlay
	{
		return;
	}
	if (!Screen)
	{
		Build(*Mode);
	}
	Update(*Mode, GetWorld()->GetDeltaSeconds());
}

void AUghHud::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Screen && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(Screen.ToSharedRef());
	}
	Screen.Reset();
	Super::EndPlay(Reason);
}

void AUghHud::Build(const AUghGameMode& Mode)
{
	UghUiStyle::FindFonts(Mode.GetAssets());
	State = MakeShared<FUghUiState>();
	State->Pictures = UghUiStyle::MakePictures(this, Pictures);
	Screen = SNew(SUghScreen).State(State);
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->AddViewportWidgetContent(Screen.ToSharedRef());
	}
}

void AUghHud::Update(const AUghGameMode& Mode, double Seconds)
{
	FUghUiState& Shown = *State;
	Shown.Time = GetWorld()->GetRealTimeSeconds();
	Shown.Problem = Mode.GetProblem();
	Shown.Screen = !Shown.Problem.IsEmpty() ? FUghUiState::EScreen::Problem
		: Mode.IsInMenu() ? FUghUiState::EScreen::Menu : FUghUiState::EScreen::Play;
	const FUghMenu& Menu = Mode.GetMenu();
	Shown.Row = Menu.GetRow();
	Shown.Choice = Menu.GetChoice();
	Shown.Levels = Mode.GetPasswords().LevelCount(Shown.Choice.Players);
	Shown.Password = Menu.GetPassword();
	Shown.bPasswordKnown = Menu.IsPasswordKnown();
	Shown.bShowingEnd = Menu.IsShowingEnd();
	Shown.LastGame = Menu.GetLastGame();
	UpdateScreens(Mode);
	UpdateIsles(Mode);
	Shown.Upscaler = Mode.GetUpscaler().Describe();

	const ugh_logic_view& View = Mode.GetSimulation().GetCurrent();
	Shown.CaptionAge = View.phase == UGH_LOGIC_PHASE_CAPTION && Shown.Phase == UGH_LOGIC_PHASE_CAPTION
		? Shown.CaptionAge + Seconds : 0;
	PlaySeconds = View.phase == UGH_LOGIC_PHASE_PLAY && Shown.Phase == UGH_LOGIC_PHASE_PLAY ? PlaySeconds + Seconds : 0;
	Shown.Phase = View.phase;
	Shown.Players = Shown.Choice.Players;
	Shown.Level = View.level;
	Shown.Lives = View.lives;
	Shown.Multiplier = View.multiplier;
	Shown.Score = View.score;
	Shown.Energy = FMath::Clamp(float(View.energy) / FullEnergy, 0.f, 1.f);
	Shown.Shown = View.phase == UGH_LOGIC_PHASE_PLAY ? FMath::Clamp(float(View.fade) / UghShapes::FadeShown, 0.f, 1.f) : 0;
	const FUghPasswords& Passwords = Mode.GetPasswords();
	Shown.LevelPassword = View.level >= 0 && View.level < Passwords.LevelCount(Shown.Players)
		? Passwords.Get(Shown.Players, View.level) : FString();
	UpdateHelp(Mode, Seconds);
	UpdateNotice(Mode, Seconds);

	// the scores earned, where they were earned on the view
	Shown.Popups.Reset();
	for (const AUghEffects::FPopup& Popup : Mode.GetEffects()->GetPopups())
	{
		const FVector At = Project(Popup.Where);
		if (At.Z > 0 && Canvas->ClipX > 0 && Canvas->ClipY > 0)
		{
			Shown.Popups.Add({ FVector2D(At.X / Canvas->ClipX, At.Y / Canvas->ClipY), Popup.Points,
				float(Popup.Age / AUghEffects::PopupSeconds) });
		}
	}
	UpdateWarnings(Mode, Seconds);
}

void AUghHud::UpdateWarnings(const AUghGameMode& Mode, double Seconds)
{
	FUghUiState& Shown = *State;
	Shown.Warnings.Reset();
	const FUghSimulation& Simulation = Mode.GetSimulation();
	const ugh_logic_view& Current = Simulation.GetCurrent();
	const ugh_logic_view& From = UghBetween::From(Simulation.GetPrevious(), Current);
	const bool bPlay = !Mode.IsInMenu() && Current.phase == UGH_LOGIC_PHASE_PLAY && Current.level_id >= 0;
	for (int32 Player = 0; Player < UE_ARRAY_COUNT(WarningAge); ++Player)
	{
		// the logic's verdict on the copter now, over it where it is drawn
		const int32 Loudness = bPlay && Player < Current.copter_count
			? UghWarning::Of(Simulation.GetLogic(), Player) : 0;
		WarningAge[Player] = Loudness == 0 ? -1 : WarningAge[Player] < 0 ? 0 : WarningAge[Player] + Seconds;
		if (Loudness == 0 || Canvas->ClipX <= 0 || Canvas->ClipY <= 0)
		{
			continue;
		}
		const ugh_logic_copter& To = Current.copters[Player];
		const ugh_logic_copter& Was = Player < From.copter_count ? From.copters[Player] : To;
		const FVector2D Corner = UghBetween::Position(Was.x, Was.y, To.x, To.y, Simulation.Alpha());
		const FVector At = Project(UghShapes::ToWorld(Corner.X +
			(UghShapes::CopterBodyLeft + UghShapes::CopterBodyRight + 1) / 2.0, Corner.Y - UghWarning::Above, 0));
		if (At.Z > 0)
		{
			Shown.Warnings.Add({ FVector2D(At.X / Canvas->ClipX, At.Y / Canvas->ClipY), Loudness, WarningAge[Player] });
		}
	}
}

void AUghHud::UpdateHelp(const AUghGameMode& Mode, double Seconds)
{
	if (Mode.IsInMenu())
	{
		bFirstHelpDone = false;   // a new game shows it again
		State->Help = 0;
		return;
	}
	// at the first level (a game not started by a password): through its caption and a while into its play, unless F1
	// hides it
	bFirstHelpDone = bFirstHelpDone || PlaySeconds > FirstHelpSeconds || Mode.IsHelpWanted() != bLastHelpWanted;
	bLastHelpWanted = Mode.IsHelpWanted();
	const bool bWanted = Mode.IsHelpWanted() || (!bFirstHelpDone && State->Level == 0);
	State->Help = FMath::Clamp(State->Help + float((bWanted ? 1 : -1) * HelpRate * Seconds), 0.f, 1.f);
}

void AUghHud::UpdateNotice(const AUghGameMode& Mode, double Seconds)
{
	State->NoticeAge += Seconds;
	if (LastVolume >= 0 && State->Volume != LastVolume)
	{
		State->Notice = FString::Printf(TEXT("Volume %d %%"), State->Volume);
		State->NoticeLevel = State->Volume / 100.f;
		State->NoticeAge = 0;
	}
	else if (!LastUpscaler.IsEmpty() && State->Upscaler != LastUpscaler)
	{
		State->Notice = State->Upscaler;
		State->NoticeLevel = -1;
		State->NoticeAge = 0;
	}
	LastVolume = State->Volume;
	LastUpscaler = State->Upscaler;
}

void AUghHud::UpdateScreens(const AUghGameMode& Mode)
{
	FUghUiState& Shown = *State;
	const FUghMenu& Menu = Mode.GetMenu();
	const FUghProfile& Profile = Mode.GetProfile();
	const FUghPasswords& Passwords = Mode.GetPasswords();
	Shown.Volume = Profile.Settings.Volume;
	Shown.Settings = Profile.Settings;
	Shown.Scores = Profile.Scores;
	Shown.Options = Mode.GetDisplayOptions();
	for (int32 Players = 1; Players <= 2; ++Players)
	{
		const int32 Last = Profile.Scores.LastLevel(Players);
		Shown.LastLevels[Players - 1] = Last;
		Shown.LastPasswords[Players - 1] =
			Last >= 0 && Last < Passwords.LevelCount(Players) ? Passwords.Get(Players, Last) : FString();
	}
	Shown.MenuScreen = Menu.GetScreen();
	Shown.SettingsRow = Menu.GetSettingsMenu().GetRow();
	const FUghControlsMenu& Controls = Menu.GetControlsMenu();
	Shown.ControlsRow = Controls.GetRow();
	Shown.ControlsColumn = Controls.GetColumn();
	Shown.bCapturing = Controls.IsCapturing();
	Shown.ControlsNotice = Controls.GetNotice();
	Shown.Highlight = Menu.GetHighlight();
	Shown.NameEntry = Menu.GetNameEntry();
	Shown.NewRank = Menu.GetNewRank();
}

void AUghHud::UpdateIsles(const AUghGameMode& Mode)
{
	FUghUiState& Shown = *State;
	const FUghIsles& Isles = Mode.GetMenu().GetIsles();
	Shown.Isles.Reset();
	using EStage = FUghIsles::EStage;
	const EStage Stage = Isles.GetStage();
	const double Time = Isles.GetStageTime(), Duration = Isles.GetStageDuration();
	// the numbers come as the flight over the archipelago nears its end, go as the flight to a stone begins
	Shown.IslesShown = !Mode.IsInMenu() ? 0.f
		: Stage == EStage::Arrive ? float(FMath::SmoothStep(0.3, 0.55, Duration > 0 ? Time / Duration : 1.0))
		: Stage == EStage::Choose ? 1.f
		: Stage == EStage::Approach ? float(1 - FMath::SmoothStep(0.0, 0.6, Time))
		: Stage == EStage::Leave ? float(1 - FMath::Clamp(Time / FUghIsles::FadeSeconds, 0.0, 1.0)) : 0.f;
	if (Shown.IslesShown <= 0 || !Canvas || Canvas->ClipX <= 0 || Canvas->ClipY <= 0)
	{
		return;
	}
	const FVector Eye = Isles.GetPose().Location;
	const TArray<FUghIslePlace>& Places = Isles.GetPlaces();
	for (int32 Level = 0; Level < Places.Num(); ++Level)
	{
		// over its flag
		const FVector Top = FUghIsles::Top(Places[Level]) + FVector(0, 0, NumberAbove);
		const FVector At = Project(Top);
		const FVector2D Where(At.X / Canvas->ClipX, At.Y / Canvas->ClipY);
		if (At.Z <= 0 || Where.X < -0.05 || Where.X > 1.05 || Where.Y < -0.05 || Where.Y > 1.05)
		{
			continue;
		}
		const float Size = FMath::Clamp(float(NumberNear / FMath::Max(FVector::Dist(Eye, Top), 1.0)), 0.4f, 1.f);
		Shown.Isles.Add({ Where, Size, Level, Isles.GetState(Level), Level == Isles.GetCursor() });
	}
	// the far ones first (the near ones over them), the cursor's last
	Shown.Isles.Sort([](const FUghUiIsle& A, const FUghUiIsle& B)
	{
		return A.bCursor != B.bCursor ? B.bCursor : A.Size < B.Size;
	});
	const int32 Players = Isles.GetPlayers();
	const FUghPasswords& Passwords = Mode.GetPasswords();
	Shown.IslesCursor = Isles.GetCursor();
	Shown.IslesCursorState = Isles.GetState(Isles.GetCursor());
	Shown.IslesPassword = Shown.IslesCursor < Passwords.LevelCount(Players) ? Passwords.Get(Players, Shown.IslesCursor)
		: FString();
	Shown.IslesCount = Isles.GetCount();
	Shown.IslesDone = 0;
	for (int32 Level = 0; Level < Isles.GetCount(); ++Level)
	{
		Shown.IslesDone += Isles.GetState(Level) == EUghIsle::Done ? 1 : 0;
	}
	Shown.IslesNotice = Isles.GetNotice();
	Shown.IslesNoticeAge = Isles.GetNoticeAge();
}
