// Pictures of the screen carved in stone and bone, drawn by code.
#pragma once

#include "CoreMinimal.h"

/**
 * The pictures of the menu and the HUD (UghUi), made when the game starts: a shape given by its distance field is
 * lit as a carved, bevelled piece of stone (or bone, wood) with grain, cracks and moss, a dark rim and a soft shadow
 * below it - the logo "UGH!", the tablet of a level's caption, the copter of a life, the bone of the energy gauge.
 * Pixels row by row (sRGB, straight alpha) for UghTexture::Create.
 */
namespace UghStoneArt
{
	/** A shape: how far a point (pixels of the picture) is outside it, negative inside. */
	using FShape = TFunction<float(const FVector2f&)>;

	/** How a shape is carved and lit. */
	struct FLook
	{
		FLinearColor Light = FLinearColor(0.62f, 0.56f, 0.47f);   // the stone's lighter grain (linear)
		FLinearColor Dark = FLinearColor(0.24f, 0.21f, 0.18f);    // its darker grain
		float Bevel = 12;          // pixels: how wide its rounded edge is
		float Rough = 2;           // pixels: how ragged its outline is
		float Grain = 1;           // how much its surface is mottled and bumpy
		float Cracks = 1;          // how dark the cracks are
		float Moss = 0;            // how much moss grows on its upward faces
		float Feature = 40;        // pixels: the size of its grain
		float Rim = 1.5f;          // pixels of a dark rim around it
		FVector2f ShadowOffset = FVector2f(0, 6);
		float ShadowSoftness = 10; // pixels
		float ShadowOpacity = 0.6f;
		uint32 Seed = 1;
	};

	/** The shape carved and lit in a picture of `Width` x `Height` pixels. */
	TArray<FColor> Render(int32 Width, int32 Height, const FShape& Shape, const FLook& Look);

	/** The logo "UGH!": chunky stone letters, each a little tilted. */
	TArray<FColor> Logo(int32 Width, int32 Height);
	/** A slab of stone with ragged edges (the caption of a level). */
	TArray<FColor> Tablet(int32 Width, int32 Height);
	/** A pedal copter in bone (a life). */
	TArray<FColor> Copter(int32 Width, int32 Height);
	/** A bone lying across (the energy gauge's frame); its shaft is the middle two fifths of its height. */
	TArray<FColor> Bone(int32 Width, int32 Height);
	/** Black from the left fading to nothing towards the right (`Opacity` at the left). */
	TArray<FColor> Fade(int32 Width, float Opacity);
}
