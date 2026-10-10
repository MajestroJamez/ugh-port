#include "UghLeaves.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MeshDescription.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "StaticMeshAttributes.h"
#include "UghElectricDreams.h"
#include "UghMeshes.h"
#include "UghShapes.h"

namespace
{
	/** The meshes' material slot. */
	const FName LeafSlot(TEXT("Leaves"));
	/** Without the sample's taro: leaves of green clay. */
	const FLinearColor ClayLeaf(0.06f, 0.2f, 0.03f);
	/** The parameter of the taro's material (its Megascans master) its colour is multiplied by. */
	const FName AlbedoTint(TEXT("Albedo Tint"));
	/** ... and how much light it lets through. */
	const FName ShineThrough[] = { FName(TEXT("Translucency Min")), FName(TEXT("Translucency Max")) };
	/** The sample's foliage sways in the wind (its material's parameters): not on a body. */
	const TCHAR* const Winds[] = { TEXT("Wind Gust Strength"), TEXT("Wind Noise Strength"),
		TEXT("Wind Gust Noise Strength"), TEXT("Animation Gradient") };

	/**
	 * A leaf round the body: where (degrees round it, 0 the front, 90 the left), its bone, its picture, its layer,
	 * how long it is (of its ring's leaves).
	 */
	struct FWornLeaf
	{
		double Angle;
		const TCHAR* Bone;
		int32 Picture;
		int32 Layer;   // the higher, the farther out (over the leaves it overlaps)
		double Length = 1;
	};
	/**
	 * The loincloth: a short leaf in front of the crotch and three at the back (on the pelvis), one on each side and
	 * two longer in front over them (on the thighs).
	 */
	const FWornLeaf Loincloth[] = { { 150, TEXT("pelvis"), 1, 0 }, { -150, TEXT("pelvis"), 2, 0 },
		{ 180, TEXT("pelvis"), 0, 1 }, { 0, TEXT("pelvis"), 0, 1, 0.75 }, { 80, TEXT("thigh_l"), 0, 1 },
		{ -80, TEXT("thigh_r"), 0, 1 }, { 22, TEXT("thigh_l"), 1, 2 }, { -22, TEXT("thigh_r"), 2, 3 } };
	/** The band round the chest: two leaves over the breasts, one on each side, one at the back. */
	const FWornLeaf Band[] = { { 180, TEXT("spine_05"), 0, 0 }, { 90, TEXT("spine_05"), 0, 1 },
		{ -90, TEXT("spine_05"), 0, 1 }, { 28, TEXT("spine_05"), 1, 2 }, { -28, TEXT("spine_05"), 2, 3 } };

	/**
	 * A ring of leaves round the body (component space of the rest pose, cm): the middle of the body at the height
	 * the leaves hang from, how long they are, and on each row of their sheets down how far the body reaches there or
	 * above to the side, the front and the back (X, Y, Z: half widths from the middle).
	 */
	struct FRing
	{
		FVector Top;
		double Length;
		TArray<FVector> Reaches;
	};

	/** The leaves stand off the body by Gap and a layer more for each layer; they flare out by Flare (radians). */
	constexpr double Gap = 1.2, LayerGap = 0.5, Flare = 0.2;
	/** A leaf's sheet: this many quads across and down. */
	constexpr int32 Across = 4, Down = 6;

