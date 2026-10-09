#include "UghSpeaker.h"

#include "AudioDevice.h"
#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Sound/SoundWaveProcedural.h"

AUghSpeaker::AUghSpeaker()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AUghSpeaker::Start()
{
	const int32 Rate = Player.GetSounds().GetSampleRate();
	if (Rate == 0)
	{
		return;   // no sound files: logged by FUghSounds
	}
	if (!GetWorld()->GetAudioDevice().IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("UGH silent: no audio device"));
		return;
	}
	Wave = NewObject<USoundWaveProcedural>(this);
	Wave->SetSampleRate(Rate);
	Wave->NumChannels = 2;   // the ambience around the original's sounds (FUghSoundPlayer::MixStereo)
	Wave->Duration = INDEFINITELY_LOOPING_DURATION;
	Wave->bLooping = false;
	Audio = NewObject<UAudioComponent>(this);
	Audio->SetSound(Wave);
	Audio->bAllowSpatialization = false;
	Audio->bIsUISound = true;
	Audio->RegisterComponent();
	Audio->Play();
}

void AUghSpeaker::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Audio)
	{
		return;
	}
	const int32 Rate = Player.GetSounds().GetSampleRate();
	const double Lead = FMath::Clamp(2.0 * DeltaSeconds, MinLeadSeconds, MaxLeadSeconds);
	const int32 Queued = Wave->GetAvailableAudioByteCount() / int32(2 * sizeof(int16));   // frames
	const int32 Wanted = FMath::RoundToInt(Lead * Rate) - Queued;
	if (Wanted <= 0)
	{
		return;
	}
	Buffer.SetNumUninitialized(2 * Wanted, EAllowShrinking::No);
	Player.MixStereo(Buffer);
	Wave->QueueAudio(reinterpret_cast<const uint8*>(Buffer.GetData()), Buffer.Num() * sizeof(int16));
}
