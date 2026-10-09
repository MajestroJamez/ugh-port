// Plays the game's sounds.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UghSoundPlayer.h"
#include "UghSpeaker.generated.h"

class UAudioComponent;
class USoundWaveProcedural;

/**
 * Plays the stereo stream of the sound player (FUghSoundPlayer::MixStereo) through the engine: a procedural sound wave fed
 * every frame a little ahead of what the engine plays. Silent until Start (the autopilot of FUghShot never starts it),
 * and without the sound files or an audio device (-nosound).
 */
UCLASS()
class AUghSpeaker : public AActor
{
	GENERATED_BODY()

public:
	AUghSpeaker();
	virtual void Tick(float DeltaSeconds) override;

	/** Sound out from now on (the files loaded by GetPlayer().Load). */
	void Start();

	FUghSoundPlayer& GetPlayer() { return Player; }
	const FUghSoundPlayer& GetPlayer() const { return Player; }

private:
	/** What is queued ahead of the engine: at least this, or two frames. */
	static constexpr double MinLeadSeconds = 0.06, MaxLeadSeconds = 0.25;

	FUghSoundPlayer Player;
	TArray<int16> Buffer;
	UPROPERTY() TObjectPtr<USoundWaveProcedural> Wave;
	UPROPERTY() TObjectPtr<UAudioComponent> Audio;
};
