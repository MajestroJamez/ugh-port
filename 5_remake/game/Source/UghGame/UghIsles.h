// The level selection: an archipelago of sea stacks, one a level.
#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "UghFlight.h"
#include "UghIntro.h"

struct FUghHighScores;

/**
 * Where progress has got to with a level: done (green), open - the one got to, or a password's - (yellow), locked
 * (red).
 */
enum class EUghIsle : uint8 { Done, Open, Locked };

/** A level's stone in the archipelago: where its foot stands (the world, at the sea), its size, turn and shape. */
struct FUghIslePlace
{
	FVector Foot = FVector::ZeroVector;
	double Scale = 1;
	double Yaw = 0;
	int32 Variant = 0;
	int32 Row = 0;      // from next to the title's stone (the first levels) out to the open sea
	int32 Column = 0;   // from the left
};

/**
 * The level selection (Jan, step 26): after the title the camera flies up over the stone the levels are carved into
 * (AUghSeaStack) and out over the open sea beyond it, where every level of the mode is a sea stack of an archipelago
 * (AUghArchipelago), in rows of PerRow from next to that stone out to the open sea, along a winding path (left to
 * right, then right to left above it: the order reads as the numbers on them), coloured by progress (EUghIsle: the
 * levels done green, the one the mode's games got to and a typed password's level yellow, the rest red and locked - the
 * original's passwords still open them). A cursor moves from stone to stone (the arrows or a gamepad's d-pad: left and
 * right along a row and on round its end, up and down to the nearest stone of the next row), the camera gliding after
 * it; Enter (A) on a done or open stone flies there - from a stop up to the speed and the place the level's own flight
 * (FUghIntro) starts from, in front of that stone, fading to black: the level starts (the logic's password path,
 * unchanged) and its flight goes on from black. Esc (B) goes back to the title. Any key hurries the flight over the
 * archipelago and the flight to a stone (UghFlight, as FUghIntro's). Only the frontend's: no logic.
 */
class FUghIsles
{
public:
	/** Stones a row; how far apart (units) along a row and between the rows; the first row this far beyond the stone. */
	static constexpr int32 PerRow = 9;
	static constexpr double ColumnGap = 19000, RowGap = 32000, Nearest = 70000;
	/** How many shapes of stone there are (AUghArchipelago), each its height above the sea (units, at scale 1). */
	static constexpr int32 Variants = 3;
	static constexpr double Heights[Variants] = { 6400, 7600, 5200 };
	/** A stone's half width at the sea at most (units, at scale 1; its relief and foot spread a little more). */
	static constexpr double StoneRadius = 3000;
	/** Seconds: the flight over the archipelago; the rest of a flight a key leaves; a fade to or from black. */
	static constexpr double ArriveSeconds = 7, HurrySeconds = 0.6, FadeSeconds = 0.3;
	/** The flight to a stone: at least, at most (seconds; as long as its way at the speed the level's flight begins). */
	static constexpr double ApproachMin = 2.5, ApproachMax = 5.5;
	/** The camera choosing: how far from the stone it looks at, how steeply down, its lens; how fast it glides (1/s). */
	static constexpr double ChooseDistance = 62000, ChoosePitch = 34, GlideRate = 4;
	static constexpr float ChooseFieldOfView = 52;

	enum class EStage : uint8 { Closed, Arrive, Choose, Approach, Arrived, Leave };
	/** What a key did: nothing for the menu, back to the title (closed), the flight to a stone begun. */
	enum class EResult : uint8 { None, Back, Fly };

	/**
	 * The flight to a stone ends this far (units) in front of it, the way the level's flight starts in front of its
	 * stone (FUghIntro): its stone looks about as big from there as the level's from its start, the row in front of it
	 * clear behind.
	 */
	static constexpr double ApproachReach = 22000;

	/**
	 * The stones of `Count` levels (the first nearest it) out at sea beyond `Home` (the middle of the stone the levels
	 * are carved into, seen from the title and the game in front of it), their feet at the world's z 0 (the screen's
	 * bottom; they reach under the sea): the same every time.
	 */
	static TArray<FUghIslePlace> Layout(int32 Count, const FVector& Home);
	/** The top of a stone: where its number shows. */
	static FVector Top(const FUghIslePlace& Place);
	/** How far progress has got with `Level` of the mode of `Players`: done, open (`Password`: a typed one's), locked. */
	static EUghIsle StateOf(const FUghHighScores& Scores, int32 Players, int32 Level, int32 Count,
		int32 Password = INDEX_NONE);
	/** The level the mode's games got to (yellow): the furthest got to, after the furthest done; 0 at first. */
	static int32 Current(const FUghHighScores& Scores, int32 Players, int32 Count);

