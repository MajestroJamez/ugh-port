#include "UghFigurePlace.h"

#include "UghShapes.h"

namespace
{
	/** How far a model looking to a side turns towards the camera (degrees). */
	double TowardCamera(EUghModel Model)
	{
		switch (Model)
		{
		case EUghModel::Flyer: return 35;
		case EUghModel::Blower: return 25;
		default: return 20;
		}
	}

	/** The yaw of a model looking `Facing` (the models look at the camera, +Y, without a turn). */
	double Yaw(EUghModel Model, EUghFacing Facing)
	{
		switch (Facing)
		{
		case EUghFacing::Away: return 180;
		case EUghFacing::Left: return 90 - TowardCamera(Model);
		case EUghFacing::Right: return -90 + TowardCamera(Model);
		default: return 0;
		}
	}
}

FTransform UghFigurePlace::Of(const FUghFigureAction& Action, const FVector2D& At, const FIntPoint& Size, double Phase,
	double Spin)
{
	const double Middle = At.X + Size.X / 2.0, Bottom = At.Y + Size.Y;
	FQuat Turn(FVector::UpVector, FMath::DegreesToRadians(Yaw(Action.Model, Action.Facing)));
	FVector Scale = FVector::OneVector;
	FVector Origin = UghShapes::ToWorld(Middle, Bottom, 0);
	switch (Action.Model)
	{
	case EUghModel::Caveman:
	{
		const double Depth = Action.Door > 0 ? DoorDepth * (1 - Phase) : Action.Door < 0 ? DoorDepth * Phase : 0;
		Origin = UghShapes::ToWorld(Middle, Action.bInWater ? At.Y + WaterLine : Bottom, Depth);
		break;
	}
	case EUghModel::Flyer:
		Origin = UghShapes::ToWorld(Middle, At.Y + Size.Y / 2.0, 0);
		Turn = FQuat(FVector::XAxisVector, FMath::DegreesToRadians(-FlyerBank)) * Turn;
		Scale = FVector(FlyerScale);
		break;
	case EUghModel::Stone:
	{
		const FQuat Roll(FVector::YAxisVector, Spin);
		const FVector Up(0, 0, Size.Y * UghShapes::UnitsPerPixel / 2);
		Origin += Up - Roll.RotateVector(Up);
		Turn = Roll;
		break;
	}
	case EUghModel::BonusItem:
		Turn = FQuat(FVector::UpVector, Spin);
		break;
	default:
		break;
	}
	return FTransform(Turn, Origin, Scale);
}
