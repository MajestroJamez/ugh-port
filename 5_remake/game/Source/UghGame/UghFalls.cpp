#include "UghFalls.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UghAssets.h"
#include "UghDecorations.h"
#include "UghMaterials.h"
#include "UghMeshes.h"
#include "UghShapes.h"

namespace
{
	using UghMeshes::FVertex;
	constexpr double Units = UghShapes::UnitsPerPixel;

	/** How fast the water flows along the stream (cm/s), how fast things fall (cm/s^2). */
	constexpr double StreamSpeed = 60, Gravity = 980;
	/** Pixels: the stream reaches this far into the walls of its channel (no gap at its banks). */
	constexpr double Tuck = 0.2;
	/** Where its bed is this steep (pixels down a pixel deep) or more, the stream is a cascade. */
	constexpr double Steep = 0.6;
	/** The waterfall: it starts at the stream's surface, widens by this part, bulges out (units). */
	constexpr double FallTop = UghStreams::WaterDown, FallWiden = 0.1, FallBulge = 1.5;
	/** It is a sheet this often across; how far it falls before it froths (pixels). */
	constexpr int32 FallColumns = 4;
	constexpr double Froth = 4;
	/** The mist: puffs a waterfall, rising this far inside its sides (pixels), this deep in front of it (units). */
	constexpr int32 MistPuffs = 80;
	constexpr double MistInside = 1, MistSpread = 40;
	/** The bridge's logs (pixels): about this thick, their ends uneven; two beams under them (units). */
	constexpr double LogThickness = 1, LogJitter = 0.12, EndJitter = 0.4, BeamThickness = 8, BeamInset = 0.8;
	/** Degrees a log is turned at most from lying straight across. */
	constexpr double LogTurn = 2;
	/**
	 * Its handrail behind the figures walking over it (pixels): posts this thick and tall at its ends, a pole on them,
	 * this far in front of the deck's back (units).
	 */
	constexpr double PostThickness = 0.6, PostHeight = 3.6, PoleThickness = 0.45, RailIn = 6;
	static_assert(PostHeight <= UghDecorations::GroundCover, "the handrail is no taller than ground cover");
	/** Units: the stream flows out of this far inside its hole. */
	constexpr double InHole = 40;
	const FLinearColor WoodColor(0.3f, 0.22f, 0.15f);

	/** A row of `Count` vertices from `From` to `To` (world), facing `Normal`, across at `Time`; Flow the second UV. */
	void AddRow(TArray<FVertex>& Vertices, const FVector& From, const FVector& To, int32 Count, const FVector& Normal,
		double Time, const FVector2f& Flow, double Bulge = 0)
	{
		for (int32 I = 0; I < Count; ++I)
		{
			const double U = I / double(Count - 1);
			const FVector At = FMath::Lerp(From, To, U) + Normal * Bulge * FMath::Sin(UE_PI * U);
			Vertices.Add({ At, FVector3f(Normal), FVector3f::UnitX(), FVector2f(U, Time), Flow });
		}
	}

	/** Quads between each row of `Count` vertices from `First` and the next. */
	void Stitch(TArray<int32>& Triangles, int32 First, int32 Rows, int32 Count)
	{
		for (int32 Row = 0; Row + 1 < Rows; ++Row)
		{
			for (int32 I = 0; I + 1 < Count; ++I)
			{
				const int32 A = First + Row * Count + I, B = A + 1, C = A + Count, D = C + 1;
				Triangles.Append({ A, C, B, B, C, D });
			}
		}
	}

	/** The way out of a sheet going from `A` down to `B` (and across x) that faces the camera (world y is -depth). */
	FVector Facing(const FVector& A, const FVector& B)
	{
		const FVector Normal = FVector::CrossProduct(B - A, FVector::XAxisVector).GetSafeNormal();
		return Normal.Y < 0 ? -Normal : Normal;
	}

