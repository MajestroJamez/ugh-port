// What the player set: the graphics, the sound, the flight, the camera's shake, the keys.
#pragma once

#include "CoreMinimal.h"
#include "UghKeyBindings.h"

class FJsonObject;

/** The upscalers (FUghUpscaler). */
enum class EUghUpscaler : uint8 { Dlss, Fsr, Tsr };

/**
 * The settings of the game (the menu's screen Settings, saved with the profile, FUghProfile): the quality preset
 * (UghGraphics: the engine's scalability groups and the heavy features of the diorama), the upscaler and the DLSS frame
 * generation (FUghUpscaler), the resolution and the window mode (none: as the engine started - the desktop's, the
 * command line's), the volumes (all, the music, the effects, the ambience), the flight to the stone at a level's start, the camera
 * shaken by an impact, the ghost of the level's best run, the pilots' keys (FUghKeyBindings).
 */
struct FUghSettings
{
	static constexpr int32 QualityLevels = 4, FrameGenerations = 4;
	/** The volumes change in steps of a tenth. */
	static constexpr int32 VolumeStep = 10;

	int32 Quality = 3;   // 0 low, 1 medium, 2 high, 3 epic
	EUghUpscaler Upscaler = EUghUpscaler::Dlss;   // where the GPU has no DLSS, FSR
	int32 FrameGeneration = 0;   // off, 2x, 3x, 4x (where supported)
	FIntPoint Resolution = FIntPoint::ZeroValue;   // zero: as the engine started
	int32 WindowMode = -1;   // EWindowMode::Type; -1: as the engine started
	int32 Volume = 100, Music = 100, Effects = 100, Ambience = 100;   // percent
	bool bIntro = true;   // the flight to the stone at a level's start
	bool bShake = true;   // the camera shaken a little by an impact (FUghImpacts)
	bool bGhost = true;   // the ghost of the level's best run (FUghGhost)
	FUghKeyBindings Keys = FUghKeyBindings::Defaults();

	static const TCHAR* QualityName(int32 Quality);
	static const TCHAR* UpscalerName(EUghUpscaler Upscaler);
	static const TCHAR* FrameGenerationName(int32 FrameGeneration);
	static const TCHAR* WindowModeName(int32 WindowMode);
	/**
	 * The quality preset a computer without a profile starts with (UghGraphics::RecommendedQuality): epic where the GPU
	 * has DLSS (an RTX: the upscaler and its frame generation carry it), low on an integrated GPU (a notebook's), else
	 * high.
	 */
	static int32 RecommendedQuality(bool bDlss, bool bIntegrated);

	TSharedRef<FJsonObject> ToJson() const;
	/** From JSON; what is missing or out of range keeps the default. */
	static FUghSettings FromJson(const FJsonObject& Json);

	bool operator==(const FUghSettings& Other) const = default;
};
