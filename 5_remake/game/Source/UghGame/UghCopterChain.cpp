#include "UghCopterModel.h"

using namespace UghCopterModel;

FUghCopterChain::FUghCopterChain()
{
	const FVector Between = Sprocket - Chainring;
	SprocketAt = FVector2D(FVector::DotProduct(Between, PilotForward), Between.Z);
	const double Distance = SprocketAt.Size();
	const double Difference = ChainringRadius - SprocketRadius;
	Beta = FMath::Atan2(SprocketAt.Y, SprocketAt.X);
	Gamma = FMath::Asin(Difference / Distance);
	Run = FMath::Sqrt(FMath::Square(Distance) - FMath::Square(Difference));
	Length = 2 * Run + SprocketRadius * (UE_PI - 2 * Gamma) + ChainringRadius * (UE_PI + 2 * Gamma);
	Links = FMath::CeilToInt32(Length / ChainPitch);
	LinksPerTurn = FMath::RoundToInt32(UE_TWO_PI * ChainringRadius / (Length / Links));
}

void FUghCopterChain::Place(double CrankTurn, TArray<FTransform>& OutLinks) const
{
	OutLinks.Reset(Links);
	const double Spacing = Length / Links;
	const double Moved = FMath::Frac(CrankTurn) * LinksPerTurn * Spacing;
	for (int32 Link = 0; Link < Links; ++Link)
	{
		OutLinks.Add(At(Link * Spacing + Moved));
	}
}

/**
 * The chain goes round clockwise as seen with forward to the right (the chainring's top goes forward): up the run on
 * the left of the line from the chainring to the sprocket, over the sprocket, down the other run, under the chainring.
 */
FTransform FUghCopterChain::At(double Along) const
{
	const double Left = Beta + UE_HALF_PI - Gamma, Right = Beta - UE_HALF_PI + Gamma;
	const FVector2D ToLeft(FMath::Cos(Left), FMath::Sin(Left)), ToRight(FMath::Cos(Right), FMath::Sin(Right));
	const double OverSprocket = SprocketRadius * (Left - Right);
	double S = FMath::Fmod(Along, Length);
	S = S < 0 ? S + Length : S;
	FVector2D Point, Way;
	auto Straight = [&](const FVector2D& From, const FVector2D& To, double Done) {
		Way = (To - From).GetSafeNormal();
		Point = From + Way * Done;
	};
	auto Round = [&](const FVector2D& Middle, double Radius, double Angle) {
		Point = Middle + Radius * FVector2D(FMath::Cos(Angle), FMath::Sin(Angle));
		Way = FVector2D(FMath::Sin(Angle), -FMath::Cos(Angle));
	};
	if (S < Run)
	{
		Straight(ToLeft * ChainringRadius, SprocketAt + ToLeft * SprocketRadius, S);
	}
	else if ((S -= Run) < OverSprocket)
	{
		Round(SprocketAt, SprocketRadius, Left - S / SprocketRadius);
	}
	else if ((S -= OverSprocket) < Run)
	{
		Straight(SprocketAt + ToRight * SprocketRadius, ToRight * ChainringRadius, S);
	}
	else
	{
		Round(FVector2D::ZeroVector, ChainringRadius, Right - (S - Run) / ChainringRadius);
	}
	const FVector Location = Chainring + PilotForward * Point.X + FVector::ZAxisVector * Point.Y;
	return FTransform(PilotTurn * FQuat(FVector::XAxisVector, FMath::Atan2(Way.Y, Way.X)), Location);
}
