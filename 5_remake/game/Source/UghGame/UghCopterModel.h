// Where the parts of the copter's model are.
#pragma once

#include "CoreMinimal.h"

/**
 * The numbers of Blender/copter_layout.py in the world's units (cm) and axes, relative to the copter's origin: the
 * bottom middle of its body (UGH_LOGIC_COPTER_BODY_*), X right, Y towards the camera (the layout's -Y), Z up. The
 * meshes of the asset UghAssets::Copter have their origins where they turn: Body_<player> and Sling at the copter's
 * origin, Rotor_<player> at the hub (it turns about Z), Crank at the middle of its axle (it turns about X, the axle
 * turned by PilotYaw).
 */
namespace UghCopterModel
{
	/** The model's parts by their mesh names; the body and the rotor are the player's (1 or 2). */
	inline const TCHAR* const Bodies[] = { TEXT("Body_1"), TEXT("Body_2") };
	inline const TCHAR* const Rotors[] = { TEXT("Rotor_1"), TEXT("Rotor_2") };
	inline const TCHAR* Crank = TEXT("Crank");
	inline const TCHAR* Sling = TEXT("Sling");
	/** The stone passenger's mesh (UghAssets::StonePassenger), its origin at its bottom middle. */
	inline const TCHAR* StonePassenger = TEXT("stone_passenger");

	/** The body's half depth: it stays behind the rock's face (FUghRockMesh::FrontDepth) and the bubbles. */
	constexpr double BodyHalfDepth = 45;
	/**
	 * Where the pilot sits (the origin of his actions sit and pedal) and his passenger, and how each is turned (yaw,
	 * degrees) from facing the camera: the pilot to the left, the passenger to the right (seen sitting from a side).
	 */
	inline const FVector PilotSeat(-28, 0, 52);
	inline const FVector PassengerSeat(42, -20, 60);
	constexpr double PilotYaw = 50, PassengerYaw = -50;
	/**
	 * In the pilot's own frame (from his seat, facing +Y; PilotYaw turns it in the copter): the middle of the crank's
	 * axle (across, X), its pedals' radius and spread from the middle, the handle he holds with his left hand (the
	 * right one mirrored).
	 */
	inline const FVector PedalAxle(0, 32, -22);
	constexpr double PedalRadius = 8, PedalSpread = 11;
	inline const FVector Grip(20, 32, 34);
	/** The middle of the crank's axle in the copter: its turn about X follows PilotYaw. */
	inline const FVector CrankAxle = PilotSeat + FRotator(0, PilotYaw, 0).RotateVector(PedalAxle);
	inline const FVector RotorHub(0, 0, 187);
	/** The stone passenger's origin when it hangs in the sling (its middle at -63, half its height 53). */
	inline const FVector Hanging(0, 0, -116);
	/**
	 * When it rides in the cabin (nobody else there), it sits on the passenger's seat this much smaller, looking at the
	 * camera.
	 */
	constexpr double SeatedStone = 0.32;
	/**
	 * The crank turns this way about X for the pilot's action pedal: its left pedal (+X) from the top towards the
	 * camera (the layout's -Y).
	 */
	constexpr double CrankDirection = -1;
}
