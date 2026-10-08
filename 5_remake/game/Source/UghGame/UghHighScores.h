// The best games and where the last ones got to.
#pragma once

#include "CoreMinimal.h"
#include "Algo/BinarySearch.h"

class FJsonObject;

/**
 * The high scores: the ten best games of each mode (one player, the team), best first - a new score goes below the
 * same score reached earlier -, each with the name typed after the game, the level it got to, its difficulty and its
 * day; the level the last game of each mode got to (the menu offers its password) and the levels done in each mode
 * (the level selection, FUghIsles: done green, the level got to yellow); the last name typed (the next
 * entry starts with it). Saved with the profile (FUghProfile).
 */
struct FUghHighScores
{
	static constexpr int32 Size = 10, Modes = 2, MaxName = 10;

	struct FEntry
	{
		FString Name;
		uint32 Score = 0;
		int32 Level = 0;        // from 0, where the game ended
		int32 Difficulty = 1;   // 0 easy, 1 medium, 2 hard
		FString Day;            // yyyy-mm-dd

		bool operator==(const FEntry& Other) const = default;
	};

	TArray<FEntry> Tables[Modes];
	int32 LastLevels[Modes] = { -1, -1 };   // from 0; -1 no game yet
	TArray<int32> DoneLevels[Modes];        // from 0, sorted, each once
	FString LastName;
	/** A level number the profile takes (more than any mode has). */
	static constexpr int32 MaxLevels = 200;

	/** The table of a mode (`Players` 1 or 2). */
	const TArray<FEntry>& Table(int32 Players) const { return Tables[ModeOf(Players)]; }
	/** The place (from 0) a score would take in the mode's table, INDEX_NONE when it would not get in (or is 0). */
	int32 RankOf(int32 Players, uint32 Score) const;
	/** Puts the entry into the mode's table (its name cut to MaxName; the tenth falls out); its place or INDEX_NONE. */
	int32 Insert(int32 Players, const FEntry& Entry);

	/** The level the last game of the mode got to (-1 none). */
	int32 LastLevel(int32 Players) const { return LastLevels[ModeOf(Players)]; }
	void SetLastLevel(int32 Players, int32 Level) { LastLevels[ModeOf(Players)] = Level; }
	/** The levels of the mode done (a game went on from them to the next, or won at the last). */
	const TArray<int32>& Done(int32 Players) const { return DoneLevels[ModeOf(Players)]; }
	bool IsDone(int32 Players, int32 Level) const { return Algo::BinarySearch(Done(Players), Level) != INDEX_NONE; }
	/** Notes the level done; false when it was already. */
	bool SetDone(int32 Players, int32 Level);

	/** Today as an entry writes it. */
	static FString Today();

	TSharedRef<FJsonObject> ToJson() const;
	/**
	 * From JSON: the entries sorted again, at most Size, empty names and scores of 0 left out; a mode without its levels
	 * done (a profile before the level selection) has the levels before the one its last game got to done.
	 */
	static FUghHighScores FromJson(const FJsonObject& Json);

	bool operator==(const FUghHighScores& Other) const;

private:
	static int32 ModeOf(int32 Players) { return Players == 2 ? 1 : 0; }
};
