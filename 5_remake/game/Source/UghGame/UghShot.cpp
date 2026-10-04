#include "UghShot.h"

#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UghGameMode.h"
#include "UghKeyboard.h"
#include "UghMenu.h"
#include "UghPasswords.h"
#include "UghShapes.h"

namespace
{
	/** The key that keeps a pilot's copter up. */
	FKey PedalKey(int32 Player)
	{
		return FUghKeyboard::KeyOf(Player, UGH_LOGIC_KEY_UP);
	}

	FString TargetName(int32 Players, int32 Level)
	{
		return FString::Printf(TEXT("%s-%02d"), Players == 2 ? TEXT("team") : TEXT("1p"), Level + 1);
	}
}

bool FUghShot::Configure()
{
	const TCHAR* CommandLine = FCommandLine::Get();
	if (!FParse::Value(CommandLine, TEXT("-UghShot="), Folder) && !FParse::Param(CommandLine, TEXT("UghShot")))
	{
		return false;
	}
	Folder = FPaths::ConvertRelativePathToFull(Folder.IsEmpty() ? FPaths::ProjectSavedDir() / TEXT("Shots") : Folder);
	FParse::Value(CommandLine, TEXT("-UghShotAt="), At);
	bMenuShot = FParse::Param(CommandLine, TEXT("UghShotMenu"));
	FParse::Value(CommandLine, TEXT("-UghShotCargo="), CargoLook);
	bHanging = FParse::Param(CommandLine, TEXT("UghShotHanging"));
	bCloseUp = FParse::Param(CommandLine, TEXT("UghShotCloseUp"));
	FString FrameText;
	TArray<FString> Numbers;
	if (FParse::Value(CommandLine, TEXT("-UghShotFrame="), FrameText, false) &&
		FrameText.ParseIntoArray(Numbers, TEXT(",")) == 4)
	{
		const FVector2D Corner(FCString::Atod(*Numbers[0]), FCString::Atod(*Numbers[1]));
		Frame = FBox2D(Corner, Corner + FVector2D(FCString::Atod(*Numbers[2]), FCString::Atod(*Numbers[3])));
	}
	if (CargoLook > 0)
	{
		Suffix += FString::Printf(TEXT("-%s%d"), bHanging ? TEXT("hanging") : TEXT("cargo"), CargoLook);
	}
	if (bCloseUp)
	{
		Suffix += TEXT("-closeup");
	}
	if (Frame)
	{
		Suffix += FString::Printf(TEXT("-frame%.0f_%.0f"), Frame->Min.X, Frame->Min.Y);
	}
	FString List = TEXT("1p:1");
	FParse::Value(CommandLine, TEXT("-UghShotLevels="), List, false);
	if (!AddTargets(List))
	{
		UE_LOG(LogTemp, Error, TEXT("UGH shot: -UghShotLevels=%s is not <1p|team>:<first>[-<last>],..."), *List);
		Targets.Reset();
	}
	return true;
}

bool FUghShot::AddTargets(const FString& List)
{
	TArray<FString> Items;
	List.ParseIntoArray(Items, TEXT(","));
	for (const FString& Item : Items)
	{
		FString Mode, Range, First, Last;
		if (!Item.Split(TEXT(":"), &Mode, &Range) || (Mode != TEXT("1p") && Mode != TEXT("team")))
		{
			return false;
		}
		if (!Range.Split(TEXT("-"), &First, &Last))
		{
			First = Last = Range;
		}
		const int32 From = FCString::Atoi(*First), To = FCString::Atoi(*Last);
		if (!First.IsNumeric() || !Last.IsNumeric() || From < 1 || To < From)
		{
			return false;
		}
		for (int32 Level = From; Level <= To; ++Level)
		{
			Targets.Add({ Mode == TEXT("team") ? 2 : 1, Level - 1 });
		}
	}
	return !Targets.IsEmpty();
}

