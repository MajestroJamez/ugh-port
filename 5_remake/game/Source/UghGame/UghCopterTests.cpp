// The copters as automation tests of the editor: the rotor's spin (Ugh.Copter.Spin), the drive's chain
// (Ugh.Copter.Chain), the imported models (Ugh.Copter.Model).
#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Misc/AutomationTest.h"
#include "StaticMeshAttributes.h"
#include "UghAssets.h"
#include "UghCaveman.h"
#include "UghCopterModel.h"
#include "UghRotorSpin.h"
#include "UghShapes.h"
#include "UghSimulation.h"
#include "UghStoneDrop.h"

namespace
{
	/** The rotor's sprites of player 1 (assets/logic/ugh-data.ugd: rotor0). */
	constexpr int32 FirstSprite = 218, LastSprite = 223;
	constexpr double FrameSeconds = 1.0 / 60;

	/** Frames of `Seconds` at 60 fps over steps of the logic in which the sprite moves on every `StepsPerSprite`. */
	void Run(FUghRotorSpin& Spin, int32& Sprite, double& Time, double Seconds, int32 StepsPerSprite)
	{
		for (const double End = Time + Seconds; Time < End; Time += FrameSeconds)
		{
			const int32 Step = int32((Time + FrameSeconds) * FUghSimulation::TickRate);
			if (StepsPerSprite > 0)
			{
				Sprite = FirstSprite + (Step / StepsPerSprite) % (LastSprite - FirstSprite + 1);
			}
			Spin.Update(Sprite, FrameSeconds);
		}
	}

	/**
	 * The box of the copter's part `Name` (its vertices, not its own box turned) as it turns about X (`Turning`: at
	 * every 15 degrees) and then `Transform` places it.
	 */
	FBox PartBox(const TCHAR* Name, const FTransform& Transform, bool bTurning)
	{
		FBox Box(ForceInit);
		UStaticMesh* Mesh = UghAssets::Mesh(UghAssets::Copter, Name);
#if WITH_EDITOR
		const FMeshDescription* Description = Mesh ? Mesh->GetMeshDescription(0) : nullptr;
#else
		const FMeshDescription* Description = nullptr;
#endif
		if (!Description)
		{
			return Mesh ? Mesh->GetBoundingBox().TransformBy(Transform) : Box;
		}
		const FStaticMeshConstAttributes Attributes(*Description);
		const TVertexAttributesConstRef<FVector3f> Positions = Attributes.GetVertexPositions();
		for (int32 Step = 0; Step < (bTurning ? 24 : 1); ++Step)
		{
			const FTransform Turned = FTransform(FQuat(FVector::XAxisVector, UE_TWO_PI * Step / 24)) * Transform;
			for (const FVertexID Vertex : Description->Vertices().GetElementIDs())
			{
				Box += Turned.TransformPosition(FVector(Positions[Vertex]));
			}
		}
		return Box;
	}

	/** The sling sways up to this far (radians; AUghCopters). */
	constexpr double MaxSwayTested = 0.25;

