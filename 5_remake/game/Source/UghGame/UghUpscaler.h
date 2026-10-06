// The upscaler and the frame generation.
#pragma once

#include "CoreMinimal.h"
#include "UghSettings.h"

/**
 * The upscaler (from the resolution of the quality preset, UghGraphics: 50 .. 67 %): DLSS where the GPU supports it,
 * else FSR, else TSR; the DLSS frame generation (off, 2x, 3x, 4x where supported). Set by the settings (FUghSettings);
 * in a game U switches to the next upscaler that works, G cycles the frame generation. Tried in the toolchain trial
 * (2_reverse_engineering/notes/phase4-modernization.md).
 */
class FUghUpscaler
{
public:
	/** The GPU has DLSS. */
	static bool HasDlss();
	/** The frame generations (FUghSettings::FrameGeneration) the GPU supports: 0 off, and 1 .. 3 (2x .. 4x). */
	static TArray<int32> FrameGenerations();

	/** `Kind`, where the GPU has it (DLSS: else FSR). */
	void Use(EUghUpscaler Kind);
	/** The next upscaler (DLSS, FSR, TSR) that the GPU supports. */
	void Next();
	/** A frame generation (0 off, 1 .. 3 2x .. 4x), off where the GPU does not support it. */
	void SetFrameGeneration(int32 FrameGeneration);
	/** The next frame generation the GPU supports. */
	void NextFrameGeneration();

	EUghUpscaler GetKind() const { return Kind; }
	int32 GetFrameGeneration() const { return FrameGeneration; }
	/** The upscaler and the frame generation in use, for the HUD. */
	FString Describe() const;

private:
	EUghUpscaler Kind = EUghUpscaler::Tsr;
	int32 FrameGeneration = 0;
};
