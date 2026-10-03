// The original's sounds and music, and which event of the logic plays which.
#pragma once

#include "CoreMinimal.h"

/**
 * The sounds of the original, rendered by the Kotlin port's OPL2 synthesizer into assets/sound/<name>.wav
 * (`.\gradlew.bat :extractor:sound`, 16-bit mono): the effects as the game plays them, the music and the flapping as
 * one pass that repeats without a seam. Which event of the logic plays which is the port's (`core/game`): the events
 * not in the table had no sound in the original (a crash, a passenger boarding, paying or in the water, a stunned
 * enemy, a collected bonus item, the level done). Without a file the game stays silent there (and logs it).
 */
class FUghSounds
{
public:
	/** How an event plays its sound. */
	enum class EAction : uint8
	{
		Play,   // once
		Loop,   // repeated until the event that stops it, for the same entity
		Stop    // stops the loop of the same sound and entity
	};

	/** The sound of an event of the logic. */
	struct FCue
	{
		int32 Event;        // UGH_LOGIC_EVENT_...
		const TCHAR* Name;  // the file's name without .wav
		EAction Action;
	};

	/** The music and the jingles the frontend plays around the logic's events. */
	static constexpr const TCHAR* MenuMusic = TEXT("music-menu");
	static constexpr const TCHAR* GameMusic = TEXT("music-game");
	static constexpr const TCHAR* EndingMusic = TEXT("music-ending");   // after the last level (with the high scores)
	static constexpr const TCHAR* GameOver = TEXT("game-over");         // "bad luck" after a lost game

	/** The sound of every event that had one in the original. */
	static TConstArrayView<FCue> Cues();
	/** The cue of an event of the logic, nullptr when it has no sound. */
	static const FCue* CueOf(int32 Event);
	/** Every file: the cues', the music, the jingles. */
	static TArray<const TCHAR*> Names();

	/** Reads every file of Names() from `Dir` (assets/sound); a missing or unreadable one is logged and stays silent. */
	void Load(const FString& Dir);

	/** The samples of a sound, nullptr when it is not loaded. */
	const TArray<int16>* Find(const TCHAR* Name) const { return Samples.Find(Name); }
	/** Samples a second of every sound; 0 before one is loaded. */
	int32 GetSampleRate() const { return SampleRate; }

	/**
	 * The samples of a 16-bit mono PCM WAV file and its rate; false (and why) when it is not one, or its rate is not
	 * `ExpectedRate` (0: any).
	 */
	static bool ReadWav(const FString& Path, int32 ExpectedRate, TArray<int16>& OutSamples, int32& OutRate,
		FString& OutError);

private:
	TMap<FString, TArray<int16>> Samples;
	int32 SampleRate = 0;
};
