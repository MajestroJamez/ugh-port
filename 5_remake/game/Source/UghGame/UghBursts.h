// The bursts the events of the logic show: what each is made of.
#pragma once

#include "CoreMinimal.h"

/** The bursts (UghBursts::Get): what an event of the logic shows (FUghEffectPlayer::Cues). */
enum class EUghBurst : uint8
{
	Splash,       // a body falling into the water: drops, spray, rings
	Surf,         // waves breaking on the stone along the water line (the caption, seen from the flight)
	Explosion,    // a copter crashing: fire, smoke, debris, sparks, a flash
	Dust,         // kicked up by a landing, a step, a fall
	Shells,       // shell money thrown up when a fare is paid, glinting
	Glints,       // a bonus: glints and motes of light (tinted by its kind), a flash
	Feathers,     // shaken off the flyer
	Downdraft,    // under the flyer's flapping wings, until it stops
	Gust,         // the blower's breath: leaves, streaks of wind, dust
	Thud,         // an enemy knocked out: dust and pebbles
	Leaves,       // falling from the tree's crown as it drops its fruit
	Celebration,  // the level done: petals and glints, a flash
	Rustle,       // a few leaves off the plants at the stone's edges a copter bumps into (AUghFringe; no event)
	Count
};

/**
 * What the bursts are made of: each a few parts, each part Count particles of one look and motion (the parameters of
 * UghMaterials::Burst); only decoration, the logic does not know them. They are short (a few seconds) and do not
 * hide a figure for long: the dust and smoke thin out as they spread.
 */
namespace UghBursts
{
	/** How a particle looks (Source/UghEditor/Shaders/UghBurstLook.hlsl's Shape). */
	enum class EShape : uint8 { Puff, Drop, Ring, Leaf, Feather, Shell, Chunk, Star, Petal, Fire };
	/** How it is drawn: translucent and lit (UghMaterials::Burst), cut out and lit (Bits), as light (Glint). */
	enum class EBlend : uint8 { Burst, Bits, Glint };
	/** How its quad turns (UghBurst.hlsl's Mode): to the camera, lying flat, tumbling (Bits only). */
	enum class EMode : uint8 { Facing, Flat, Tumbling };
	/** Where a burst happens, for a shot of it (FUghShot): in the air, on the ground, on the water. */
	enum class ERest : uint8 { Air, Ground, Water };

	/** A part of a burst (the parameters of UghMaterials::Burst; Strength its Opacity, or Intensity of a Glint). */
	struct FPart
	{
		EBlend Blend = EBlend::Burst;
		EShape Shape = EShape::Puff;
		EMode Mode = EMode::Facing;
		int32 Count = 10;
		float Life = 1, Stagger = 0;
		float Speed = 0, Spread = 1, Lift = 1;
		FVector3f Direction = FVector3f(0, 0, 1);
		float Gravity = 0, Drag = 0;
		float Size = 10, Grow = 1, Stretch = 0;
		float Flutter = 0, Spin = 0;
		FVector3f Box = FVector3f::ZeroVector;
		FLinearColor Color = FLinearColor::White;
		float Strength = 0.5f;
		float Roughness = 0.7f;
		bool bTinted = false;   // its colour times the event's tint (a bonus item's kind)
		bool bFacing = false;   // its Direction mirrored when the figure faces left
	};

	/** A flash of light with a burst: its colour, candelas at once, dying away in Seconds. */
	struct FFlash
	{
		FLinearColor Color = FLinearColor::Black;
		float Candelas = 0;
		float Seconds = 0;
	};

	struct FBurst
	{
		const TCHAR* Name;            // as -UghShotEffect names it
		TConstArrayView<FPart> Parts;
		bool bRepeat = false;         // goes on until its event stops it (a loop)
		FFlash Flash;
		double Extent = 20;           // pixels around its place a close-up of it shows
		double ShotAge = 0.3;         // seconds into it a shot shows it
		ERest Rest = ERest::Air;
		double Depth = 0;             // its depth (units, negative towards the camera: the surf before the cliff's face)
	};

	const FBurst& Get(EUghBurst Burst);
	/** The burst named `Name`, none for another name. */
	TOptional<EUghBurst> Find(const FString& Name);
	/** Seconds until the last particle of a burst played once is gone. */
	double Lasts(EUghBurst Burst);
}
