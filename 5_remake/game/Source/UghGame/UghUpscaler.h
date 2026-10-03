// The upscaler and the frame generation.
#pragma once

#include "CoreMinimal.h"

/**
 * The upscaler at 67 % resolution: DLSS where the GPU supports it, else FSR, else TSR; U switches to the next one
 * that works, G cycles the DLSS frame generation (off, 2x, 3x, 4x where supported). Tried in the toolchain trial
 * (2_reverse_engineering/notes/phase4-modernization.md).
 */
class FUghUpscaler
{
public:
	/** The best upscaler the GPU supports. */
	void ChooseBest();
	/** The next upscaler (DLSS, FSR, TSR) that the GPU supports. */
	void Next();
	void NextFrameGeneration();

	/** The upscaler and the frame generation in use, for the HUD. */
	FString Describe() const;

private:
	enum class EKind : uint8 { Dlss, Fsr, Tsr };

	void Use(EKind Kind);

	EKind Kind = EKind::Tsr;
	int32 FrameGeneration = 0;   // index into the modes, 0 off
};
