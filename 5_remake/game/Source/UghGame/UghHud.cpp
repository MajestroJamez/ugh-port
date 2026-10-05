#include "UghHud.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "UghEffects.h"
#include "UghGameMode.h"
#include "UghShapes.h"
#include "UghUi.h"
#include "UghUiState.h"
#include "UghUiStyle.h"

namespace
{
	constexpr float FullEnergy = UGH_LOGIC_FULL_ENERGY;
	/** How fast the help comes and goes (its share a second). */
	constexpr double HelpRate = 4;
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
	Shown.Volume = Mode.GetVolumePercent();
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
