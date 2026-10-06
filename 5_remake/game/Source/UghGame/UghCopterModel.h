// Where the parts of the copter's model are.
#pragma once

#include "CoreMinimal.h"

/**
 * The numbers of Blender/copter_layout.py in the world's units (cm) and axes, relative to the copter's origin: the
 * bottom middle of its body (UGH_LOGIC_COPTER_BODY_*), X right, Y towards the camera (the layout's -Y), Z up. The
 * meshes of the asset UghAssets::Copter have their origins where they turn: Body_<player> and Sling at the copter's
 * origin, Rotor_<player> and Shaft at the hub (they turn about Z), Crank at the middle of its axle and Drive at the
 * sprocket's middle (they turn about X, turned by PilotYaw), ChainLink at its middle (along Y).
 */
namespace UghCopterModel
{
	/** The model's parts by their mesh names; the body and the rotor are the player's (1 or 2). */
	inline const TCHAR* const Bodies[] = { TEXT("Body_1"), TEXT("Body_2") };
	inline const TCHAR* const Rotors[] = { TEXT("Rotor_1"), TEXT("Rotor_2") };
	inline const TCHAR* Shaft = TEXT("Shaft");
	inline const TCHAR* Crank = TEXT("Crank");
	inline const TCHAR* Drive = TEXT("Drive");
	inline const TCHAR* ChainLink = TEXT("ChainLink");
	inline const TCHAR* Sling = TEXT("Sling");

	/** The body's half depth: it stays behind the rock's face (FUghRockMesh::FrontDepth) and the bubbles. */
	constexpr double BodyHalfDepth = 45;
	/**
	 * Where the pilot sits (the origin of his actions sit and pedal) and his passenger, and how each is turned (yaw,
	 * degrees) from facing the camera: the pilot nearly to the left (pedalling seen almost from his side), the
	 * passenger to the right (seen sitting from a side).
	 */
	inline const FVector PilotSeat(-28, -7, 60);
	inline const FVector PassengerSeat(42, -15, 60);
	constexpr double PilotYaw = 70, PassengerYaw = -50;
	/**
	 * In the pilot's own frame (from his seat, facing +Y; PilotYaw turns it in the copter): the middle of the crank's
	 * axle (across, X), its pedals' radius and spread from the middle, the handle he holds with his left hand (the
	 * right one mirrored).
	 */
	inline const FVector PedalAxle(0, 34, -25);
	constexpr double PedalRadius = 12, PedalSpread = 11;
	inline const FVector Grip(22, 36, 36);
	inline const FVector RotorHub(0, 0, 187);

	/** The pilot's frame in the copter: X along the crank's axle (his left), Y forward. */
	inline const FQuat PilotTurn(FRotator(0, PilotYaw, 0));
	inline const FVector PilotAcross = PilotTurn.RotateVector(FVector::XAxisVector);
	inline const FVector PilotForward = PilotTurn.RotateVector(FVector::YAxisVector);
	/** The middle of the crank's axle in the copter: its turn about X follows PilotYaw. */
	inline const FVector CrankAxle = PilotSeat + PilotTurn.RotateVector(PedalAxle);

	/**
	 * The drive (copter_layout.py): a chain from the chainring on the crank's axle (ChainringSide along it, on the
	 * pilot's left, the camera's side) up to the sprocket on the layshaft at LayshaftHeight, parallel to the axle and
	 * pointing at the rotor's axis; its pinion turns the crown wheel on the rotor's shaft (Shaft) with as many pegs.
	 * The chainring has Ratio times the sprocket's teeth: the sprocket and the rotor turn Ratio times for a turn of
	 * the crank (FUghRotorSpin::RotorTurnsPerPedal), all the chain's way in the plane square to the axle.
	 */
	constexpr double ChainringSide = 32, ChainringRadius = 18, LayshaftHeight = 130, CrownRadius = 10;
	constexpr int32 ChainringTeeth = 18, Ratio = 3;
	constexpr double SprocketRadius = ChainringRadius / Ratio;
	inline const FVector Chainring = CrankAxle + PilotAcross * ChainringSide;
	inline const FVector Sprocket = PilotAcross * FVector::DotProduct(FVector(Chainring.X, Chainring.Y, 0), PilotAcross)
		+ FVector(0, 0, LayshaftHeight);
	/** The chain's links are at most this far apart along it (the chainring's teeth). */
	constexpr double ChainPitch = UE_TWO_PI * ChainringRadius / ChainringTeeth;

	/** The stone passenger's origin when it hangs in the sling (its middle at -63, half its height 53). */
	inline const FVector Hanging(0, 0, -116);
	/**
	 * When it rides in the cabin (nobody else there), it sits on the passenger's seat this much smaller, looking at the
	 * camera.
	 */
	constexpr double SeatedStone = 0.32;
	/**
	 * The crank turns this way about X for the pilot's action pedal: its left pedal (+X) from the top towards the
	 * camera (the layout's -Y), the top of the chainring forward; the chain and the sprocket go with it.
	 */
	constexpr double CrankDirection = -1;
}

/**
 * The chain of the copter's drive: its way round the chainring and the sprocket, and where its links are when the
 * crank has turned so far.
 */
class FUghCopterChain
{
public:
	FUghCopterChain();

	/** The chain's length along its links' middles (cm), and how many links it has. */
	double GetLength() const { return Length; }
	int32 GetLinks() const { return Links; }

	/**
	 * The links (ChainLink meshes in the copter's body) when the crank has turned `CrankTurn` (0 .. 1, as
	 * FUghRotorSpin::PedalTurn) by CrankDirection: the chain moves with the chainring's teeth, the same at 0 and 1.
	 */
	void Place(double CrankTurn, TArray<FTransform>& OutLinks) const;

	/** The point `Along` the chain (cm from where the chain leaves the chainring upwards) and its way there. */
	FTransform At(double Along) const;

private:
	FVector2D SprocketAt;   // in the chain's plane from the chainring: forward, up
	double Gamma = 0;       // the straight runs' slant against the line between the wheels
	double Beta = 0;        // that line's angle in the plane
	double Run = 0;         // a straight run's length
	double Length = 0;
	int32 Links = 0;
	int32 LinksPerTurn = 0;   // the links a turn of the crank moves the chain by
};
