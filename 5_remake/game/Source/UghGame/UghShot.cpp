#include "UghShot.h"

#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UghGameMode.h"
#include "UghMenu.h"
#include "UghPasswords.h"
#include "UghProfile.h"
#include "UghBursts.h"
#include "UghShapes.h"

namespace
{
	/** The key that keeps a pilot's copter up (the profile's). */
	FKey PedalKey(const AUghGameMode& Mode, int32 Player)
	{
		return Mode.GetProfile().Settings.Keys.KeyOf(Player, UGH_LOGIC_KEY_UP);
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
	bLand = FParse::Param(CommandLine, TEXT("UghShotLand"));
	bBubbles = FParse::Param(CommandLine, TEXT("UghShotBubbles"));
	bCloseUp = FParse::Param(CommandLine, TEXT("UghShotCloseUp"));
	FParse::Value(CommandLine, TEXT("-UghShotLook="), Look);
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
	if (bLand)
	{
		Suffix += TEXT("-landed");
	}
	if (bBubbles)
	{
		Suffix += TEXT("-bubbles");
	}
	if (bCloseUp)
	{
		Suffix += TEXT("-closeup");
	}
	if (!Look.IsEmpty())
	{
		Suffix += TEXT("-") + Look;
	}
	if (Frame)
	{
		Suffix += FString::Printf(TEXT("-frame%.0f_%.0f"), Frame->Min.X, Frame->Min.Y);
	}
	double Intro = 0;
	if (FParse::Value(CommandLine, TEXT("-UghShotIntro="), Intro))
	{
		IntroAt = Intro;
		Suffix += FString::Printf(TEXT("-intro%g"), Intro);
	}
	FString EffectList;
	if (FParse::Value(CommandLine, TEXT("-UghShotEffect="), EffectList, false))
	{
		EffectList.ParseIntoArray(Effects, TEXT(","));
		if (Effects == TArray<FString>{ TEXT("all") })
		{
			Effects.Reset();
			for (int32 Burst = 0; Burst < int32(EUghBurst::Count); ++Burst)
			{
				Effects.Add(UghBursts::Get(EUghBurst(Burst)).Name);
			}
		}
		Effects.RemoveAll([](const FString& Name)
		{
			if (UghBursts::Find(Name))
			{
				return false;
			}
			UE_LOG(LogTemp, Error, TEXT("UGH shot: no burst %s (UghBursts)"), *Name);
			return true;
		});
	}
	double Age = 0;
	if (FParse::Value(CommandLine, TEXT("-UghShotEffectAge="), Age))
	{
		EffectAge = Age;
	}
	bEndShot = FParse::Param(CommandLine, TEXT("UghShotEnd"));
	uint32 Score = 0;
	if (FParse::Value(CommandLine, TEXT("-UghShotScore="), Score))
	{
		EndScore = Score;
	}
	FString ScreenList;
	if (FParse::Value(CommandLine, TEXT("-UghShotScreens="), ScreenList, false))
	{
		ScreenList.ParseIntoArray(Screens, TEXT(","));
		Screens.RemoveAll([](const FString& Screen)
		{
			const bool bKnown = Screen == TEXT("settings") || Screen == TEXT("controls") || Screen == TEXT("scores");
			UE_CLOG(!bKnown, LogTemp, Error, TEXT("UGH shot: no screen %s (settings, controls, scores)"), *Screen);
			return !bKnown;
		});
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
			for (int32 Effect = 0; Effect < FMath::Max(1, Effects.Num()); ++Effect)
			{
				Targets.Add({ Mode == TEXT("team") ? 2 : 1, Level - 1, Effects.IsEmpty() ? FString() : Effects[Effect] });
			}
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
	if (!Screens.IsEmpty() && Mode.IsInMenu())
	{
		// the menu's screens by their keys, each shot once it shows
		const FKey Key = ScreenKey(Mode.GetMenu(), Screens[0]);
		if (Key.IsValid())
		{
			Tap(Mode, Key);
			PhaseTime = 0;
			return EAction::None;
		}
		PhaseTime += DeltaSeconds;
		if (PhaseTime < ScreenShotAfter)
		{
			return EAction::None;
		}
		PhaseTime = 0;
		const FString Name = Screens[0];
		Screens.RemoveAt(0);
		return TakeShot(Name);
	}
	if (Next == Targets.Num())
	{
		return EAction::Quit;
	}
	const FTarget& Target = Targets[Next];
	TargetTime += DeltaSeconds;
	if (Mode.IsInMenu() && bShotTaken && bEndWanted)
	{
		// given up for the card of the game's end
		PhaseTime += DeltaSeconds;
		if (PhaseTime < EndShotAfter)
		{
			return EAction::None;
		}
		bEndWanted = false;
		return TakeShot(NameOf(Target));
	}
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
	if (View.phase == UGH_LOGIC_PHASE_CAPTION && IntroAt)
	{
		// no key: the flight goes on to the moment of the shot (its end at the latest; not while it settles in black)
		const FUghIntro& Intro = Mode.GetIntro();
		if (Intro.GetTime() < FMath::Clamp(*IntroAt, UE_KINDA_SMALL_NUMBER, FUghIntro::Duration))
		{
			return EAction::None;
		}
		UE_LOG(LogTemp, Display, TEXT("UGH shot: the flight at %.2f s"), Intro.GetTime());
		bShotTaken = true;
		return TakeShot(NameOf(Target));
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
	Hover(Mode, View, DeltaSeconds);
	PhaseTime += DeltaSeconds;
	++Frames;
	if (PhaseTime <= At)
	{
		return EAction::None;
	}
	UE_LOG(LogTemp, Display, TEXT("UGH shot: level_id %d, copter %d,%d, %.0f fps"), View.level_id, View.copters[0].x,
		View.copters[0].y, Frames / PhaseTime);
	bShotTaken = true;
	if (bEndShot)
	{
		bEndWanted = true;   // the shot when the game was given up
		PhaseTime = 0;
		return EAction::None;
	}
	return TakeShot(NameOf(Target));
}

FString FUghShot::NameOf(const FTarget& Target) const
{
	const FString Effect = Target.Effect.IsEmpty() ? FString() : TEXT("-") + Target.Effect;
	return TargetName(Target.Players, Target.Level) + Suffix + Effect + (bEndShot ? TEXT("-end") : TEXT(""));
}

FUghShot::EAction FUghShot::TakeShot(const FString& Name)
{
	Path = Folder / Name + TEXT(".png");
	UE_LOG(LogTemp, Display, TEXT("UGH shot %s"), *Path);
	Wait = AfterShot;
	return EAction::TakeShot;
}

void FUghShot::Dress(ugh_logic_view& View, const ugh_logic* Logic)
{
	for (int32 Player = 0; CargoLook > 0 && Player < View.copter_count; ++Player)
	{
		View.copters[Player].cargo_look = CargoLook;
		View.copters[Player].destination = bHanging ? -1 : 1;
	}
	if (bBubbles && Bubbles.IsEmpty())
	{
		for (int32 Sprite = 0; Sprite < BubbleSearch; ++Sprite)
		{
			ugh_logic_sprite Info;
			if (ugh_logic_get_sprite(Logic, Sprite, &Info) && FCStringAnsi::Strstr(Info.name, "Bubble"))
			{
				Bubbles.Add(Sprite);
			}
		}
	}
	for (int32 Index = 0, Shown = 0; !Bubbles.IsEmpty() && Index < View.entity_count; ++Index)
	{
		ugh_logic_entity& Entity = View.entities[Index];
		if (Entity.kind == UGH_LOGIC_ENTITY_PASSENGER && Entity.sprite >= 0)
		{
			Entity.bubble = Bubbles[Shown++ % Bubbles.Num()];
		}
	}
}

TOptional<FBox2D> FUghShot::CloseUp(const ugh_logic_view& View, const TOptional<FVector2D>& Looked,
	double Around) const
{
	const bool bCopters = bCloseUp && View.copter_count > 0;
	if (View.phase != UGH_LOGIC_PHASE_PLAY || (!Frame && !bCopters && !Looked))
	{
		return {};
	}
	if (Looked)
	{
		// around it, a little more room above (its flame, its smoke)
		return Frame ? Frame->ShiftBy(*Looked)
			: FBox2D(*Looked - Around * FVector2D(1, 1.2), *Looked + Around * FVector2D(1, 0.8));
	}
	if (Frame)
	{
		return bCopters ? Frame->ShiftBy(FVector2D(View.copters[0].x, View.copters[0].y) / UghShapes::Subpixels) : Frame;
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

FKey FUghShot::ScreenKey(const FUghMenu& Menu, const FString& Screen)
{
	using EScreen = FUghMenu::EScreen;
	const EScreen Wanted = Screen == TEXT("settings") ? EScreen::Settings
		: Screen == TEXT("controls") ? EScreen::Controls : EScreen::Scores;
	const EScreen Shown = Menu.GetScreen();
	if (Menu.IsShowingEnd())
	{
		return EKeys::Escape;   // closes the card (carves a name)
	}
	if (Shown == Wanted)
	{
		return FKey();
	}
	if (Shown == EScreen::Title)
	{
		const FUghMenu::ERow Row = Wanted == EScreen::Scores ? FUghMenu::ERow::Scores : FUghMenu::ERow::Settings;
		return Menu.GetRow() == Row ? EKeys::Enter : EKeys::Down;
	}
	if (Shown == EScreen::Settings && Wanted == EScreen::Controls)
	{
		return Menu.GetSettingsMenu().GetRow() == FUghSettingsMenu::ERow::Controls ? EKeys::Enter : EKeys::Down;
	}
	return EKeys::Escape;   // back
}

void FUghShot::DressEnd(FUghGameEnd& End) const
{
	End.Score = EndScore.Get(End.Score);
}

FKey FUghShot::MenuKey(const FUghMenu& Menu, const FUghPasswords& Passwords, const FTarget& Target) const
{
	if (Menu.GetScreen() != FUghMenu::EScreen::Title || Menu.GetNameEntry())
	{
		return EKeys::Escape;   // back to the title (a name carved as it is)
	}
	if (Menu.GetRow() > FUghMenu::ERow::Play)
	{
		return EKeys::Up;   // Enter there would not play
	}
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

void FUghShot::Hover(AUghGameMode& Mode, const ugh_logic_view& View, double Seconds)
{
	for (int32 Player = 0; Player < View.copter_count; ++Player)
	{
		const int32 Y = View.copters[Player].y;
		const double Down = bLand ? LandSpeed * UghShapes::Subpixels * Seconds : 0;
		HoverY[Player] = HoverY[Player] < 0 ? Y : HoverY[Player] + Down;
		const bool bPedal = Y > HoverY[Player];
		if (bPedal != bPedalling[Player])
		{
			Mode.HandleKey(PedalKey(Mode, Player), bPedal ? IE_Pressed : IE_Released);
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
			Mode.HandleKey(PedalKey(Mode, Player), IE_Released);
			bPedalling[Player] = false;
		}
	}
}

void FUghShot::Tap(AUghGameMode& Mode, const FKey& Key)
{
	Mode.HandleKey(Key, IE_Pressed);
	Mode.HandleKey(Key, IE_Released);
}