FUghShot::EAction FUghShot::Tick(AUghGameMode& Mode, float DeltaSeconds)
{
	if (Wait > 0)
	{
		Wait -= DeltaSeconds;
		return EAction::None;
	}
	if (bMenuShot)
	{
		PhaseTime += DeltaSeconds;
		if (!Mode.IsInMenu() || PhaseTime < MenuShotAfter)
		{
			return EAction::None;
		}
		bMenuShot = false;
		PhaseTime = 0;
		return TakeShot(TEXT("menu"));
	}
	if (Next == Targets.Num())
	{
		return EAction::Quit;
	}
	const FTarget& Target = Targets[Next];
	TargetTime += DeltaSeconds;
	if (Mode.IsInMenu() && bShotTaken)
	{
		// given up after the shot: the next one
		++Next;
		TargetTime = 0;
		bShotTaken = false;
		Phase = -1;
		return EAction::None;
	}
	if (!bShotTaken &&
		(TargetTime > LevelTimeLimit || Target.Level >= Mode.GetPasswords().LevelCount(Target.Players)))
	{
		UE_LOG(LogTemp, Error, TEXT("UGH shot: no screenshot of %s"), *TargetName(Target.Players, Target.Level));
		bShotTaken = true;   // give it up
		return EAction::None;
	}
	if (Mode.IsInMenu())
	{
		Tap(Mode, MenuKey(Mode.GetMenu(), Mode.GetPasswords(), Target));
		return EAction::None;
	}
	if (bShotTaken)
	{
		// Esc after the pedals' releases: the last key event, so it gives the game up
		ReleasePedals(Mode);
		Mode.HandleKey(EKeys::Escape, IE_Pressed);
		return EAction::None;
	}

	const ugh_logic_view& View = Mode.GetSimulation().GetCurrent();
	if (View.phase != Phase)
	{
		UE_LOG(LogTemp, Display, TEXT("UGH shot: phase %d, %.0f fps"), View.phase,
			DeltaSeconds > 0 ? 1 / DeltaSeconds : 0.f);
		Phase = View.phase;
		PhaseTime = 0;
		Frames = 0;
		ReleasePedals(Mode);
		HoverY[0] = HoverY[1] = -1;
	}
	if (View.phase == UGH_LOGIC_PHASE_CAPTION)
	{
		PhaseTime += DeltaSeconds;
		if (PhaseTime >= CaptionKeyEvery)
		{
			Tap(Mode, EKeys::Enter);
			PhaseTime = 0;
		}
		return EAction::None;
	}
	if (View.phase != UGH_LOGIC_PHASE_PLAY || View.fade < UghShapes::FadeShown)
	{
		return EAction::None;
	}
	Hover(Mode, View);
	PhaseTime += DeltaSeconds;
	++Frames;
	if (PhaseTime <= At)
	{
		return EAction::None;
	}
	UE_LOG(LogTemp, Display, TEXT("UGH shot: level_id %d, copter %d,%d, %.0f fps"), View.level_id, View.copters[0].x,
		View.copters[0].y, Frames / PhaseTime);
	bShotTaken = true;
	return TakeShot(TargetName(Target.Players, Target.Level) + Suffix);
}

FUghShot::EAction FUghShot::TakeShot(const FString& Name)
{
	Path = Folder / Name + TEXT(".png");
	UE_LOG(LogTemp, Display, TEXT("UGH shot %s"), *Path);
	Wait = AfterShot;
	return EAction::TakeShot;
}

void FUghShot::Dress(ugh_logic_view& View) const
{
	for (int32 Player = 0; CargoLook > 0 && Player < View.copter_count; ++Player)
	{
		View.copters[Player].cargo_look = CargoLook;
		View.copters[Player].destination = bHanging ? -1 : 1;
	}
}

TOptional<FBox2D> FUghShot::CloseUp(const ugh_logic_view& View) const
{
	if (Frame && View.phase == UGH_LOGIC_PHASE_PLAY)
	{
		return Frame;
	}
	if (!bCloseUp || View.phase != UGH_LOGIC_PHASE_PLAY || View.copter_count == 0)
	{
		return {};
	}
	FBox2D Copters(ForceInit);
	for (int32 Player = 0; Player < View.copter_count; ++Player)
	{
		const FVector2D Corner = FVector2D(View.copters[Player].x, View.copters[Player].y) / UghShapes::Subpixels;
		Copters += Corner + FVector2D(UghShapes::CopterBodyLeft, 0);
		// the body, and below it a hanging passenger
		const double Bottom = UghShapes::CopterBodyHeight + (bHanging ? HangingBelow : 0);
		Copters += Corner + FVector2D(UghShapes::CopterBodyRight + 1, Bottom);
	}
	return Copters.ExpandBy(CloseUpMargin);
}

FKey FUghShot::MenuKey(const FUghMenu& Menu, const FUghPasswords& Passwords, const FTarget& Target) const
{
	if (Menu.GetChoice().Players != Target.Players)
	{
		return Menu.GetRow() == FUghMenu::ERow::Players ? EKeys::Right : EKeys::Up;
	}
	// the first level needs no password
	const FString Wanted = Target.Level == 0 ? FString() : Passwords.Get(Target.Players, Target.Level);
	const FString& Typed = Menu.GetPassword();
	if (Typed == Wanted)
	{
		return EKeys::Enter;
	}
	// (an empty prefix starts no string for FString)
	const bool bOnTheWay = Typed.IsEmpty() || Wanted.StartsWith(Typed, ESearchCase::CaseSensitive);
	return bOnTheWay ? FUghMenu::KeyOf(Wanted[Typed.Len()]) : EKeys::BackSpace;
}

void FUghShot::Hover(AUghGameMode& Mode, const ugh_logic_view& View)
{
	for (int32 Player = 0; Player < View.copter_count; ++Player)
	{
		const int32 Y = View.copters[Player].y;
		HoverY[Player] = HoverY[Player] < 0 ? Y : HoverY[Player];
		const bool bPedal = Y > HoverY[Player];
		if (bPedal != bPedalling[Player])
		{
			Mode.HandleKey(PedalKey(Player), bPedal ? IE_Pressed : IE_Released);
			bPedalling[Player] = bPedal;
		}
	}
}

void FUghShot::ReleasePedals(AUghGameMode& Mode)
{
	for (int32 Player = 0; Player < UE_ARRAY_COUNT(bPedalling); ++Player)
	{
		if (bPedalling[Player])
		{
			Mode.HandleKey(PedalKey(Player), IE_Released);
			bPedalling[Player] = false;
		}
	}
}

void FUghShot::Tap(AUghGameMode& Mode, const FKey& Key)
{
	Mode.HandleKey(Key, IE_Pressed);
	Mode.HandleKey(Key, IE_Released);
}
