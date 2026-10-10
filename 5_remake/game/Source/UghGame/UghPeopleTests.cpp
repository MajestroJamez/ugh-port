// The people as an automation test of the editor: the actions of the MetaHumans (Ugh.Figures.People).
#if WITH_DEV_AUTOMATION_TESTS

#include "Animation/AnimationPoseData.h"
#include "Animation/AnimSequence.h"
#include "Animation/AttributesRuntime.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "Engine/SkeletalMesh.h"
#include "Misc/AutomationTest.h"
#include "UghCaveman.h"
#include "UghCopterModel.h"
#include "UghFigurePlace.h"
#include "UghMetaHumans.h"

namespace
{
	/**
	 * Where the joint `Bone` of `Mesh` is (its component space) in `Action` at `Fraction` of its loop, as the game
	 * shows it (the engine's pose of the animation on the mesh, retargeted as the game does).
	 */
	FVector JointAt(const USkeletalMesh* Mesh, const UAnimSequence* Action, FName Bone, double Fraction)
	{
		FMemMark Mark(FMemStack::Get());
		const FReferenceSkeleton& Skeleton = Mesh->GetRefSkeleton();
		TArray<FBoneIndexType> Required;
		for (int32 Index = 0; Index < Skeleton.GetNum(); ++Index)
		{
			Required.Add(FBoneIndexType(Index));
		}
		FBoneContainer Bones(Required, UE::Anim::FCurveFilterSettings(), *const_cast<USkeletalMesh*>(Mesh));
		FCompactPose Pose;
		Pose.SetBoneContainer(&Bones);
		FBlendedCurve Curve;
		Curve.InitFrom(Bones);
		UE::Anim::FStackAttributeContainer Attributes;
		FAnimationPoseData Data(Pose, Curve, Attributes);
		Action->GetAnimationPose(Data, FAnimExtractContext(Fraction * Action->GetPlayLength()));
		FCSPose<FCompactPose> Component;
		Component.InitPose(Pose);
		const FCompactPoseBoneIndex Joint = Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(Skeleton.FindBoneIndex(Bone)));
		return Component.GetComponentSpaceTransform(Joint).GetLocation();
	}

	/** The joints whose place the test checks, by side. */
	const FName Feet[] = { TEXT("foot_l"), TEXT("foot_r") };
	const FName Hands[] = { TEXT("hand_l"), TEXT("hand_r") };
	const FName Head(TEXT("head")), Pelvis(TEXT("pelvis"));

	/**
	 * The leaves of `Person` (in his own size) cover his hips in the rest pose: from above the pelvis to below the
	 * hip joints, across them, in front of and behind them; with `bTop` his chest too (round spine_05, below it).
	 */
	void CheckLeaves(FAutomationTestBase& Test, const FUghMetaHuman& Person, bool bTop)
	{
		const USkeletalMesh* Mesh = Person.GetBody().GetMesh();
		const FReferenceSkeleton& Skeleton = Mesh->GetRefSkeleton();
		const TArray<FTransform> Rest = FUghLeaves::RestPose(Skeleton);
		auto Joint = [&](const TCHAR* Bone) { return Rest[Skeleton.FindBoneIndex(Bone)].GetLocation(); };
		const FVector Hip = Joint(TEXT("thigh_l")), Middle = Joint(TEXT("pelvis")), Breast = Joint(TEXT("spine_05"));
		const double Span = Person.GetHeight() * 0.1;
		const FBox Hips = Person.GetLeaves().Bounds(Mesh, { TEXT("pelvis"), TEXT("thigh_l"), TEXT("thigh_r") });
		Test.TestTrue(FString::Printf(TEXT("%s's leaves %s cover the hips"), *Mesh->GetName(), *Hips.ToString()),
			Hips.Max.Z > Middle.Z && Hips.Min.Z < Hip.Z - Span && Hips.Min.X < -FMath::Abs(Hip.X) &&
			Hips.Max.X > FMath::Abs(Hip.X) && Hips.Min.Y < Middle.Y - Span / 2 && Hips.Max.Y > Middle.Y + Span / 2);
		const FBox Chest = Person.GetLeaves().Bounds(Mesh, { TEXT("spine_05") });
		Test.TestEqual(FString::Printf(TEXT("%s's leaves %s on the chest"), *Mesh->GetName(), *Chest.ToString()),
			Chest.IsValid && Chest.Max.Z > Breast.Z - Span && Chest.Min.Z < Breast.Z - Span &&
			Chest.Max.Y > Breast.Y + Span / 2, bTop);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghPeopleTest, "Ugh.Figures.People",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * Each MetaHuman's actions put him where the game wants him, in the people's height (the game's units): standing on
 * the ground and in the slab of the play, sitting on the seat, pedalling the copter's crank holding its handles,
 * hanging from where his hands hold, in the water with his head at the surface, flung windmilling his arms, ducking
 * on the ground shielding his head, walking off with his arms up. His leaves cover his hips from the belt to the thighs
 * (and the woman's chest).
 */
bool FUghPeopleTest::RunTest(const FString& Parameters)
{
	using enum EUghCaveAction;
	constexpr double Slab = 35, Near = 4;   // the slab of the play (20) and a swinging hand; near enough
	constexpr int32 Samples = 8;
	for (int32 Look = 0; Look < UE_ARRAY_COUNT(UghMetaHumans::Names); ++Look)
	{
		const TCHAR* Name = UghMetaHumans::Names[Look];
		FUghMetaHuman Person;
		if (!Person.Load(Look, FUghCaveman::ActionNames()))
		{
			AddInfo(FString::Printf(TEXT("no MetaHuman %s (metahumans.ps1): not checked"), Name));
			continue;
		}
		const USkeletalMesh* Mesh = Person.GetBody().GetMesh();
		const double Scale = UghFigurePlace::PersonHeight / Person.GetHeight();
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
			for (const EUghCaveAction Action : { Idle, Walk, Wave, Duck, Cheer })
			{
				const double Ground = FMath::Min(At(Action, Feet[0], Fraction).Z, At(Action, Feet[1], Fraction).Z);
				Check(Ground > 0 && Ground < 12, TEXT("stands on the ground"), Action, FVector(0, 0, Ground));
				for (const FName Bone : { Head, Hands[0], Hands[1], Feet[0], Feet[1] })
				{
					// one walks to a side, looking 20 degrees towards the camera (UghFigurePlace)
					const FVector Joint = (Action == Walk || Action == Cheer ? FRotator(0, 70, 0) : FRotator::ZeroRotator)
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
				const FVector Foot = At(Pedal, Feet[Side], Fraction) - UghCopterModel::PedalAxle;
				Check(FVector2D(Foot.Y, Foot.Z).Size() < UghCopterModel::PedalRadius + 2 * Near &&
					FMath::IsNearlyEqual(Foot.X, Sign * UghCopterModel::PedalSpread, Near), TEXT("pedals"), Pedal, Foot);
				const FVector Holding = At(Hang, Hands[Side], Fraction);
				Check(Holding.Size() < 2 * Near, TEXT("holds the rope"), Hang, Holding);
			}
			// ducking: a fifth lower than standing, the hands over the head; cheering: the hands high over it
			const FVector Ducked = At(Duck, Head, Fraction);
			Check(Ducked.Z < At(Idle, Head, Fraction).Z - 0.18 * UghFigurePlace::PersonHeight, TEXT("ducks"), Duck,
				Ducked);
			for (int32 Side = 0; Side < 2; ++Side)
			{
				const FVector Shield = At(Duck, Hands[Side], Fraction) - Ducked;
				Check(Shield.Z > 0 && FVector2D(Shield.X, Shield.Y).Size() < 0.15 * UghFigurePlace::PersonHeight,
					TEXT("shields his head"), Duck, Shield);
				const FVector Up = At(Cheer, Hands[Side], Fraction) - At(Cheer, Head, Fraction);
				Check(Up.Z > 0.1 * UghFigurePlace::PersonHeight, TEXT("has his arms up"), Cheer, Up);
			}
			for (const EUghCaveAction Action : { Tread, Fall, Flail })
			{
				const FVector Joint = At(Action, Head, Fraction);
				Check(Joint.Z > -2 && Joint.Z < 8 && FMath::Abs(Joint.Y) < Slab, TEXT("has his head at the surface"),
					Action, Joint);
			}
		}
		// flung through the air he windmills his arms: far out beside him, over his head and down again
		double Out = 0, Highest = -UE_BIG_NUMBER, Lowest = UE_BIG_NUMBER;
		for (int32 Sample = 0; Sample < Samples; ++Sample)
		{
			const FVector Hand = At(Flail, Hands[0], double(Sample) / Samples);
			Out = FMath::Max(Out, FMath::Abs(Hand.X));
			Highest = FMath::Max(Highest, Hand.Z);
			Lowest = FMath::Min(Lowest, Hand.Z);
		}
		Check(Out > 45 && Highest > 20 && Highest - Lowest > 40, TEXT("windmills his arms"), Flail,
			FVector(Out, Lowest, Highest));
		// the left pedal is on top at the start of the loop, the right one half a turn later
		const double Axle = UghCopterModel::PedalAxle.Z;
		Check(At(Pedal, Feet[0], 0).Z > Axle && At(Pedal, Feet[1], 0.5).Z > Axle, TEXT("turns the crank"), Pedal,
			At(Pedal, Feet[0], 0));
		CheckLeaves(*this, Person, UghMetaHumans::Tops[Look]);
	}
	return true;
}

#endif