	/**
	 * How far the body reaches at height `Z` (component space) by the shapes of the physics asset's bodies of `Bones`
	 * (capsules swept along their axes, spheres): the box of its cross-section (X, Y); empty when none reaches there.
	 */
	FBox2D CrossSection(const USkeletalMesh* Body, const TArray<FTransform>& Pose, TConstArrayView<const TCHAR*> Bones,
		double Z)
	{
		FBox2D Box(ForceInit);
		const UPhysicsAsset* Physics = Body->GetPhysicsAsset();
		if (!Physics)
		{
			return Box;
		}
		auto Sweep = [&](const FTransform& Space, const FVector& From, const FVector& To, double R0, double R1) {
			for (int32 Step = 0; Step <= 8; ++Step)
			{
				const FVector At = Space.TransformPosition(FMath::Lerp(From, To, Step / 8.0));
				const double Radius = FMath::Lerp(R0, R1, Step / 8.0), Off = At.Z - Z;
				if (FMath::Abs(Off) < Radius)
				{
					const double Round = FMath::Sqrt(Radius * Radius - Off * Off);
					Box += FVector2D(At.X - Round, At.Y - Round);
					Box += FVector2D(At.X + Round, At.Y + Round);
				}
			}
		};
		for (const USkeletalBodySetup* Setup : Physics->SkeletalBodySetups)
		{
			const int32 Bone = Body->GetRefSkeleton().FindBoneIndex(Setup->BoneName);
			auto Wanted = [&](const TCHAR* Name) { return Setup->BoneName == Name; };
			if (Bone == INDEX_NONE || !Bones.ContainsByPredicate(Wanted))
			{
				continue;
			}
			for (const FKSphylElem& Capsule : Setup->AggGeom.SphylElems)
			{
				const FVector Half = Capsule.Rotation.RotateVector(FVector(0, 0, Capsule.Length / 2));
				Sweep(Pose[Bone], Capsule.Center - Half, Capsule.Center + Half, Capsule.Radius, Capsule.Radius);
			}
			for (const FKTaperedCapsuleElem& Capsule : Setup->AggGeom.TaperedCapsuleElems)
			{
				const FVector Half = Capsule.Rotation.RotateVector(FVector(0, 0, Capsule.Length / 2));
				Sweep(Pose[Bone], Capsule.Center - Half, Capsule.Center + Half, Capsule.Radius1, Capsule.Radius0);
			}
			for (const FKSphereElem& Sphere : Setup->AggGeom.SphereElems)
			{
				Sweep(Pose[Bone], Sphere.Center, Sphere.Center, Sphere.Radius, Sphere.Radius);
			}
		}
		return Box;
	}

	/**
	 * The ring of leaves `Length` long hanging from `Top` (its height and the middle front to back) round the bodies of
	 * `Bones`, reaching `Front` more to the front; without them round a body of `Height` cm as it usually is there.
	 */
	FRing RingAt(const USkeletalMesh* Body, const TArray<FTransform>& Pose, TConstArrayView<const TCHAR*> Bones,
		const FVector& Top, double Length, double Front, double Height)
	{
		FRing Ring{ Top, Length, {} };
		FVector Reach = FVector::ZeroVector;
		for (int32 Row = 0; Row <= Down; ++Row)
		{
			const FBox2D Box = CrossSection(Body, Pose, Bones, Top.Z - Length * Row / Down);
			if (Box.bIsValid)
			{
				Reach = Reach.ComponentMax(FVector(FMath::Max(-Box.Min.X, Box.Max.X), Box.Max.Y - Top.Y,
					Top.Y - Box.Min.Y));
			}
			Ring.Reaches.Add(Reach + FVector(0, Front, 0));
		}
		if (Reach.GetMin() <= 0)
		{
			UE_LOG(LogTemp, Display, TEXT("UGH no physics bodies of %s at %.0f cm: leaves round an average body"),
				*Body->GetName(), Top.Z);
			Ring.Reaches.Init(FVector(0.09 * Height, 0.06 * Height + Front, 0.07 * Height), Down + 1);
		}
		return Ring;
	}

	/** How far the ring reaches from its middle on `Row` at `Angle` (radians, 0 the front): an ellipse each way. */
	double Reach(const FRing& Ring, double Row, double Angle)
	{
		const FVector At = FMath::Lerp(Ring.Reaches[FMath::FloorToInt(Row)], Ring.Reaches[FMath::CeilToInt(Row)],
			FMath::Frac(Row));
		const double Deep = FMath::Cos(Angle) > 0 ? At.Y : At.Z;
		return 1 / FMath::Sqrt(FMath::Square(FMath::Sin(Angle) / At.X) + FMath::Square(FMath::Cos(Angle) / Deep));
	}

