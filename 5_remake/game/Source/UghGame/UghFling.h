// A passenger a copter knocked off its pad, flung into the sea.
#pragma once

#include "CoreMinimal.h"
#include "ugh_logic.h"

class FUghSprites;

/** Where a flung passenger shows at a moment (FUghFlings::Of). */
struct FUghFlight
{
	double Depth = 0;      // units, negative towards the camera (UghShapes)
	double Lift = 0;       // pixels above where the logic has it
	bool bInAir = false;   // its feet not in the water yet: it flails
};

/** A splash where a flung passenger's feet reach the water. */
struct FUghFlingSplash
{
	int32 Passenger = INDEX_NONE;
	FVector2D Place = FVector2D::ZeroVector;   // pixels: its middle on the surface
	double Depth = 0;                          // units, negative towards the camera
	double Scale = 1;
};

/**
 * The passengers a copter knocked off their pads, flung into the sea in front of the stone. In the logic a passenger
 * a copter in the air touches on its pad (OnPickupPad) goes into the state Splash (the event PassengerInWater at
 * once): it falls straight down from where it stood - through whatever ledge or rock is below, it lands on nothing -,
 * faster and faster until it is a body's height under the water, is braked there, comes up and is put with its feet at
 * the surface, where it swims (a copter on the water may pick it up) or after a while sinks and is gone. Its x never
 * changes; only the logic's height is its.
 *
 * Seen here it is thrown towards the camera in an arc instead: across (x) and up and down where the logic has it, but
 * nearer to the camera the further it has fallen - as a body thrown level, the way towards the camera growing with
 * the time (the square root of the share of its fall) - and lifted a little in a hump at first (a thrown body under a
 * stronger gravity), flailing its arms; where its feet reach the water (EntryBelowTop below its top) it splashes in
 * front of the stone (FUghFlingSplash, the burst plunge, not the splash of the event), goes under (DiveShare deep),
 * and under the water goes back to where the logic has it (ReturnShare of the time of its fall), so that it comes up
 * and swims exactly where the logic has it: the fling is over when the logic has it afloat (its height still after
 * coming up). A fall shorter than MinDrop (the water rose to a passenger on its pad, or its pad is at the water) is
 * not flung. Only decoration: nothing goes back to the logic. Test Ugh.Fling (UghFlingTests.cpp).
 */
class FUghFlings
{
public:
	/** The feet of a flung passenger (falling with its arms up) are this many pixels below its sprite's top. */
	static constexpr double EntryBelowTop = 15;
	/** A shorter fall (pixels, from its top to where its feet reach the water) is no fling. */
	static constexpr double MinDrop = 6;
	/**
	 * How far towards the camera it lands (units): this many a pixel of its fall, within these, and at most RoomThrow a
	 * pixel of the room from the water down to Room pixels below the screen (seen nearer, its splash is seen lower).
	 */
	static constexpr double ThrowPerPixel = 12, MinThrow = 400, MaxThrow = 1500, RoomThrow = 50, Room = 20;
	/** How high it is thrown up at first (the hump, pixels): this share of its fall, at most. */
	static constexpr double LiftShare = 0.12, MaxLift = 14;
	/** Under the water it goes back to the logic's place in this share of the time of its fall (seconds within). */
	static constexpr double ReturnShare = 0.34, MinReturn = 0.25, MaxReturn = 0.8;
	/** Its splash is bigger the further it fell: 1 and this much a pixel, at most. */
	static constexpr double SplashPerPixel = 0.005, MaxSplash = 1.6;
	/**
	 * Under the water it dives this share of the logic's depth (the logic's head, HeadBelowTop under its top, goes a body
	 * deep and more; nobody plays with it there, and it is seen through the clear water in front of the stone).
	 */
	static constexpr double DiveShare = 0.5, HeadBelowTop = 4;
	/** A fling is forgotten after this long whatever the logic does (seconds). */
	static constexpr double MaxSeconds = 8;

	/** Which sprites of `Logic`'s data are passengers in the water ("kind1-water.standing"); `InSprites`: sizes. */
	void Load(const ugh_logic* Logic, const FUghSprites* InSprites);

	/** An event of the logic after the steps that led to `View`: a passenger knocked into the water high above it. */
	void OnEvent(const ugh_logic_event& Event, const ugh_logic_view& View);
	/** The views of the last step of a frame (after its events): where the passengers stand, the flings that end. */
	void OnView(const ugh_logic_view& Previous, const ugh_logic_view& Current);
	/** A frame between `Previous` and `Current` (Alpha), `Seconds` after the last: feet reaching the water splash. */
	void Show(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha, double Seconds);
	/** The splashes since the last call. */
	TArray<FUghFlingSplash> TakeSplashes();

	/** Where flung passenger `Passenger` shows, its top at `Top` (pixels, the logic's), the water at `Surface`. */
	TOptional<FUghFlight> Of(int32 Passenger, double Top, double Surface) const;
	bool IsFlung(int32 Passenger) const { return Flings.Contains(Passenger); }
	/** Seconds since the last fling began (a shot of it), none without one. */
	TOptional<double> Age() const;
	/** Seconds into the last fling its feet reached the water, none before. */
	TOptional<double> SplashAge() const;
	/** Forgets everything (a new level, an attempt, the menu). */
	void Reset();

private:
	struct FFling
	{
		double Start = 0;    // its top when it was knocked off (pixels)
		double Middle = 0;   // its middle across (pixels)
		double Throw = 0;    // units towards the camera where it lands
		double Lift = 0;     // pixels
		double Drop = 0;     // pixels from its top to where its feet reach the water
		double Age = 0;      // seconds
		double Splashed = -1;   // seconds into it its feet reached the water, -1 not yet
		double Return = 0;   // seconds it takes back under the water
		bool bRising = false;   // coming up again in the logic
	};

	bool IsInWater(int32 Sprite) const { return InWater.IsValidIndex(Sprite) && InWater[Sprite]; }
	const FFling* Last() const;

	const FUghSprites* Sprites = nullptr;
	TBitArray<> InWater;            // by sprite
	TMap<int32, FFling> Flings;     // by passenger
	TMap<int32, double> LandTops;   // the passengers' tops on land in the last view (pixels)
	TArray<FUghFlingSplash> Splashes;
	int32 Newest = INDEX_NONE;      // the passenger flung last
};
