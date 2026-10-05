#include "UghBubbles.h"

#include "UghPadSigns.h"
#include "ugh_logic.h"

namespace
{
	using FPoint = FVector2D;

	/** The tip of the tail on the left (pixels from the top left corner); the right one is mirrored. */
	const FPoint TailTip(0.5, 14.5);
	/** The tail: from its root at the bubble's bottom towards its tip, thinning; a curve of this many circles. */
	const FPoint TailRoot(4, 10.2), TailBend(2.2, 12.6);
	constexpr double TailRootRadius = 2.1, TailTipRadius = 0.35;
	constexpr int32 TailSteps = 16;
	/** The bubble's body, its outline, the board in it, a tally mark (and its dark rim), the question mark. */
	const FPoint Middle(8, 6.3), BodyHalf(7.6, 5.9), BoardHalf(5.3, 3.4);
	constexpr double BodyRound = 4.6, Outline = 0.45, BoardRound = 0.9, BoardRim = 0.4;
	constexpr double MarkRadius = 0.45, MarkRim = 0.25, MarkGap = 1.9, MarkLength = 4.2;
	constexpr double QuestionRadius = 2.1, QuestionThickness = 0.62, DotRadius = 0.78;
	const FPoint QuestionMiddle(8, 4.9);

	const FLinearColor White(0.99f, 0.99f, 0.98f), Shade(0.86f, 0.89f, 0.93f), Ink(0.22f, 0.25f, 0.31f);
	const FLinearColor Wood(0.59f, 0.41f, 0.24f), WoodRim(0.37f, 0.24f, 0.13f), Cut(0.96f, 0.87f, 0.7f);
	const FLinearColor CutRim(0.27f, 0.17f, 0.09f), Question(0.36f, 0.22f, 0.12f);

	/** Signed distances (pixels, negative inside). */
	double RoundBox(const FPoint& P, const FPoint& Centre, const FPoint& Half, double Round)
	{
		const FPoint Q = (P - Centre).GetAbs() - Half + FPoint(Round, Round);
		return FPoint(FMath::Max(Q.X, 0.0), FMath::Max(Q.Y, 0.0)).Size() + FMath::Min(FMath::Max(Q.X, Q.Y), 0.0) - Round;
	}

	double Capsule(const FPoint& P, const FPoint& A, const FPoint& B, double Radius)
	{
		const FPoint AB = B - A;
		const double Along = FMath::Clamp(FPoint::DotProduct(P - A, AB) / AB.SizeSquared(), 0.0, 1.0);
		return (P - A - AB * Along).Size() - Radius;
	}

	/** The union of A and B, rounded where they meet. */
	double Join(double A, double B, double Round)
	{
		const double H = FMath::Clamp(0.5 + 0.5 * (B - A) / Round, 0.0, 1.0);
		return FMath::Lerp(B, A, H) - Round * H * (1 - H);
	}

	/** The body with its tail on the left (P mirrored for one on the right). */
	double Body(const FPoint& P)
	{
		double Tail = TNumericLimits<double>::Max();
		for (int32 Step = 0; Step <= TailSteps; ++Step)
		{
			const double T = double(Step) / TailSteps;
			const FPoint On = TailRoot * FMath::Square(1 - T) + TailBend * 2 * T * (1 - T) + TailTip * T * T;
			Tail = FMath::Min(Tail, (P - On).Size() - FMath::Lerp(TailRootRadius, TailTipRadius, T));
		}
		return Join(RoundBox(P, Middle, BodyHalf, BodyRound), Tail, 0.8);
	}

	/** The tally marks of `Marks` as segments: upright ones, the fifth across them. */
	TArray<TPair<FPoint, FPoint>> MarkStrokes(int32 Marks)
	{
		TArray<TPair<FPoint, FPoint>> Strokes;
		const int32 Upright = FMath::Min(Marks, 4);
		for (int32 Index = 0; Index < Upright; ++Index)
		{
			const double X = Middle.X + (Index - (Upright - 1) / 2.0) * MarkGap;
			Strokes.Add({ FPoint(X, Middle.Y - MarkLength / 2), FPoint(X, Middle.Y + MarkLength / 2) });
		}
		if (Marks == UghPadSigns::MostMarks)
		{
			const double Reach = 1.5 * MarkGap + 0.9;
			Strokes.Add({ FPoint(Middle.X - Reach, Middle.Y + 1.3), FPoint(Middle.X + Reach, Middle.Y - 1.3) });
		}
		return Strokes;
	}

