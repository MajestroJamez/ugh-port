// Where the springs of a level are, their streams and their waterfalls.
#pragma once

#include "CoreMinimal.h"

class FUghRockField;
struct FUghPadSign;
struct ugh_logic;

/**
 * A spring of a level and the way of its water - only the frontend's: the logic knows nothing of it, nothing of it
 * touches the plane of the play. The water comes out of a hole in the cave's back wall above a ledge, runs across the
 * ledge towards the camera in a stream, under a footbridge of logs where the figures walk along the ledge (its top
 * the ledge's surface: the deck lies on the rock of the mask), over the edge of the rock's face and down the face into
 * the sea as a waterfall, which falls in front of rock only (the mask is solid all the way down to the water there).
 * Pixels of the screen (y down); depths in units behind the plane of the play (negative towards the camera).
 */
struct FUghStream
{
	/** The ledge's row (solid, air above), the stream's banks (pixels). */
	int32 Y = 0;
	double Left = 0, Right = 0;
	/** The spring: the depth of its hole in the back wall (units), how high its bottom is above the ledge (pixels). */
	double Spring = 0, SpringHeight = 0;
	/** The stream's bed from the bridge back to the spring: x the depth (units), y and z the floor's y at its banks. */
	TArray<FVector> Bed;
	/** The bridge: its deck's ends (pixels), its front and back (units); its top is the ledge's surface. */
	double BridgeLeft = 0, BridgeRight = 0, BridgeFront = 0, BridgeBack = 0;
	/**
	 * The waterfall: the row of the water's surface it falls into (at the start of the level), and its depth (units)
	 * every FallStep pixels down from the ledge's surface to a pixel below that row - in front of the face.
	 */
	int32 Foot = 0;
	TArray<double> Fall;

	double X() const { return (Left + Right) / 2; }
	/** Where its water runs above the ledge on the screen (pixels): its bed, its spring's hole and a little around. */
	FBox2D Course() const;
	/**
	 * What of the screen it needs (pixels): the rock all through it from the ledge down to under the water, the air
	 * above the ledge for its spring; nothing of the play may be there but the figures walking over its bridge.
	 */
	FBox2D Area() const;

	bool operator==(const FUghStream& Other) const = default;
};

/**
 * The streams of a level: where a ledge has rock under it all the way down to the water (its face drops into the sea)
 * at least MinDrop pixels high, the back wall of the cave behind it, room above it, away from the pads (where copters
 * land), their boards and the cave's entrances; the highest such waterfall of the level, the same every time.
 */
namespace UghStreams
{
	/** Pixels: the stream's width, how far the bridge reaches beyond its banks, the rock around them (Area). */
	constexpr double Width = 7, Overhang = 2, Margin = 4;
	/** Pixels: the lowest waterfall, the room above the ledge, how far from a pad's end. */
	constexpr int32 MinDrop = 12, Room = 14, PadMargin = 16;
	/**
	 * Pixels: the waterfall's depth is known this often down its height; the spring's hole is looked for in the back
	 * wall this high above the ledge (FUghStream::SpringHeight: where its bed ends, rising into the wall).
	 */
	constexpr double FallStep = 0.5, SpringHeight = 2.5;
	/**
	 * Units: the bridge's deck reaches this far behind the plane of the play (behind the copters' bodies); the spring's
	 * water lands on its bed this far in front of the hole (where the bed ends).
	 */
	constexpr double BridgeBack = 60, Landing = 30;
	/**
	 * Pixels: the stream's channel is this deep under its bed (cut into the rock in front of and behind the slab of
	 * the play, never in it: the bridge's deck spans it there), the water in it this far under the ledge's surface.
	 */
	constexpr double ChannelDepth = 1.5, WaterDown = 0.9;
	/** Pixels: the channel is cut into the rock from this far in front of and behind the plane of the play on. */
	constexpr double ChannelFrom = 3;
	/** The spring's hole: this high over the channel at the bed's end (pixels), this deep into the wall (units). */
	constexpr double HoleHeight = 2.4, HoleDeep = 60;
	/** At most this many streams a level (UghWater::MaxFalls is enough for their feet). */
	constexpr int32 MaxStreams = 1;

	/** The streams of the level being played (its rock `Field`, its water's row at the start, the pads' boards). */
	TArray<FUghStream> Plan(const ugh_logic* Logic, const FUghRockField& Field, int32 WaterRow,
		const TArray<FUghPadSign>& Signs);
	/** Where nothing else may come nearer the plane of the play than Front (units) on the screen's Box (pixels). */
	struct FRoom
	{
		FBox2D Box;
		double Front;
	};
	/**
	 * What `Stream` keeps clear: its spring and its bed at any depth (nothing on it, nothing in front of its spring),
	 * its bridge nearer than behind its deck.
	 */
	TArray<FRoom> Rooms(const FUghStream& Stream);
	/**
	 * How far `Point` (x, y pixels of the screen, depth pixels) is inside the channel of `Stream` (pixels, negative
	 * outside): its bed's width, ChannelDepth under the floor, from the face to the wall and into the wall as its
	 * hole; nothing nearer the plane of the play than ChannelFrom.
	 */
	double Channel(const FUghStream& Stream, const FVector& Point);
	/** Where the waterfalls pour into the sea (UghWater): x, y the middle of each foot (world), w its width (units). */
	TArray<FVector4> Feet(const TArray<FUghStream>& Streams);
}
