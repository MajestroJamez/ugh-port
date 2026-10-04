#include "UghBetween.h"

#include "UghShapes.h"

FVector2D UghBetween::Pixels(int32 X, int32 Y)
{
	return FVector2D(X, Y) / UghShapes::Subpixels;
}

bool UghBetween::Moved(int32 X0, int32 Y0, int32 X1, int32 Y1)
{
	const FVector2D Step = Pixels(X1, Y1) - Pixels(X0, Y0);
	return FMath::Abs(Step.X) <= MaxStepPixels && FMath::Abs(Step.Y) <= MaxStepPixels;
}

FVector2D UghBetween::Position(int32 X0, int32 Y0, int32 X1, int32 Y1, double Alpha)
{
	return Moved(X0, Y0, X1, Y1) ? FMath::Lerp(Pixels(X0, Y0), Pixels(X1, Y1), Alpha) : Pixels(X1, Y1);
}

const ugh_logic_view& UghBetween::From(const ugh_logic_view& Previous, const ugh_logic_view& Current)
{
	return Previous.phase == UGH_LOGIC_PHASE_PLAY && Previous.level_id == Current.level_id ? Previous : Current;
}