	/** The question mark: a hook from the left over the top down to the middle, a stem, a dot. */
	double QuestionMark(const FPoint& P)
	{
		double Distance = TNumericLimits<double>::Max();
		constexpr int32 Steps = 24;
		const double From = FMath::DegreesToRadians(165.0), To = FMath::DegreesToRadians(450.0);
		FPoint Last;
		for (int32 Step = 0; Step <= Steps; ++Step)
		{
			const double Angle = FMath::Lerp(From, To, double(Step) / Steps);
			const FPoint On = QuestionMiddle + FPoint(FMath::Cos(Angle), FMath::Sin(Angle)) * QuestionRadius;
			if (Step > 0)
			{
				Distance = FMath::Min(Distance, Capsule(P, Last, On, QuestionThickness));
			}
			Last = On;
		}
		Distance = FMath::Min(Distance, Capsule(P, Last, Last + FPoint(0, 1.3), QuestionThickness));
		return FMath::Min(Distance, (P - FPoint(Middle.X, 10.3)).Size() - DotRadius);
	}
}

FVector2D FUghBubblePlace::Tip() const
{
	return At + FPoint(bTailLeft ? TailTip.X : UghBubbles::Width - TailTip.X, TailTip.Y);
}

TOptional<FUghBubbleLook> UghBubbles::Look(const ugh_logic* Logic, int32 Sprite)
{
	ugh_logic_sprite Info;
	if (!ugh_logic_get_sprite(Logic, Sprite, &Info))
	{
		return {};
	}
	const FString Name(Info.name);
	if (Name == TEXT("impatientBubble"))
	{
		return FUghBubbleLook{ 0, true };
	}
	// the frame is the pad's index: the board of its number (the original's last one, of the sixth pad on, blank)
	if (Name == TEXT("destinationBubble"))
	{
		return FUghBubbleLook{ UghPadSigns::Marks(Info.frame + 1), false };
	}
	return {};
}

FUghBubblePlace UghBubbles::Place(const FVector2D& Passenger, double PassengerWidth)
{
	const double Right = Passenger.X + OffsetX;
	if (Right + Width <= UghShapes::ScreenWidth)
	{
		return { FPoint(Right, Passenger.Y + OffsetY), true };
	}
	return { FPoint(Passenger.X + PassengerWidth - OffsetX - Width, Passenger.Y + OffsetY), false };
}

TArray<FColor> UghBubbles::Draw(const FUghBubbleLook& Look, bool bTailLeft)
{
	constexpr int32 Columns = PictureWidth, Rows = PictureHeight;
	const TArray<TPair<FPoint, FPoint>> Strokes = MarkStrokes(Look.Marks);
	// how much of a texel a shape at distance D covers
	auto Cover = [](double D) { return float(FMath::Clamp(0.5 - D * Scale, 0.0, 1.0)); };
	TArray<FColor> Pixels;
	Pixels.SetNumUninitialized(Columns * Rows);
	for (int32 Row = 0; Row < Rows; ++Row)
	{
		for (int32 Column = 0; Column < Columns; ++Column)
		{
			const FPoint P((Column + 0.5) / Scale, (Row + 0.5) / Scale);
			const double Edge = Body(bTailLeft ? P : FPoint(Width - P.X, P.Y));
			FLinearColor Colour = FMath::Lerp(Ink, FMath::Lerp(White, Shade, float(P.Y / Height)), Cover(Edge + Outline));
			if (Look.bQuestion)
			{
				Colour = FMath::Lerp(Colour, Question, Cover(QuestionMark(P)));
			}
			else
			{
				const double Board = RoundBox(P, Middle, BoardHalf, BoardRound);
				Colour = FMath::Lerp(Colour, FMath::Lerp(WoodRim, Wood, Cover(Board + BoardRim)), Cover(Board));
				double Mark = TNumericLimits<double>::Max();
				for (const TPair<FPoint, FPoint>& Stroke : Strokes)
				{
					Mark = FMath::Min(Mark, Capsule(P, Stroke.Key, Stroke.Value, MarkRadius));
				}
				Colour = FMath::Lerp(Colour, FMath::Lerp(CutRim, Cut, Cover(Mark)), Cover(Mark - MarkRim));
			}
			Colour.A = Cover(Edge);
			Pixels[Row * Columns + Column] = Colour.ToFColor(false);
		}
	}
	return Pixels;
}