	/** The sheet of `Leaf` on `Ring` into `Description` (in the space of its bone, `Bone`). */
	void AddLeaf(FMeshDescription& Description, const FRing& Ring, const FWornLeaf& Leaf, const FTransform& Bone)
	{
		FStaticMeshAttributes Attributes(Description);
		const FBox2f Picture = UghElectricDreams::LeafPictures[Leaf.Picture];
		const double Long = Ring.Length * Leaf.Length, Width = Long * Picture.GetSize().X / Picture.GetSize().Y;
		const double Middle = FMath::DegreesToRadians(Leaf.Angle);
		const double Off = Gap + LayerGap * Leaf.Layer;
		auto Point = [&](double U, double V) {   // U -0.5 .. 0.5 across, V 0 .. 1 down
			const double Row = FMath::Clamp(V * Leaf.Length, 0.0, 1.0) * Down;
			const double Angle = Middle + U * Width / (Reach(Ring, Row, Middle) + Off);
			const double Out = Reach(Ring, Row, Angle) + Off + V * Long * FMath::Sin(Flare);
			return Bone.InverseTransformPosition(Ring.Top + FVector(FMath::Sin(Angle) * Out, FMath::Cos(Angle) * Out,
				-V * Long * FMath::Cos(Flare)));
		};
		if (Description.PolygonGroups().Num() == 0)
		{
			Attributes.GetPolygonGroupMaterialSlotNames()[Description.CreatePolygonGroup()] = LeafSlot;
		}
		const FPolygonGroupID Group(0);
		TArray<FVertexInstanceID> Grid;
		for (int32 Row = 0; Row <= Down; ++Row)
		{
			for (int32 Column = 0; Column <= Across; ++Column)
			{
				const double U = double(Column) / Across - 0.5, V = double(Row) / Down;
				const FVertexID Vertex = Description.CreateVertex();
				Attributes.GetVertexPositions()[Vertex] = FVector3f(Point(U, V));
				const FVertexInstanceID Instance = Description.CreateVertexInstance(Vertex);
				const FVector Along = Point(U + 0.01, V) - Point(U - 0.01, V);
				const FVector Downward = Point(U, V + 0.01) - Point(U, V - 0.01);
				Attributes.GetVertexInstanceNormals()[Instance] = FVector3f(FVector::CrossProduct(Along, Downward)
					.GetSafeNormal());   // outwards
				Attributes.GetVertexInstanceTangents()[Instance] = FVector3f(Along.GetSafeNormal());
				// the cut at the belt (the picture's bottom), the tip down
				Attributes.GetVertexInstanceUVs().Set(Instance, 0, FVector2f(FMath::Lerp(Picture.Min.X,
					Picture.Max.X, float(U + 0.5)), FMath::Lerp(Picture.Max.Y, Picture.Min.Y, float(V))));
				Grid.Add(Instance);
			}
		}
		for (int32 Row = 0; Row < Down; ++Row)
		{
			for (int32 Column = 0; Column < Across; ++Column)
			{
				const int32 Corner = Row * (Across + 1) + Column;
				const FVertexInstanceID A = Grid[Corner], B = Grid[Corner + 1], C = Grid[Corner + Across + 2],
					D = Grid[Corner + Across + 1];
				Description.CreateTriangle(Group, { A, C, B });   // facing outwards
				Description.CreateTriangle(Group, { A, D, C });
			}
		}
	}
}

