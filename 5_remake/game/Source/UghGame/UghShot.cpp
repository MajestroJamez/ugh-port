#include "UghShot.h"

#include "InputCoreTypes.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UghKeyboard.h"
#include "UghShapes.h"
#include "UghSimulation.h"

bool FUghShot::Configure()
{
	const TCHAR* CommandLine = FCommandLine::Get();
	if (!FParse::Value(CommandLine, TEXT("-UghShot="), Path) && !FParse::Param(CommandLine, TEXT("UghShot")))
	{
		return false;
	}
	Path = FPaths::ConvertRelativePathToFull(Path.IsEmpty() ? FPaths::ProjectSavedDir() / TEXT("Shots/level1.png") : Path);
	FParse::Value(CommandLine, TEXT("-UghShotAt="), At);
	return true;
}

FUghShot::EAction FUghShot::Tick(FUghSimulation& Simulation, float DeltaSeconds)
{
	const ugh_logic_view& View = Simulation.GetCurrent();
	TotalTime += DeltaSeconds;
	if (TotalTime > TimeLimit)
	{
		UE_LOG(LogTemp, Error, TEXT("UGH shot: no screenshot after %.0f s (phase %d)"), TimeLimit, View.phase);
		return EAction::Quit;
	}
	if (View.phase != Phase)
	{
		UE_LOG(LogTemp, Display, TEXT("UGH shot: phase %d, %.0f fps"), View.phase, DeltaSeconds > 0 ? 1 / DeltaSeconds : 0.f);
		Phase = View.phase;
		PhaseTime = 0;
		HoverY = -1;
	}
	if (View.phase == UGH_LOGIC_PHASE_CAPTION)
	{
		PhaseTime += DeltaSeconds;
		if (PhaseTime >= CaptionKeyEvery)
		{
			FUghKeyboard::Handle(Simulation, EKeys::Enter, IE_Pressed);
			FUghKeyboard::Handle(Simulation, EKeys::Enter, IE_Released);
			PhaseTime = 0;
		}
		return EAction::None;
	}
	if (View.phase != UGH_LOGIC_PHASE_PLAY || View.fade < UghShapes::FadeShown)
	{
		return EAction::None;
	}
	// hovering: pedal while below the height it had when the level was fully shown
	HoverY = HoverY < 0 ? View.copters[0].y : HoverY;
	const bool bPedal = View.copters[0].y > HoverY;
	if (bPedal != bPedalling)
	{
		FUghKeyboard::Handle(Simulation, EKeys::Up, bPedal ? IE_Pressed : IE_Released);
		bPedalling = bPedal;
	}
	const double Before = PhaseTime;
	PhaseTime += DeltaSeconds;
	if (Before <= At && PhaseTime > At)
	{
		UE_LOG(LogTemp, Display, TEXT("UGH shot %s (level_id %d, copter %d,%d)"), *Path, View.level_id,
			View.copters[0].x, View.copters[0].y);
		return EAction::TakeShot;
	}
	return Before <= At + QuitAfterShot && PhaseTime > At + QuitAfterShot ? EAction::Quit : EAction::None;
}
