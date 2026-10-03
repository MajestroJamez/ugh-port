// The passwords of the levels.
#pragma once

#include "CoreMinimal.h"

class FJsonObject;

/**
 * The original's passwords (assets/levels.json): each level of a mode has one, in the order of the mode, and it
 * starts a game at that level. The modes have their own lists (one player 69 levels, the team 81).
 */
class FUghPasswords
{
public:
	/** Takes the passwords of the levels' file (UghJson::LevelsFile) read from `Path`; false and the reason when not. */
	bool Load(const FJsonObject& LevelsFile, const FString& Path, FString& OutError);

	/** How many levels the mode of `Players` (1 or 2) has. */
	int32 LevelCount(int32 Players) const { return Of(Players).Num(); }
	/** The password of `Level` (from 0) in the mode of `Players`. */
	const FString& Get(int32 Players, int32 Level) const { return Of(Players)[Level]; }
	/** The level `Password` starts at in the mode of `Players` (any case), INDEX_NONE when it is none of its. */
	int32 Find(int32 Players, const FString& Password) const;

private:
	const TArray<FString>& Of(int32 Players) const { return Players == 2 ? Team : OnePlayer; }

	TArray<FString> OnePlayer, Team;
};