void FUghLeaves::Make(const USkeletalMesh* Body, double Height, bool bTop, const FLinearColor& Tint)
{
	Parts.Reset();
	const FReferenceSkeleton& Skeleton = Body->GetRefSkeleton();
	const TArray<FTransform> Pose = FUghLeaves::RestPose(Skeleton);
	const TCHAR* const Hips[] = { TEXT("pelvis"), TEXT("spine_01"), TEXT("thigh_l"), TEXT("thigh_r") };
	const TCHAR* const Chest[] = { TEXT("spine_04"), TEXT("spine_05") };
	for (const TCHAR* Bone : { TEXT("pelvis"), TEXT("spine_05"), TEXT("thigh_l"), TEXT("thigh_r") })
	{
		if (Skeleton.FindBoneIndex(Bone) == INDEX_NONE)
		{
			UE_LOG(LogTemp, Display, TEXT("UGH %s has no bone %s: no leaves"), *Body->GetName(), Bone);
			return;
		}
	}
	auto At = [&](const TCHAR* Bone) { return Pose[Skeleton.FindBoneIndex(Bone)].GetLocation(); };
	// the belt over the hip bones, the leaves to the middle of the thighs; the band under the arms over the breasts
	const FVector Pelvis = At(TEXT("pelvis")), Breast = At(TEXT("spine_05"));
	const FRing Rings[] = {
		RingAt(Body, Pose, Hips, Pelvis + FVector(0, 0, 0.085 * Height), 0.24 * Height, 0, Height),
		RingAt(Body, Pose, Chest, FVector(0, Breast.Y, Breast.Z + 0.025 * Height), 0.15 * Height, 0.03 * Height,
			Height) };
	TMap<FName, FMeshDescription> Descriptions;
	for (int32 Ring = 0; Ring < (bTop ? 2 : 1); ++Ring)
	{
		for (const FWornLeaf& Leaf : Ring == 0 ? TConstArrayView<FWornLeaf>(Loincloth) : TConstArrayView<FWornLeaf>(Band))
		{
			FMeshDescription* Description = Descriptions.Find(Leaf.Bone);
			if (!Description)
			{
				Description = &Descriptions.Add(Leaf.Bone);
				FStaticMeshAttributes(*Description).Register();
			}
			AddLeaf(*Description, Rings[Ring], Leaf, Pose[Skeleton.FindBoneIndex(Leaf.Bone)]);
		}
	}
	for (TPair<FName, FMeshDescription>& Each : Descriptions)
	{
		Parts.Add({ Each.Key, UghMeshes::FromDescription(GetTransientPackage(), Each.Value, LeafSlot) });
	}
	UMaterialInterface* Taro = UghElectricDreams::Material(UghElectricDreams::LeafMaterial);
	UMaterialInstanceDynamic* Leaf = Taro ? UMaterialInstanceDynamic::Create(Taro, GetTransientPackage())
		: UghShapes::Clay(GetTransientPackage(), ClayLeaf * Tint);
	if (Taro && !Tint.Equals(FLinearColor::White))
	{
		// tinted, and no light through them: the taro's own green would shine through (the figures' light from behind)
		Leaf->SetVectorParameterValue(AlbedoTint, Tint);
		for (const FName& Through : ShineThrough)
		{
			Leaf->SetScalarParameterValue(Through, 0);
		}
	}
	for (const TCHAR* Wind : Taro ? TConstArrayView<const TCHAR*>(Winds) : TConstArrayView<const TCHAR*>())
	{
		Leaf->SetScalarParameterValue(Wind, 0);
	}
	Material = Leaf;
}

TArray<FTransform> FUghLeaves::RestPose(const FReferenceSkeleton& Skeleton)
{
	TArray<FTransform> Pose;
	for (int32 Bone = 0; Bone < Skeleton.GetNum(); ++Bone)
	{
		const int32 Parent = Skeleton.GetParentIndex(Bone);
		Pose.Add(Skeleton.GetRefBonePose()[Bone] * (Parent == INDEX_NONE ? FTransform::Identity : Pose[Parent]));
	}
	return Pose;
}

FBox FUghLeaves::Bounds(const USkeletalMesh* Body, TConstArrayView<const TCHAR*> Bones) const
{
	const TArray<FTransform> Pose = RestPose(Body->GetRefSkeleton());
	FBox Box(ForceInit);
	for (const FUghLeafPart& Part : Parts)
	{
		if (Bones.ContainsByPredicate([&](const TCHAR* Bone) { return Part.Bone == Bone; }))
		{
			Box += Part.Mesh->GetBoundingBox().TransformBy(Pose[Body->GetRefSkeleton().FindBoneIndex(Part.Bone)]);
		}
	}
	return Box;
}

void FUghLeaves::Add(AActor* Owner, USkeletalMeshComponent* Body) const
{
	for (const FUghLeafPart& Part : Parts)
	{
		UStaticMeshComponent* Leaves = NewObject<UStaticMeshComponent>(Owner, NAME_None, RF_Transient);
		Leaves->SetStaticMesh(Part.Mesh);
		Leaves->SetMaterial(0, Material);
		Leaves->SetMobility(EComponentMobility::Movable);
		Leaves->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Leaves->SetVisibleInRayTracing(false);   // as the body (FUghMetaHuman)
		Leaves->SetVisibility(false);
		Leaves->SetupAttachment(Body, Part.Bone);
		Leaves->RegisterComponent();
		Owner->AddInstanceComponent(Leaves);
	}
}
