#include "UghMenuView.h"

#include "UghShapes.h"
#include "UghStackField.h"

namespace
{
	/** Units: how far from the stone's middle the camera is, how high above the sea. */
	constexpr double Distance = 38000, Height = 3200;
	/** Degrees around the stone from straight in front of its face: the middle of the swing, how far either side. */
	constexpr double Around = -24, Swing = 16;
	/** Degrees: the lens across, how far left of the stone it looks (the stone right of the middle). */
	constexpr float FieldOfView = 42, LookLeft = 13;
	/** The exposure out in the daylight (EV), as at the start of the flight to the stone. */
	constexpr float Outdoors = -0.8f;
}

FVector UghMenuView::StoneMiddle()
{
	const FVector Middle = FUghStackField::Origin +
		FVector(FUghStackField::Columns, FUghStackField::Rows, FUghStackField::LayerCount) * FUghStackField::Cell / 2;
	return UghShapes::ToWorld(Middle.X, Middle.Y, Middle.Z * UghShapes::UnitsPerPixel);
}

FUghCameraPose UghMenuView::At(const FUghCameraPose& Game, double SeaZ, double Time)
{
	const FVector Middle = StoneMiddle();
	const double Angle = Around + Swing * FMath::Sin(UE_TWO_PI * Time / Period);
	const FVector Towards = FRotator(0, Game.Rotation.Yaw + Angle, 0).Vector();
	FUghCameraPose Pose;
	Pose.Location = Middle - Towards * Distance;
	Pose.Location.Z = SeaZ + Height;
	Pose.Rotation = (Middle - Pose.Location).Rotation();
	Pose.Rotation.Yaw -= LookLeft;
	Pose.FieldOfView = FieldOfView;
	Pose.MotionBlur = 0;
	Pose.ExposureBias = Outdoors;
	return Pose;
}