	/**
	 * Opens the selection of the mode of `Players` with `Count` levels as the profile's `Scores` have them, the cursor on
	 * `Password`'s level (opened by it; INDEX_NONE none) or on the one got to; the camera flies from `From` (the title's)
	 * around `Home` on the sea at `SeaZ`. `bFlight` false: no flight there (straight to choosing).
	 */
	void Open(int32 Players, int32 Count, const FUghHighScores& Scores, int32 Password, const FUghCameraPose& From,
		const FVector& Home, double SeaZ, bool bFlight = true);
	/** Closed at once (a game began, a shot). */
	void Close() { Stage = EStage::Closed; }
	/** A key of the menu (FUghControls::MenuKeyOf; a gamepad's B as FUghControls::BackKey). */
	EResult HandleKey(const FKey& Key);
	/**
	 * `Seconds` on: the flights, the glide, the fades; the camera for the game's camera `Game` (the level's flight
	 * starts from it, FUghIntro) over the sea at `SeaZ`.
	 */
	void Advance(double Seconds, const FUghCameraPose& Game, double SeaZ);

	EStage GetStage() const { return Stage; }
	bool IsOpen() const { return Stage != EStage::Closed; }
	/** The cursor's level (from 0); the level chosen when Arrived. */
	int32 GetCursor() const { return Cursor; }
	/** The flight to the chosen stone is over (black): the game starts. */
	bool IsArrived() const { return Stage == EStage::Arrived; }
	int32 GetPlayers() const { return Players; }
	int32 GetCount() const { return Places.Num(); }
	const TArray<FUghIslePlace>& GetPlaces() const { return Places; }
	EUghIsle GetState(int32 Level) const { return States.IsValidIndex(Level) ? States[Level] : EUghIsle::Locked; }
	bool IsSelectable(int32 Level) const { return GetState(Level) != EUghIsle::Locked; }
	/** The camera now (after Advance). */
	const FUghCameraPose& GetPose() const { return Pose; }
	/** How much of the scene shows, 0 .. 1 (black at the end of the flight to a stone, going back). */
	double Shown() const;
	/** Seconds into the stage (a flight's own clock in a flight). */
	double GetStageTime() const;
	/** How long the stage's flight is (seconds; 0 not a flight). */
	double GetStageDuration() const;
	/** The cursor's glide has come to rest. */
	bool IsSettled() const { return Stage == EStage::Choose && Glide.Size() < SettledSpeed; }
	/** Why the last Enter did nothing (a locked stone), and how long ago (seconds); empty none. */
	const FString& GetNotice() const { return Notice; }
	double GetNoticeAge() const { return NoticeAge; }

	/** The camera choosing at `Level`'s stone (the world). */
	FUghCameraPose ChooseView(int32 Level, double SeaZ) const;
	/** The camera high over the middle of the archipelago (the flight over it passes there). */
	FUghCameraPose Overview(double SeaZ) const;
	/** Where the flight to `Level`'s stone ends: where the level's own flight starts, in front of that stone instead. */
	FUghCameraPose ApproachEnd(int32 Level, const FUghCameraPose& Game, double SeaZ) const;
	/** The cursor's next stone for an arrow (Left, Right, Up, Down), the same when there is none. */
	int32 Neighbour(int32 Level, const FKey& Arrow) const;

private:
	/** A glide this slow (units a second) has come to rest. */
	static constexpr double SettledSpeed = 20;

	FUghCameraPose FlightPose(const FUghCameraPose& Game, double SeaZ) const;

	EStage Stage = EStage::Closed;
	int32 Players = 1;
	int32 Cursor = 0;
	TArray<FUghIslePlace> Places;
	TArray<EUghIsle> States;
	FVector Home = FVector::ZeroVector;
	FUghCameraPose From;     // where the flight began
	FUghCameraPose Pose;     // now
	/**
	 * The flight over the archipelago: from a stop, braking to a stop where it chooses; the flight to a stone: from a
	 * stop up to the speed the level's own flight goes on at (no braking).
	 */
	FUghFlightClock ArriveClock{ ArriveSeconds, HurrySeconds, UghFlight::FProfile{ 0.25, 0.5 } };
	FUghFlightClock ApproachClock{ ApproachMax, HurrySeconds, UghFlight::FProfile{ 0.5, 1 } };
	double StageTime = 0;
	double ReturnAge = 1e9;   // seconds since it closed going back to the title (its fade from black)
	// the glide while choosing: the camera's place and the point it looks at, their speeds, the wanted ones
	FVector Eye = FVector::ZeroVector, Aim = FVector::ZeroVector, EyeSpeed = FVector::ZeroVector,
		AimSpeed = FVector::ZeroVector, EyeGoal = FVector::ZeroVector, AimGoal = FVector::ZeroVector;
	FVector Glide = FVector::ZeroVector;   // EyeSpeed, for IsSettled
	float Lens = ChooseFieldOfView;
	bool bNewStage = false;   // the first Advance of a stage (its start taken from the camera then)
	FString Notice;
	double NoticeAge = 1e9;
};
