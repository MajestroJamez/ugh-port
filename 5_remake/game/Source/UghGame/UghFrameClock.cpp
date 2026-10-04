#include "UghFrameClock.h"

void FUghFrameClock::Update(int32 Frame, int32 Frames, double Seconds)
{
	Frames = FMath::Max(Frames, 1);
	const double Start = double(Frame) / Frames, End = double(Frame + 1) / Frames;
	if (Frame != LastFrame)
	{
		// the next frame of the animation: how long the last one took tells the speed
		if (LastFrame >= 0 && Frame == (LastFrame + 1) % Frames && Since > 0)
		{
			const double Keep = FMath::Exp(-Since / SmoothSeconds);
			FramesPerSecond = FramesPerSecond * Keep + (1 / Since) * (1 - Keep);
		}
		LastFrame = Frame;
		Since = 0;
		Position = Start;
	}
	Since += Seconds;
	Position = FMath::Min(Position + FramesPerSecond * Seconds / Frames, End - UE_KINDA_SMALL_NUMBER);
}
