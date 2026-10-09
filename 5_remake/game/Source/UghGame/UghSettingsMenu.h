// The menu's screen of the settings.
#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "UghSettings.h"

/** What the graphics can be set to on this computer and what they are now (UghGraphics::Options). */
struct FUghDisplayOptions
{
	bool bDlss = false;   // the GPU has DLSS
	TArray<int32> FrameGenerations = { 0 };   // the DLSS frame generations it supports (0 off)
	TArray<FIntPoint> Resolutions;   // of the screen, smallest first
	/** The resolution and the window mode now: shown while the settings set none. */
	FIntPoint Current = FIntPoint(1920, 1080);
	int32 CurrentWindowMode = 1;

	/** The upscaler the settings get: DLSS only where the GPU has it, else FSR. */
	EUghUpscaler UpscalerOf(const FUghSettings& Settings) const;
	FIntPoint ResolutionOf(const FUghSettings& Settings) const;
	int32 WindowModeOf(const FUghSettings& Settings) const;
};

/**
 * The screen Settings of the menu (FUghMenu): a row each - the graphics (the quality, the upscaler, the frame
 * generation, the resolution, the window), the sound (the volume, the music, the effects), the flight to the stone
 * at a level's start, the camera shaken by an impact, the ghost of the best run, the keys (the screen Controls), back.
 * Up and Down choose a row, Left and Right change it (Enter too, forwards), Enter on Controls opens it, Esc (a
 * gamepad's B) or Enter on Back goes back. Only the presses count; UghUi shows it.
 */
class FUghSettingsMenu
{
public:
	enum class ERow : uint8
	{
		Quality, Upscaler, FrameGeneration, Resolution, Window, Volume, Music, Effects, Intro, Shake, Ghost, Controls, Back
	};
	static constexpr int32 RowCount = 13;
	/** The rows of the parts of the screen begin at these: the graphics, the sound, the game. */
	static constexpr ERow Parts[] = { ERow::Quality, ERow::Volume, ERow::Intro };

	/** What a key did. */
	enum class EResult : uint8 { None, Changed, Controls, Back };

	EResult HandleKey(const FKey& Key, FUghSettings& Settings, const FUghDisplayOptions& Options);
	/** Opened from the title screen: the first row. */
	void Open() { Row = ERow::Quality; }
	ERow GetRow() const { return Row; }

	static const TCHAR* LabelOf(ERow Row);
	/** A row's value as the screen writes it (empty for Controls and Back). */
	static FString ValueOf(ERow Row, const FUghSettings& Settings, const FUghDisplayOptions& Options);
	/** A volume's row as a part 0 .. 1 (its bar), else -1. */
	static float LevelOf(ERow Row, const FUghSettings& Settings);
	/** What a row does, for the line under the screen. */
	static FString HintOf(ERow Row, const FUghSettings& Settings, const FUghDisplayOptions& Options);

private:
	/** The value of the row one step on (`Direction` 1 or -1); false when it cannot change. */
	bool Change(int32 Direction, FUghSettings& Settings, const FUghDisplayOptions& Options) const;

	ERow Row = ERow::Quality;
};
