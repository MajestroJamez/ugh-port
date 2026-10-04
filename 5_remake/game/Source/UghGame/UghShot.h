// The game playing by itself for screenshots.
#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "ugh_logic.h"

class AUghGameMode;
class FUghMenu;
class FUghPasswords;

/**
 * -UghShot=<folder> [-UghShotLevels=<list>] [-UghShotAt=<seconds>] [-UghShotMenu]: the game takes screenshots by
 * itself (a check without a window, with -RenderOffscreen). The list is items <mode>:<first>[-<last>] separated by
 * commas, the mode 1p or team, the levels from 1 in the order of the mode (default 1p:1). For every level it starts a
 * game from the menu with the keys a player would press (the mode, the level's password, Enter), goes on from the
 * caption, keeps the copters hovering where they are when the level is fully shown, saves <folder>/<mode>-<NN>.png
 * `At` seconds later (and logs the frame rate meanwhile) and gives the game up (Esc), back to the menu. -UghShotMenu
 * first saves menu.png. It quits after the last level; a level that takes longer than LevelTimeLimit is left out (and
 * logged).
 *
 * For a look at the copters (the autopilot never picks a passenger up): -UghShotCargo=<look> shows them with a
 * passenger of the logic's cargo look sitting in the cabin, with -UghShotHanging hanging below instead (only the
 * picture: the logic is not changed); -UghShotCloseUp frames the copters instead of the screen, -UghShotFrame=<left>,
 * <top>,<width>,<height> that part of the screen (pixels: a look at the figures). Such a shot's name ends in
 * -cargo<look>, -hanging<look>, -closeup, -frame<left>_<top> (in this order).
 */
class FUghShot
{
public:
	/** What the game mode does after a frame. */
	enum class EAction : uint8 { None, TakeShot, Quit };

	/** Reads the command line; false when no shot is asked for. */
	bool Configure();
	/** Where the screenshot of the last TakeShot goes. */
	const FString& GetPath() const { return Path; }

	/** One frame of the autopilot: its keys go to the game mode. */
	EAction Tick(AUghGameMode& Mode, float DeltaSeconds);

	/** The view as the shot shows it: the copters with the passenger of -UghShotCargo. */
	void Dress(ugh_logic_view& View) const;
	/** The pixels to frame (in the play): -UghShotFrame its part of the screen, -UghShotCloseUp the copters. */
	TOptional<FBox2D> CloseUp(const ugh_logic_view& View) const;

private:
	static constexpr double CaptionKeyEvery = 0.3, MenuShotAfter = 1, AfterShot = 0.5, LevelTimeLimit = 60;
	/** A close-up shows this many pixels around the copters. */
	static constexpr double CloseUpMargin = 12;
	/** A hanging passenger reaches this many pixels below the body (the stone passenger, 1 px below it, 11 px high). */
	static constexpr double HangingBelow = 12;

	struct FTarget
	{
		int32 Players;
		int32 Level;   // from 0
	};

	/** Parses the list of -UghShotLevels; false when an item is not <mode>:<first>[-<last>]. */
	bool AddTargets(const FString& List);
	EAction TakeShot(const FString& Name);
	/** The next key that turns the menu into the target's game. */
	FKey MenuKey(const FUghMenu& Menu, const FUghPasswords& Passwords, const FTarget& Target) const;
	/** The copters hover: pedal while below the height they had when the level was fully shown. */
	void Hover(AUghGameMode& Mode, const ugh_logic_view& View);
	void ReleasePedals(AUghGameMode& Mode);
	static void Tap(AUghGameMode& Mode, const FKey& Key);

	FString Folder;
	FString Path;
	double At = 2;
	bool bMenuShot = false;
	int32 CargoLook = 0;      // -UghShotCargo
	bool bHanging = false;    // -UghShotHanging
	bool bCloseUp = false;    // -UghShotCloseUp
	TOptional<FBox2D> Frame;  // -UghShotFrame
	FString Suffix;           // of the shots' names
	TArray<FTarget> Targets;
	int32 Next = 0;            // the target being shot
	double TargetTime = 0;     // since it began
	double Wait = 0;           // nothing until then: a screenshot is being saved
	bool bShotTaken = false;   // of the target: give the game up
	int32 Phase = -1;          // of the last frame
	double PhaseTime = 0;      // how long it has been in it (in the play: since fully shown)
	int32 Frames = 0;          // drawn in that time: the frame rate of the level
	int32 HoverY[2] = { -1, -1 };   // 1/32 px
	bool bPedalling[2] = { false, false };
};
