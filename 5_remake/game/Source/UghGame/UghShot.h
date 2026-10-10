// The game playing by itself for screenshots.
#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "ugh_logic.h"
#include "UghKnockPilot.h"
#include "UghLively.h"

class AUghGameMode;
class FUghMenu;
class FUghPasswords;
class FUghReplay;
struct FUghGameEnd;
struct FUghHighScores;

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
 * passenger of the logic's cargo look sitting in the cabin, with -UghShotHanging hanging below instead (the stone, 4,
 * always hangs: the logic never puts it in the cabin; only the picture: the logic is not changed); -UghShotLand lets
 * the copters come down slowly instead of hovering, until they
 * stand on the ground (a pad) below; -UghShotCloseUp frames the copters instead of the screen, -UghShotFrame=<left>,
 * <top>,<width>,<height> that part of the screen (pixels: a look at the figures; with -UghShotCloseUp from the first
 * copter's corner, wherever it hovers: a look into its cabin). -UghShotBubbles gives every passenger shown a speech
 * bubble, each the next of the data's bubbles (only the picture). -UghShotLook=campfire (or torch) frames the first
 * campfire (torch) of the level, with -UghShotFrame that part from its middle. Such a shot's name ends in
 * -cargo<look>, -hanging<look>, -landed, -bubbles, -closeup, -campfire / -torch, -frame<left>_<top> (in this order).
 * -UghShotIntro=<seconds> saves the flight to the stone at the start of the level that many seconds into it instead
 * (FUghIntro; FUghIntro::Duration: its end, the game's camera; later: that much after its end, the caption still
 * shown), its name ending in -intro<seconds> after those, and gives the game up. -UghShotEdge=left|right|top flies the
 * first copter into that edge of the screen (beside it at the height -UghShotEdgeY pixels, else 20; the top pedalling
 * up) and takes the shot -UghShotEdgeAfter seconds (else 0.25) after it got there, its name ending in -edge<edge> after
 * -frame...: a look at the soft edges (AUghFringe). -UghShotEffect=<bursts> (names of UghBursts separated by commas, or
 * all) shoots each level once for each burst, held by the first copter when the play begins (beside it in the air, on
 * the ground or the water under it) -UghShotEffectAge seconds into it (else its ShotAge) and framed around it, the name
 * ending in -<burst> after the others: a look at the bursts of the events (AUghEffects), which come from no event then;
 * -UghShotEffectAt=<x>,<y> holds it at that place of the screen instead (pixels; the ground under it, the water below
 * it), the name ending in -at<x>_<y> before -<burst>; -UghShotWide shows it from the game's camera (-wide), not framed.
 * -UghShotFling=<seconds> (several separated by commas: the level shot once for each) lets the first copter knock the
 * first passenger on land off its pad (FUghKnockPilot) and takes the shot that many seconds after the logic knocked it
 * into the water (FUghFlings: flung towards the camera, its splash), the name ending in -fling<seconds> after the
 * others.
 * -UghShotDunk=<seconds> (several separated by commas: the level shot once for each) lets the first copter fly over
 * open water and fall into the sea (FUghDunkPilot) and takes the shot that many seconds after its waterline met the
 * surface (FUghDunks: its splash; negative: before it, as its free fall foretells), the name ending in -dunk<seconds>
 * after the others.
 * -UghShotDrop=<seconds> (several separated by commas: the level shot once for each) lets the first copter take the
 * stone on its sling, fly above the first enemy and let it go (FUghDropPilot), and takes the shot that many seconds
 * after the logic let it fall (the stone tumbling down, bouncing off the enemy), the name ending in -drop<seconds>
 * after the others.
 * -UghShotFare lets the first copter take a fare (FUghFarePilot) and takes the shot on its way to the pad the passenger
 * wants (the status shows it), the name ending in -fare.
 * -UghShotRush=left|right|down with -UghShotRushAfter=<seconds> (several separated by commas: the level shot once for
 * each) lets the first copter climb to the height -UghShotRushY (pixels, else 40), then fly that way as fast as it can
 * - sideways steering and pedalling to keep its height, down diving -, and takes the shot that many seconds after it
 * began, the name ending in -rush<way><seconds> after the others: a look at the copter fast (FUghMotionBlur), at the
 * warning over it.
 * -UghShotDifficulty=<0 .. 2> plays at that difficulty (easy, medium, hard; else the menu's medium).
 * -UghShotIsles=<moments> opens the level selection (FUghIsles; the autopilot skips it otherwise) for each level and
 * takes shots on the way to it (separated by commas: over:<seconds> into the flight over the archipelago, choose -
 * the cursor moved by its keys onto the level's stone, the camera at rest -, approach:<seconds> into the flight to the
 * stone, arrive its end before black), each named -isles-<moment> after the others (over2, choose, approach1.5,
 * arrive); then the level plays and is shot as ever. The password is typed only for a level the profile has locked.
 * -UghShotEnd gives each level up instead of its shot and saves the card of the game's end in the menu (the name
 * ending in -end after the others); -UghShotScore=<points> makes that game end with so many points (only the picture: a
 * score among the high scores shows the name being typed). -UghShotScreens=<screens> (settings, controls, scores, replays
 * separated by commas) first opens each screen of the menu by its keys and saves <folder>/<screen>.png. The game's
 * profile is the defaults (FUghProfile) or the one of -UghProfile, never saved; the replays (FUghReplays) only those of
 * -UghReplays, never written. -UghShotSaveReplay=<file> writes the replay of each level played (the logic's, when it
 * ended: given up) to the file (shot.ps1 -SaveReplay: a replay to watch, to show as a ghost). -UghShotWatch=<file>
 * watches that replay instead of playing levels and saves <folder>/watch-<mode>-<NN>.png `At` seconds after its play is
 * fully shown, then quits. -UghShotGhost=<file>: the ghost (FUghGhost) flies that replay in its level and mode instead
 * of the best one kept.
 * -UghShotLively=idle|wave|duck|joy (FUghLively) takes the shot once a passenger on land shows that a while (a duck a
 * quarter of a second) instead, framed around it, the name ending in -<state> after the others: wave and duck with
 * the first copter flying into the first passenger on land (FUghKnockPilot: it waves at it coming, ducks before it
 * is knocked off), idle with the copters hovering; joy (a passenger delivered walking off glad) only watching a replay
 * of a level played (-UghShotWatch, instead of `At`).
 *
 * The frame times (perf.ps1): with the frame rate of a level's hover it logs its frames' median, 1 % low (the frame
 * rate of the slowest 1 % of its frames) and slowest frame, and the slowest frame and the hitches (frames over
 * HitchSeconds) of the whole level once it is seen (the flight, the hover; not while it is built and settles in the
 * black). -UghShotExec=<commands> runs console commands when the shot is taken (e.g. ProfileGPU: the GPU's passes of
 * that frame in the log): items separated by semicolons, one a level in turn (the next level drawn with them; the same
 * level repeated: settings compared in the same warmth of the GPU), each of commands separated by commas.
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

	/**
	 * The view of `Logic` as the shot shows it: the copters with the passenger of -UghShotCargo, the passengers with
	 * the bubbles of -UghShotBubbles.
	 */
	void Dress(ugh_logic_view& View, const ugh_logic* Logic);
	/**
	 * The pixels to frame (in the play): -UghShotFrame its part of the screen, -UghShotCloseUp the copters, both that
	 * part of the first copter; `Looked` (the middle of what -UghShotLook or -UghShotEffect wants, pixels) `Around` it,
	 * with -UghShotFrame that part from it.
	 */
	TOptional<FBox2D> CloseUp(const ugh_logic_view& View, const TOptional<FVector2D>& Looked,
		double Around = LookAround) const;
	/** A game's end as the shot shows it: with the points of -UghShotScore. */
	void DressEnd(FUghGameEnd& End) const;
	/** What -UghShotLook=<kind> wants framed: "campfire", "torch" (the first of its kind of the level), or nothing. */
	const FString& GetLook() const { return Look; }
	/** The burst (UghBursts) the level being shot shows held (-UghShotEffect), empty for none; how far into it. */
	FString GetEffect() const { return Next < Targets.Num() ? Targets[Next].Effect : FString(); }
	TOptional<double> GetEffectAge() const { return EffectAge; }
	/** Where the burst is held instead of by the first copter (-UghShotEffectAt, pixels); from the game's camera. */
	const TOptional<FVector2D>& GetEffectAt() const { return EffectAt; }
	bool IsWide() const { return bWide; }
	/** The camera shaken by an impact in the shots (-UghShotShake; else they stand still). */
	bool WantsShake() const { return bShake; }
	/** Where the replay of each level the autopilot played goes (-UghShotSaveReplay=<file>; the last one stays), empty none. */
	const FString& GetSaveReplay() const { return SaveReplay; }
	/** The replay the ghost flies in the level of its mode (-UghShotGhost=<file>), none: the store's best. */
	TSharedPtr<FUghReplay> LoadGhost() const;

	/** The level selection is shot (-UghShotIsles): the menu opens it. */
	bool WantsIsles() const { return !IslesShots.IsEmpty(); }
	/** The flight between two levels is shot (-UghShotVoyage): its archipelago is made at the start. */
	bool WantsVoyage() const { return !VoyageShots.IsEmpty(); }

	/** A frame longer than this is a hitch (seconds). */
	static constexpr double HitchSeconds = 0.05;

	/** A look at a decoration (-UghShotLook) shows this many pixels around its middle. */
	static constexpr double LookAround = 16;
	/** A lively passenger (-UghShotLively) is shot showing it this long (a duck: this long), this many pixels around. */
	static constexpr double LivelyAfter = 0.4, DuckAfter = 0.25, LivelyAround = 24;
	/** What the passengers' shot waits for (-UghShotLively), none. */
	const TOptional<EUghLively>& GetLively() const { return Lively; }

private:
	static constexpr double CaptionKeyEvery = 0.3, MenuShotAfter = 4, ScreenShotAfter = 1, EndShotAfter = 1,
		AfterShot = 0.5, LevelTimeLimit = 60;
	/** The level selection is chosen in at least this long (seconds): its frame rate measured meanwhile. */
	static constexpr double ChooseAtLeast = 2;
	/** The speech bubbles are looked for among the sprites below this one. */
	static constexpr int32 BubbleSearch = 1000;
	/** A close-up shows this many pixels around the copters. */
	static constexpr double CloseUpMargin = 12;
	/** A hanging passenger reaches this many pixels below the body (the stone passenger, 1 px below it, 11 px high). */
	static constexpr double HangingBelow = 12;
	/** -UghShotRush: the copter holds its height by where it will be this many steps on. */
	static constexpr int32 RushLookAhead = 8;
	/** -UghShotLand: the height the copters hover at goes down this fast (pixels a second). */
	static constexpr double LandSpeed = 12;

	struct FTarget
	{
		int32 Players;
		int32 Level;   // from 0
		FString Effect;   // the burst it shows (-UghShotEffect), empty none
		double Fling = -1;   // seconds after a passenger was knocked off (-UghShotFling), -1 none
		TOptional<double> Dunk;   // seconds after the copter fell into the sea (-UghShotDunk; negative before), none
		TOptional<double> Drop;   // seconds after the stone was let go (-UghShotDrop), none
		TOptional<double> Rush;   // seconds after the copter began to rush (-UghShotRush), none
	};

	/**
	 * Parses the list of -UghShotLevels (each level once a burst of -UghShotEffect); false when an item is not
	 * <mode>:<first>[-<last>].
	 */
	bool AddTargets(const FString& List);
	/** The screenshot's name of `Target`: its mode and level, the suffix of the options, its burst. */
	FString NameOf(const FTarget& Target) const;
	EAction TakeShot(const FString& Name);
	/** Logs the frame times of the level's hover and of the whole level. */
	void LogFrames(const ugh_logic_view& View) const;
	/** The next key that opens the menu's screen of -UghShotScreens (settings, controls, scores); none when it shows. */
	static FKey ScreenKey(const FUghMenu& Menu, const FString& Screen);
	/**
	 * The next key that turns the menu into the target's game (with the level selection: its password only when the
	 * profile's `Scores` have the level locked).
	 */
	FKey MenuKey(const FUghMenu& Menu, const FUghPasswords& Passwords, const FUghHighScores& Scores,
		const FTarget& Target) const;
	/** A shot of the level selection on the way to a level (-UghShotIsles). */
	struct FIslesShot
	{
		FString Name;      // over2, choose, approach1.5, arrive
		uint8 Stage = 0;   // FUghIsles::EStage
		double At = 0;     // seconds into it; Choose: once at rest on the target, the end of the approach: -1
	};
	/** In the level selection: its shots, the cursor's keys to the target's stone, Enter there. */
	EAction IslesTick(AUghGameMode& Mode, const FTarget& Target, double Seconds);
	/** The copters hover: pedal while below the height they had when the level was fully shown. */
	void Hover(AUghGameMode& Mode, const ugh_logic_view& View, double Seconds);
	/**
	 * -UghShotEdge: the first copter flies into the edge (left, right: at the height EdgeY, steering that way; top:
	 * pedalling up); true EdgeAfter seconds after it got there (the shot).
	 */
	bool FlyToEdge(AUghGameMode& Mode, const ugh_logic_view& View, double Seconds);
	/**
	 * -UghShotFling: the first copter knocks the first passenger on land off its pad (FUghKnockPilot), then hovers; true
	 * `Age` seconds after the logic knocked it into the water (the shot).
	 */
	bool Knock(AUghGameMode& Mode, const ugh_logic_view& View, double Seconds, double Age);
	/** -UghShotLively: a passenger on land has shown that long enough for its shot (FUghLively). */
	bool LivelyShown(const AUghGameMode& Mode) const;
	/**
	 * -UghShotDunk: the first copter flies over open water and falls into the sea (FUghDunkPilot); true `Age` seconds
	 * after its waterline met the surface (negative: before, as the free fall foretells it).
	 */
	bool Dunk(AUghGameMode& Mode, const ugh_logic_view& View, double Age);
	/**
	 * -UghShotDrop: the first copter takes the stone, flies above the first enemy and lets it go (FUghDropPilot); true
	 * `Age` seconds after the logic let it fall.
	 */
	bool Drop(AUghGameMode& Mode, const ugh_logic_view& View, double Seconds, double Age);
	/**
	 * -UghShotFare: the first copter takes a fare (FUghFarePilot: lands by the first passenger, it gets in, on to its
	 * pad); true FareShownAfter seconds after it got in (or when the pilot is lost).
	 */
	bool Fare(AUghGameMode& Mode, const ugh_logic_view& View, double Seconds);
	/**
	 * -UghShotRush: the first copter climbs to RushY, then rushes that way (sideways keeping its height, down diving);
	 * true `Age` seconds after it began.
	 */
	bool RushOn(AUghGameMode& Mode, const ugh_logic_view& View, double Seconds, double Age);
	/** Holds pilot 1's logic key `LogicKey` (UGH_LOGIC_KEY_UP, DOWN, LEFT, RIGHT, FIRE) or lets it go. */
	void Hold(AUghGameMode& Mode, int32 LogicKey, bool bHeld);
	void ReleasePedals(AUghGameMode& Mode);
	static void Tap(AUghGameMode& Mode, const FKey& Key);
	/** -UghShotWatch: the replay watched, shot `At` seconds after its play is fully shown, then stopped; then it quits. */
	EAction WatchTick(AUghGameMode& Mode, double Seconds);

	FString Folder;
	FString Path;
	double At = 2;
	bool bMenuShot = false;
	int32 CargoLook = 0;      // -UghShotCargo
	bool bHanging = false;    // -UghShotHanging
	bool bLand = false;       // -UghShotLand
	bool bBubbles = false;    // -UghShotBubbles
	TArray<int32> Bubbles;    // the sprites of the data's speech bubbles, found the first time
	bool bCloseUp = false;    // -UghShotCloseUp
	FString Look;             // -UghShotLook
	TOptional<FBox2D> Frame;  // -UghShotFrame
	TOptional<double> IntroAt; // -UghShotIntro
	FString Edge;              // -UghShotEdge: left, right, top
	double EdgeAfter = 0.25, EdgeY = 20;   // -UghShotEdgeAfter, -UghShotEdgeY
	double AtEdge = -1;        // seconds the copter has been at the edge, -1 not there
	FKey Steering;             // the key it holds to fly there
	TArray<FString> Effects;   // -UghShotEffect
	TOptional<double> EffectAge;   // -UghShotEffectAge
	TOptional<FVector2D> EffectAt;  // -UghShotEffectAt
	bool bWide = false;        // -UghShotWide
	bool bShake = false;       // -UghShotShake
	TArray<double> Flings;     // -UghShotFling
	double FlingTime = -1;     // since the passenger was knocked off, -1 not yet
	TArray<double> Dunks;      // -UghShotDunk
	FUghDunkPilot DunkPilot;   // of the level being shot
	TArray<double> Drops;      // -UghShotDrop
	FUghDropPilot DropPilot;   // of the level being shot
	double DropTime = -1;      // since the stone was let go, -1 not yet
	double BounceTime = -1;    // when it bounced off the enemy (seconds after it was let go), -1 not yet
	bool bFare = false;        // -UghShotFare
	FUghFarePilot FarePilot{ 0 };   // of the level being shot
	double FareTime = -1;      // since the passenger got in, -1 not yet
	/** -UghShotFare: shot this long after the passenger got in (on the way to its pad, the status shows it). */
	static constexpr double FareShownAfter = 1.5;
	FString Rush;              // -UghShotRush: left, right, down
	TArray<double> Rushes;     // -UghShotRushAfter
	double RushY = 40;         // -UghShotRushY
	double RushTime = -1;      // since the copter began to rush, -1 not yet
	int32 Difficulty = -1;     // -UghShotDifficulty, -1 the menu's
	bool bFiring = false;      // pilot 1's fire held by Drop
	bool bDiving = false;      // pilot 1's down held by RushOn
	bool bSteering[2] = { false, false };   // pilot 1's left and right held by Knock
	TArray<FIslesShot> IslesShots;   // -UghShotIsles
	TArray<FIslesShot> IslesLeft;    // of the target being shot
	double IslesKeyTime = 0;         // since the cursor's last key
	TArray<float> IslesFrames;       // the frame times over the archipelago (choosing)
	/** A shot of the flight between two levels (FUghVoyage): its name's end, seconds into it (-1: its arrival). */
	struct FVoyageShot
	{
		FString Name;
		double At = 0;
	};
	/**
	 * The flight between two levels flown: its shots due taken, the level left hovered until the logic moves on; true
	 * when they are all taken.
	 */
	bool VoyageTick(AUghGameMode& Mode, const ugh_logic_view& View, double Seconds, EAction& Action);
	TArray<FVoyageShot> VoyageShots;   // -UghShotVoyage
	TArray<FVoyageShot> VoyageLeft;    // of the target being shot (also the level selection's approach, arrive)
	bool bVoyageBegun = false;         // the target's flight began
	bool bVoyageEnds = false;          // its shots end the target (-UghShotVoyage; not the level selection's)
	TArray<float> VoyageFrames;        // its frame times
	bool bEndShot = false;     // -UghShotEnd
	bool bEndWanted = false;   // the target was given up: its end is to be shot
	TOptional<EUghLively> Lively;   // -UghShotLively
	TOptional<uint32> EndScore;   // -UghShotScore
	TArray<FString> Screens;   // -UghShotScreens still to be shot
	FString SaveReplay;        // -UghShotSaveReplay
	FString WatchFile;         // -UghShotWatch
	FString GhostFile;         // -UghShotGhost
	FString WatchName;         // its shot's
	bool bWatchStarted = false, bWatchShot = false;
	FString Suffix;           // of the shots' names
	TArray<FTarget> Targets;
	int32 Next = 0;            // the target being shot
	double TargetTime = 0;     // since it began
	double Wait = 0;           // nothing until then: a screenshot is being saved
	bool bShotTaken = false;   // of the target: give the game up
	int32 Phase = -1;          // of the last frame
	double PhaseTime = 0;      // how long it has been in it (in the play: since fully shown)
	int32 Frames = 0;          // drawn in that time: the frame rate of the level
	TArray<float> HoverFrames; // their times (seconds)
	TArray<float> LevelFrames; // the times of the target's frames since it left the menu
	FString Exec;              // -UghShotExec
	double HoverY[2] = { -1, -1 };   // 1/32 px
	bool bPedalling[2] = { false, false };
};