	/**
	 * The water of `Stream`: the stream from inside its hole along its bed and its channel to the face's edge, the
	 * waterfall from there down to under the sea's surface.
	 */
	void AddWater(const FUghStream& Stream, TArray<FVertex>& Vertices, TArray<int32>& Triangles)
	{
		const double X = Stream.X(), Y = Stream.Y;
		// the stream in its channel out of its hole towards the camera (a cascade where the bed curves up into the
		// wall), through the rock of the slab of the play under the bridge's deck, out to the face's edge
		const FVector Sunk(0, UghStreams::WaterDown, UghStreams::WaterDown);
		TArray<FVector> Way = { Stream.Bed.Last() + Sunk + FVector(InHole, 0, 0) };
		for (int32 Step = Stream.Bed.Num() - 1; Step >= 0; --Step)
		{
			Way.Add(Stream.Bed[Step] + Sunk);
		}
		Way.Append({ FVector(0, Y + UghStreams::WaterDown, Y + UghStreams::WaterDown),
			FVector(Stream.BridgeFront, Y + UghStreams::WaterDown, Y + UghStreams::WaterDown) });
		double Time = 0;
		int32 First = Vertices.Num();
		for (int32 Step = 0; Step < Way.Num(); ++Step)
		{
			const FVector& At = Way[Step];
			const FVector& Nearer = Way[FMath::Min(Step + 1, Way.Num() - 1)];
			const double Run = FMath::Max(At.X - Nearer.X, 1.0) / Units;
			const double Drop = FMath::Max((Nearer.Y + Nearer.Z) - (At.Y + At.Z), 0.0) / 2;
			// (it faces up, towards the camera where it drops: world y is minus the depth)
			AddRow(Vertices, UghShapes::ToWorld(Stream.Left - Tuck, At.Y, At.X),
				UghShapes::ToWorld(Stream.Right + Tuck, At.Z, At.X), 3, FVector(0, Drop, Run).GetSafeNormal(), Time,
				FVector2f(FMath::Min(Drop / Run / Steep, 1.0), 0));
			Time += FMath::Sqrt(Run * Run + Drop * Drop) * Units / StreamSpeed;
		}
		Stitch(Triangles, First, Way.Num(), 3);
		// the waterfall: from the water's surface at the face's edge, faster and faster, widening a little
		const double Height = (Stream.Fall.Num() - 1) * UghStreams::FallStep;
		auto FallAt = [&](int32 Row)
		{
			return UghShapes::ToWorld(X, Y + Row * UghStreams::FallStep, Stream.Fall[Row]);
		};
		const int32 Top = FMath::CeilToInt32(FallTop / UghStreams::FallStep), Rows = Stream.Fall.Num() - Top + 1;
		First = Vertices.Num();
		for (int32 Row = Top - 1; Row < Stream.Fall.Num(); ++Row)
		{
			const double Down = FMath::Max(Row * UghStreams::FallStep, FallTop), Fallen = (Down - FallTop) * Units;
			const double Half = (UghStreams::Width / 2 + Tuck) * (1 + FallWiden * Down / Height);
			const int32 Last = Stream.Fall.Num() - 1;
			const FVector Normal = Facing(FallAt(FMath::Max(Row - 1, 0)), FallAt(FMath::Min(Row + 1, Last)));
			// (as long as it took to fall that far, leaving the edge at the stream's speed)
			const double Seconds = (FMath::Sqrt(FMath::Square(StreamSpeed) + 2 * Gravity * Fallen) - StreamSpeed) / Gravity;
			AddRow(Vertices, UghShapes::ToWorld(X - Half, Y + Down, Stream.Fall[Row]),
				UghShapes::ToWorld(X + Half, Y + Down, Stream.Fall[Row]), FallColumns, Normal, Time + Seconds,
				FVector2f(FMath::SmoothStep(FallTop, FallTop + Froth, Down), Down / Height), FallBulge);
		}
		Stitch(Triangles, First, Rows, FallColumns);
	}

	/** The puffs of the mist at the foot of the waterfall of `Stream`, up to its ledge (the bounds: the sea rises). */
	void AddMist(const FUghStream& Stream, FRandomStream& Random, TArray<UghMeshes::FQuad>& Quads)
	{
		for (int32 Puff = 0; Puff < MistPuffs; ++Puff)
		{
			const double X = Random.FRandRange(Stream.Left + MistInside, Stream.Right - MistInside);
			const double Depth = Stream.Fall.Last() - Random.FRandRange(0, MistSpread);
			const double Height = Random.FRandRange(Stream.Y, Stream.Foot);
			Quads.Add({ UghShapes::ToWorld(X, Height, Depth), FVector2D(1), FVector2D(Random.FRand(), Random.FRand()) });
		}
	}

	/** The numbers of `Stream` (the same every time). */
	FRandomStream RandomOf(const FUghStream& Stream)
	{
		return FRandomStream(Stream.Y * 1000 + FMath::FloorToInt32(Stream.Left));
	}
}

