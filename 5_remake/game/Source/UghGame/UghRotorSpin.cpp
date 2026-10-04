#include "UghRotorSpin.h"

void FUghRotorSpin::Update(int32 Sprite, double Seconds)
{
	if (Seconds <= 0)
	{
		return;
	}
	// a step moves the rotor at most one sprite on; a lower one is the cycle beginning again
	const int32 Changed = LastSprite < 0 || Sprite == LastSprite ? 0 : Sprite > LastSprite ? Sprite - LastSprite : 1;
	LastSprite = Sprite;
	const double Keep = FMath::Exp(-Seconds / SmoothSeconds);
	SpritesPerSecond = SpritesPerSecond * Keep + Changed / Seconds * (1 - Keep);
	Turns = FMath::Fmod(Turns + Speed() * Seconds, RotorTurnsPerPedal);
}
