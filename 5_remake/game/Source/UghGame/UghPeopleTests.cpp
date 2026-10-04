// The people as an automation test of the editor: the actions of the MetaHumans (Ugh.Figures.People).
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITORONLY_DATA

#include "Animation/AnimData/IAnimationDataModel.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "Misc/AutomationTest.h"
#include "UghCaveman.h"
#include "UghCopterModel.h"
#include "UghFigurePlace.h"
#include "UghMetaHumans.h"

namespace
{
	/** Where the joint `Bone` of `Mesh` is (its component space) in `Action` at `Fraction` of its loop. */
	FVector JointAt(const USkeletalMesh* Mesh, const UAnimSequence* Action, FName Bone, double Fraction)
	{
		const FReferenceSkeleton& Skeleton = Mesh->GetRefSkeleton();
		const IAnimationDataModel* Model = Action->GetDataModel();
		const FFrameTime Time = Model->GetFrameRate().AsFrameTime(Fraction * Action->GetPlayLength());
		FTransform Joint = FTransform::Identity;
		for (int32 Index = Skeleton.FindBoneIndex(Bone); Index != INDEX_NONE; Index = Skeleton.GetParentIndex(Index))
		{
			const FName Name = Skeleton.GetBoneName(Index);
			Joint = Joint * (Model->IsValidBoneTrackName(Name)
				? Model->EvaluateBoneTrackTransform(Name, Time, EAnimInterpolationType::Linear)
				: Skeleton.GetRefBonePose()[Index]);
		}
		return Joint.GetLocation();
	}

	/** The joints whose place the test checks, by side. */
	const FName Feet[] = { TEXT("foot_l"), TEXT("foot_r") };
	const FName Hands[] = { TEXT("hand_l"), TEXT("hand_r") };
	const FName Head(TEXT("head")), Pelvis(TEXT("pelvis"));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghPeopleTest, "Ugh.Figures.People",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * Each MetaHuman's actions put him where the game wants him, in the caveman's height (the game's units): standing on
 * the ground and in the slab of the play, sitting on the seat, pedalling the copter's crank holding its handles,
 * hanging from where his hands hold, in the water with his head at the surface.
 */
bool FUghPeopleTest::RunTest(const FString& Parameters)
{
	using enum EUghCaveAction;
	constexpr double Slab = 30, Near = 4;   // the slab of the play (20) and a swinging hand; near enough
	constexpr int32 Samples = 8;
	for (const TCHAR* Name : UghMetaHumans::Names)
	{
		FUghMetaHuman Person;
		if (!Person.Load(Name, FUghCaveman::ActionNames()))
		{
			AddInfo(FString::Printf(TEXT("no MetaHuman %s (metahumans.ps1): not checked"), Name));
			continue;
		}
		const USkeletalMesh* Mesh = Person.GetBody().GetMesh();
		const double Scale = UghFigurePlace::CavemanHeight / Person.GetHeight();
		auto At = [&](EUghCaveAction Action, FName Bone, double Fraction) {
			return JointAt(Mesh, Person.GetBody().GetAction(int32(Action)), Bone, Fraction) * Scale;
		};
		auto Check = [&](bool bOk, const TCHAR* What, EUghCaveAction Action, const FVector& Where) {
			TestTrue(FString::Printf(TEXT("%s %s in %s (%s)"), Name, What, FUghCaveman::ActionNames()[int32(Action)],
				*Where.ToCompactString()), bOk);
		};
		for (int32 Sample = 0; Sample < Samples; ++Sample)
		{
			const double Fraction = double(Sample) / Samples;
			for (const EUghCaveAction Action : { Idle, Walk, Wave })
			{
				const double Ground = FMath::Min(At(Action, Feet[0], Fraction).Z, At(Action, Feet[1], Fraction).Z);
				Check(Ground > 0 && Ground < 12, TEXT("stands on the ground"), Action, FVector(0, 0, Ground));
				for (const FName Bone : { Head, Hands[0], Hands[1], Feet[0], Feet[1] })
				{
					// one walks to a side, looking 20 degrees towards the camera (UghFigurePlace)
					const FVector Joint = (Action == Walk ? FRotator(0, 70, 0) : FRotator::ZeroRotator)
						.RotateVector(At(Action, Bone, Fraction));
					Check(FMath::Abs(Joint.Y) < Slab, TEXT("stays in the slab of the play"), Action, Joint);
				}
			}
			const FVector Seat = At(Sit, Pelvis, Fraction);
			Check(Seat.Z > 0 && Seat.Z < 20 && At(Sit, Feet[0], Fraction).Z < 0, TEXT("sits on the seat"), Sit, Seat);
			for (int32 Side = 0; Side < 2; ++Side)
			{
				const double Sign = Side == 0 ? 1 : -1;
				const FVector Grip(Sign * UghCopterModel::Grip.X, UghCopterModel::Grip.Y, UghCopterModel::Grip.Z);
				const FVector Hand = At(Pedal, Hands[Side], Fraction);
				Check(FVector::Dist(Hand, Grip) < Near, TEXT("holds the handle"), Pedal, Hand);
				const FVector Axle = UghCopterModel::CrankAxle - UghCopterModel::PilotSeat;
				const FVector Foot = At(Pedal, Feet[Side], Fraction) - Axle;
				Check(FVector2D(Foot.Y, Foot.Z).Size() < UghCopterModel::PedalRadius + 2 * Near &&
					FMath::IsNearlyEqual(Foot.X, Sign * UghCopterModel::PedalSpread, Near), TEXT("pedals"), Pedal, Foot);
				const FVector Holding = At(Hang, Hands[Side], Fraction);
				Check(Holding.Size() < 2 * Near, TEXT("holds the rope"), Hang, Holding);
			}
			for (const EUghCaveAction Action : { Tread, Fall })
			{
				const FVector Joint = At(Action, Head, Fraction);
				Check(Joint.Z > -2 && Joint.Z < 8 && FMath::Abs(Joint.Y) < Slab, TEXT("has his head at the surface"),
					Action, Joint);
			}
		}
		// the left pedal is on top at the start of the loop, the right one half a turn later
		const double Axle = (UghCopterModel::CrankAxle - UghCopterModel::PilotSeat).Z;
		Check(At(Pedal, Feet[0], 0).Z > Axle && At(Pedal, Feet[1], 0.5).Z > Axle, TEXT("turns the crank"), Pedal,
			At(Pedal, Feet[0], 0));
	}
	return true;
}

#endif
