// What the game keeps between its runs.
#pragma once

#include "CoreMinimal.h"
#include "UghHighScores.h"
#include "UghSettings.h"

/**
 * What the game keeps between its runs: the settings (FUghSettings) and the high scores with the levels the last games
 * got to (FUghHighScores), one small JSON file in the user's Saved folder (`Saved/UghProfile.json` of the project, of
 * the packaged game) or `-UghProfile=<file>`. A missing file is the defaults; a broken one too (logged, and kept until
 * the next save overwrites it).
 */
struct FUghProfile
{
	static constexpr int32 Version = 1;

	FUghSettings Settings;
	FUghHighScores Scores;

	/** `-UghProfile=<file>`, else Saved/UghProfile.json. */
	static FString DefaultPath();
	/** Reads the file at `Path`: false and the reason when it is there but cannot be read (then the defaults). */
	bool Load(const FString& Path, FString& OutError);
	/** Writes it (its folder made); false and the reason when it cannot. */
	bool Save(const FString& Path, FString& OutError) const;

	bool operator==(const FUghProfile& Other) const = default;
};