	/** The body's box (cm, around the copter's origin) of UGH_LOGIC_COPTER_BODY_*. */
	FBox BodyBox()
	{
		const double Half = (UghShapes::CopterBodyRight - UghShapes::CopterBodyLeft + 1) * UghShapes::UnitsPerPixel / 2;
		return FBox(FVector(-Half, -UghCopterModel::BodyHalfDepth, 0),
			FVector(Half, UghCopterModel::BodyHalfDepth, UghShapes::CopterBodyHeight * UghShapes::UnitsPerPixel));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghRotorSpinTest, "Ugh.Copter.Spin",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUghRotorSpinTest::RunTest(const FString& Parameters)
{
	FUghRotorSpin Spin;
	int32 Sprite = FirstSprite;
	double Time = 0;
	Run(Spin, Sprite, Time, 3, 3);
	const double Expected = FUghSimulation::TickRate / 3 / FUghRotorSpin::SpritesPerTurn;
	TestTrue(FString::Printf(TEXT("turns as fast as its sprites (%.2f, wanted %.2f a second)"), Spin.Speed(), Expected),
		FMath::Abs(Spin.Speed() - Expected) < Expected * 0.15);
	const double Turn = Spin.RotorTurn();
	Run(Spin, Sprite, Time, 0.5, 3);
	const double Off = FMath::Frac(Spin.RotorTurn() - Turn - Expected * 0.5 + 0.5) - 0.5;   // turns, -0.5 .. 0.5
	TestTrue(FString::Printf(TEXT("turns on smoothly (%.2f turns off)"), Off), FMath::Abs(Off) < 0.15);
	TestTrue(TEXT("the crank turns slower"), FMath::IsNearlyEqual(
		FMath::Frac(Spin.PedalTurn() * FUghRotorSpin::RotorTurnsPerPedal), Spin.RotorTurn(), 1e-6));
	Run(Spin, Sprite, Time, 2, 0);
	TestTrue(FString::Printf(TEXT("stops when its sprites stop (%.3f)"), Spin.Speed()), Spin.Speed() < 0.01);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghCopterChainTest, "Ugh.Copter.Chain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUghCopterChainTest::RunTest(const FString& Parameters)
{
	using namespace UghCopterModel;
	TestEqual(TEXT("the rotor turns as many times for a turn of the crank as the chainring has the sprocket's teeth"),
		double(Ratio), FUghRotorSpin::RotorTurnsPerPedal);
	TestTrue(TEXT("the chain runs square to the crank's axle"),
		FMath::Abs(FVector::DotProduct(Sprocket - Chainring, PilotAcross)) < 0.01);
	TestTrue(TEXT("the layshaft points at the rotor's axis"),
		FMath::Abs(FVector::CrossProduct(FVector(Sprocket.X, Sprocket.Y, 0), PilotAcross).Size()) < 0.01 &&
		FVector2D(Sprocket).Size() > CrownRadius + 3);
	const FUghCopterChain Chain;
	TArray<FTransform> Links, Later;
	Chain.Place(0, Links);
	TestEqual(TEXT("a link a tooth at most"), Links.Num(), FMath::CeilToInt32(Chain.GetLength() / ChainPitch));
	const FBox Wanted = BodyBox().ExpandBy(FVector(0, -2, 0));
	for (int32 Link = 0; Link < Links.Num(); ++Link)
	{
		const FVector At = Links[Link].GetLocation();
		const FVector Next = Links[(Link + 1) % Links.Num()].GetLocation();
		TestTrue(FString::Printf(TEXT("link %d %s in the body, the next close"), Link, *At.ToCompactString()),
			Wanted.IsInside(At) && FVector::Dist(At, Next) <= ChainPitch + 0.01);
		const double ToChainring = FVector::Dist(At, Chainring), ToSprocket = FVector::Dist(At, Sprocket);
		TestTrue(FString::Printf(TEXT("link %d on the chain's way"), Link), ToChainring > ChainringRadius - 0.01 &&
			ToSprocket > SprocketRadius - 0.01);
		const FVector Along = Links[Link].GetRotation().GetAxisY();
		TestTrue(FString::Printf(TEXT("link %d along the chain"), Link),
			FMath::Abs(FVector::DotProduct(Along, (Next - At).GetSafeNormal())) > 0.8);
	}
	// it moves with the chainring's teeth, the top of the chainring going forward, and is the same after a turn
	Chain.Place(0.01, Later);
	int32 Lowest = 0;
	for (int32 Link = 1; Link < Links.Num(); ++Link)
	{
		Lowest = Links[Link].GetLocation().Z < Links[Lowest].GetLocation().Z ? Link : Lowest;
	}
	const FVector Moved = Later[Lowest].GetLocation() - Links[Lowest].GetLocation();
	TestTrue(FString::Printf(TEXT("under the chainring it goes back as fast as its teeth (%s)"),
		*Moved.ToCompactString()), FMath::IsNearlyEqual(FVector::DotProduct(Moved, -PilotForward),
			UE_TWO_PI * ChainringRadius * 0.01, 0.05));
	Chain.Place(1, Later);
	TestTrue(TEXT("the same after a turn of the crank"), Later[0].GetLocation().Equals(Links[0].GetLocation(), 0.01));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghCopterSlingTest, "Ugh.Copter.Sling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * The stone in the sling over the ground (UghSling): with room it hangs freely as ever; lower, at every height of the
 * copter over the ground down to standing on it and every sway, it is never in the ground; pushed aside (or towards
 * the camera) as far as it wants, it clears the body once its top is up at the body; it moves smoothly as the copter
 * comes down.
 */
bool FUghCopterSlingTest::RunTest(const FString& Parameters)
{
	using namespace UghCopterModel;
	const FBox Body = BodyBox();
	const FBox Stone(FVector(-StoneHalfWidth, -StoneHalfDepth, 0), FVector(StoneHalfWidth, StoneHalfDepth, StoneHeight));
	const FUghSlingPose Free = UghSling::Pose(0, 12, FVector2D::ZeroVector);
	TestTrue(TEXT("with room it hangs freely"), Free.Stone.Equals(Hanging) && Free.Sling.IsNearlyZero() &&
		Free.Lift == 0 && UghSling::Pushed(Free) == 0 && Free.Knot.Equals(SlingKnot));
	for (const FVector2D Way : { FVector2D(UghSling::SideClear, 0), FVector2D(-UghSling::SideClear, 0),
		FVector2D(0, UghSling::FrontClear) })
	{
		int32 InGround = 0, InBody = 0, Jumps = 0;
		FVector Last = Free.Stone;
		for (double Room = 14; Room >= 0; Room -= 0.05)
		{
			for (const double Sway : { -MaxSwayTested, 0.0, MaxSwayTested })
			{
				const double Pushed = UghSling::Pushed(UghSling::Pose(Sway, Room, FVector2D::ZeroVector));
				const FUghSlingPose Pose = UghSling::Pose(Sway, Room, Pushed * Way);
				const FBox Seen = Stone.TransformBy(FTransform(Pose.Turn, Pose.Stone));
				InGround += Seen.Min.Z < -Room * UghShapes::UnitsPerPixel - 0.01 ? 1 : 0;
				InBody += Sway == 0 && Pose.Stone.Z + StoneHeight >= UghSling::PushTo && Seen.Intersect(Body) ? 1 : 0;
				if (Sway == 0)
				{
					Jumps += FVector::Dist(Pose.Stone, Last) > 6 ? 1 : 0;   // 0.05 px lower: a few cm at most
					Last = Pose.Stone;
				}
			}
		}
		const FString Name = Way.ToString();
		TestEqual(FString::Printf(TEXT("%s: heights and sways with the stone in the ground"), *Name), InGround, 0);
		TestEqual(FString::Printf(TEXT("%s: heights with the stone in the body"), *Name), InBody, 0);
		TestEqual(FString::Printf(TEXT("%s: jumps of the stone as the copter comes down"), *Name), Jumps, 0);
	}
	const FUghSlingPose Landed = UghSling::Pose(0, 0, FVector2D(UghSling::SideClear, 0));
	TestTrue(FString::Printf(TEXT("landed it stands on the ground beside the copter %s"), *Landed.Stone.ToString()),
		FMath::IsNearlyZero(Landed.Stone.Z) && UghSling::Pushed(UghSling::Pose(0, 0, FVector2D::ZeroVector)) == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghCopterModelTest, "Ugh.Copter.Model",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUghCopterModelTest::RunTest(const FString& Parameters)
{
	using namespace UghCopterModel;
	FUghCaveman Caveman;
	UStaticMesh* Body = UghAssets::Mesh(UghAssets::Copter, Bodies[0]);
	if (!Body || !Caveman.Load())
	{
		AddInfo(TEXT("the copter's models are not imported (fetch-assets.ps1, build.ps1): nothing to check"));
		return true;
	}
	// the body fills the copter's body in the slab of the play and stays in it
	const FBox Wanted = BodyBox();
	for (const TCHAR* Name : Bodies)
	{
		const FBox Box = UghAssets::Mesh(UghAssets::Copter, Name)->GetBoundingBox();
		TestTrue(FString::Printf(TEXT("%s %s inside the body %s"), Name, *Box.ToString(), *Wanted.ToString()),
			Wanted.ExpandBy(FVector(1, 0.5, 1)).IsInside(Box));
		TestTrue(FString::Printf(TEXT("%s fills the body"), Name),
			Box.GetSize().X > Wanted.GetSize().X * 0.95 && Box.GetSize().Z > Wanted.GetSize().Z * 0.95);
	}
	for (const TCHAR* Name : Rotors)
	{
		const FBox Box = UghAssets::Mesh(UghAssets::Copter, Name)->GetBoundingBox().ShiftBy(RotorHub);
		TestTrue(FString::Printf(TEXT("%s at the top of the body %s"), Name, *Box.ToString()),
			Box.Min.Z > Wanted.Max.Z * 0.8 && Box.Max.Z < Wanted.Max.Z + 5);
	}
	// the drive's parts in the body, the model's wheels as big as the chain's way round them, the crown wheel's pegs
	// down to the pinion's top
	const TPair<const TCHAR*, FTransform> Turning[] = {
		{ Shaft, FTransform(RotorHub) }, { Crank, FTransform(PilotTurn, CrankAxle) },
		{ Drive, FTransform(PilotTurn, Sprocket) } };
	for (const TPair<const TCHAR*, FTransform>& Part : Turning)
	{
		// the shaft turns about Z, round as it is
		const FBox Box = PartBox(Part.Key, Part.Value, Part.Key != Shaft);
		TestTrue(FString::Printf(TEXT("%s %s inside the body as it turns"), Part.Key, *Box.ToString()),
			Box.IsValid && Wanted.ExpandBy(FVector(1, 0.5, 1)).IsInside(Box));
	}
	const FBox CrankBox = UghAssets::Mesh(UghAssets::Copter, Crank)->GetBoundingBox();
	TestTrue(FString::Printf(TEXT("the chainring %s"), *CrankBox.ToString()),
		FMath::IsNearlyEqual(CrankBox.Max.Z, ChainringRadius + 1.6, 1.0) && CrankBox.Max.X > ChainringSide);
	const FBox DriveBox = UghAssets::Mesh(UghAssets::Copter, Drive)->GetBoundingBox();
	TestTrue(FString::Printf(TEXT("the pinion %s meshes with the crown"), *DriveBox.ToString()),
		FMath::IsNearlyEqual(DriveBox.Max.Z, CrownRadius + 1.2, 1.0) &&
		FMath::IsNearlyEqual(DriveBox.Min.X, CrownRadius - FVector2D(Sprocket).Size() - 3.5, 1.0));
	const FBox ShaftBox = UghAssets::Mesh(UghAssets::Copter, Shaft)->GetBoundingBox().ShiftBy(RotorHub);
	TestTrue(FString::Printf(TEXT("the crown wheel %s"), *ShaftBox.ToString()),
		FMath::IsNearlyEqual(ShaftBox.Min.Z, LayshaftHeight + CrownRadius, 1.5) &&
		FMath::IsNearlyEqual(ShaftBox.Max.X, CrownRadius + 3, 1.0));
	// the stone passenger is as big as its sprite (16 x 11 px) and stands on its origin
	const FBox Stone = UghAssets::Stone()->GetBoundingBox();
	TestTrue(FString::Printf(TEXT("the stone passenger %s"), *Stone.ToString()),
		FMath::IsNearlyEqual(Stone.GetSize().X, 16 * UghShapes::UnitsPerPixel, 15.0) &&
		FMath::IsNearlyEqual(Stone.GetSize().Z, 11 * UghShapes::UnitsPerPixel, 15.0) && FMath::Abs(Stone.Min.Z) < 5);
	const FBox Cradle = UghAssets::Mesh(UghAssets::Copter, Sling)->GetBoundingBox();
	TestTrue(TEXT("the sling holds it"),
		Cradle.Min.Z < Hanging.Z + Stone.GetSize().Z / 2 && Cradle.Max.Z <= Wanted.Min.Z + 20);
	// the rope from the hook to the knot: a metre down, its knot on the sling's
	const FBox RopeBox = UghAssets::Mesh(UghAssets::Copter, SlingRope)->GetBoundingBox();
	TestTrue(FString::Printf(TEXT("the sling's rope %s"), *RopeBox.ToString()),
		FMath::IsNearlyEqual(RopeBox.Min.Z, -100, 2.0) && FMath::IsNearlyEqual(RopeBox.Max.Z, 0, 2.0) &&
		FMath::Abs(Cradle.Max.Z - SlingKnot.Z) < 6);
	TestTrue(TEXT("the stone's size in UghCopterModel"),
		FMath::IsNearlyEqual(Stone.GetSize().X / 2, StoneHalfWidth, 3.0) &&
		FMath::IsNearlyEqual(Stone.GetSize().Z, StoneHeight, 6.0) &&
		FMath::IsNearlyEqual(Stone.GetSize().Y / 2, StoneHalfDepth, 3.0));
	// every look of a passenger in a cabin is a caveman but the stone passenger's (4)
	for (int32 Look = 1; Look <= 4; ++Look)
	{
		TestEqual(FString::Printf(TEXT("look %d is a person"), Look), FUghCaveman::IsPassenger(Look), Look < 4);
	}
	return true;
}

#endif