TArray<FTransform> UghFalls::Bridge(const FUghStream& Stream)
{
	FRandomStream Random = RandomOf(Stream);
	TArray<FTransform> Logs;
	const double Surface = UghShapes::ToWorld(0, Stream.Y, 0).Z - UnderSurface;
	const double Middle = (Stream.BridgeLeft + Stream.BridgeRight) / 2;
	const double Length = (Stream.BridgeRight - Stream.BridgeLeft) * Units;
	// (the engine's cylinder stands along z)
	const FQuat AlongX(FVector::YAxisVector, UE_HALF_PI), AlongDepth(FVector::XAxisVector, UE_HALF_PI);
	for (double Front = Stream.BridgeFront; Front < Stream.BridgeBack;)
	{
		const double Thick = LogThickness * Units * (1 + Random.FRandRange(-LogJitter, LogJitter));
		const double Depth = FMath::Min(Front + Thick / 2, Stream.BridgeBack - Thick / 2);
		const FVector At = UghShapes::ToWorld(Middle + Random.FRandRange(-EndJitter, EndJitter), 0, Depth);
		const FQuat Turn(FVector::ZAxisVector, FMath::DegreesToRadians(Random.FRandRange(-LogTurn, LogTurn)));
		Logs.Add(FTransform(Turn * AlongX, FVector(At.X, At.Y, Surface - Thick / 2), FVector(Thick, Thick,
			Length + Random.FRandRange(-EndJitter, EndJitter) * Units) / UghShapes::ShapeSize));
		Front += Thick;
	}
	const double Deck = Stream.BridgeBack - Stream.BridgeFront;
	for (const double Along : { Stream.BridgeLeft + BeamInset, Stream.BridgeRight - BeamInset })
	{
		const FVector At = UghShapes::ToWorld(Along, 0, (Stream.BridgeFront + Stream.BridgeBack) / 2);
		Logs.Add(FTransform(AlongDepth, FVector(At.X, At.Y, Surface - LogThickness * Units - BeamThickness / 3),
			FVector(BeamThickness, BeamThickness, Deck + BeamThickness) / UghShapes::ShapeSize));
	}
	// the handrail at the back: a post at each end, a pole along their tops
	const double Back = Stream.BridgeBack - RailIn, Post = PostThickness * Units, Tall = PostHeight * Units;
	for (const double Along : { Stream.BridgeLeft + PostThickness / 2, Stream.BridgeRight - PostThickness / 2 })
	{
		const FVector At = UghShapes::ToWorld(Along, 0, Back);
		Logs.Add(FTransform(FQuat::Identity, FVector(At.X, At.Y, Surface + Tall / 2), FVector(Post, Post, Tall) /
			UghShapes::ShapeSize));
	}
	const FVector Pole = UghShapes::ToWorld(Middle, 0, Back);
	const double Thin = PoleThickness * Units;
	Logs.Add(FTransform(AlongX, FVector(Pole.X, Pole.Y, Surface + Tall - Thin / 2), FVector(Thin, Thin, Length) /
		UghShapes::ShapeSize));
	return Logs;
}

AUghFalls::AUghFalls()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AUghFalls::BeginPlay()
{
	Super::BeginPlay();
	FlowMaterial = UghShapes::Material(this, UghMaterials::Flow);
	MistMaterial = UghShapes::Material(this, UghMaterials::Mist);
	Water = UghMeshes::NewPart<UStaticMeshComponent>(this, FlowMaterial);
	Mist = UghMeshes::NewPart<UStaticMeshComponent>(this, MistMaterial);
	UMaterialInterface* Wood = UghAssets::Material(UghAssets::BridgeWood);
	Logs = UghShapes::AddShapes(this, UghShapes::EShape::Cylinder, Wood ? Wood : UghShapes::Clay(this, WoodColor));
}

void AUghFalls::Show(const TArray<FUghStream>& InStreams)
{
	Streams = InStreams;
	TArray<FVertex> Vertices;
	TArray<int32> Triangles;
	TArray<UghMeshes::FQuad> Puffs;
	TArray<FTransform> Bridges;
	for (const FUghStream& Stream : Streams)
	{
		FRandomStream Random = RandomOf(Stream);
		AddWater(Stream, Vertices, Triangles);
		AddMist(Stream, Random, Puffs);
		Bridges.Append(UghFalls::Bridge(Stream));
	}
	Water->SetStaticMesh(Streams.IsEmpty() ? nullptr : UghMeshes::Build(this, Vertices, Triangles));
	Mist->SetStaticMesh(Streams.IsEmpty() ? nullptr : UghMeshes::Quads(this, Puffs));
	UghShapes::SetShapes(Logs, Bridges);
}

void AUghFalls::SetWater(double Surface)
{
	const float Level = UghShapes::ToWorld(0, Surface, 0).Z;
	FlowMaterial->SetScalarParameterValue(UghMaterials::WaterLevelParameter, Level);
	MistMaterial->SetScalarParameterValue(UghMaterials::WaterLevelParameter, Level);
	Mist->SetVisibility(!Feet(Surface).IsEmpty());
}

TArray<FVector4> AUghFalls::Feet(double Surface) const
{
	return UghStreams::Feet(Streams.FilterByPredicate([&](const FUghStream& Stream) { return Surface > Stream.Y; }));
}
