#include "UghRockMesh.h"

#include "Async/ParallelFor.h"
#include "Engine/StaticMesh.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "UghMeshes.h"
#include "UghSurfaceNets.h"

namespace
{
	/** A depth of the field (pixels) as a point of the world. */
	FVector World(const FVector& Point)
	{
		return UghShapes::ToWorld(Point.X, Point.Y, Point.Z * UghShapes::UnitsPerPixel);
	}

	/** A direction of the field (pixels) as one of the world. */
	FVector WorldDirection(const FVector& Direction)
	{
		return (World(Direction) - World(FVector::ZeroVector)).GetSafeNormal();
	}

	/** How far from the surface (pixels) the openness looks: near (a crevice) and far (a hollow). */
	constexpr double NearLook = 2, FarLook = 6;
	/** The depth behind the slab where the surface counts as at the back (vertex colour green 1), pixels. */
	constexpr double DeepAt = 60;
	/** The grass hangs this many pixels (here more, there less) over the rock's top edges. */
	constexpr double LipMin = 2, LipMax = 8;
	/** The large patches of the surface: noise this many times per pixel (a few metres), and three times finer. */
	constexpr double PatchScale = 0.025;
	/** The static mesh's only material slot. */
	const FName SlotName(TEXT("Rock"));
}

void FUghRockMesh::Build(const FUghRockField& Field)
{
	Vertices.Reset();
	Normals.Reset();
	UVs.Reset();
	Colors.Reset();
	Triangles.Reset();
	if (Field.IsEmpty())
	{
		return;
	}
	const double Started = FPlatformTime::Seconds();
	TArray<FVector> Points;
	TArray<FUghNetQuad> Quads;
	UghSurfaceNets::Build(Field, Points, Quads);
	Build(Points, Quads, [&](const FVector& Point) { return -Field.Gradient(Point).GetSafeNormal(); },
		[&](const FVector& Point, const FVector& Outward) { return Shade(Field, Point, Outward); });
	UE_LOG(LogTemp, Display, TEXT("UGH rock: %d vertices, %d triangles in %.0f ms"), Vertices.Num(),
		Triangles.Num() / 3, (FPlatformTime::Seconds() - Started) * 1000);
}

void FUghRockMesh::Build(const TArray<FVector>& Points, const TArray<FUghNetQuad>& Quads,
	TFunctionRef<FVector(const FVector&)> Outward, TFunctionRef<FColor(const FVector&, const FVector&)> Color)
{
	Vertices.SetNumUninitialized(Points.Num());
	Normals.SetNumUninitialized(Points.Num());
	UVs.SetNumUninitialized(Points.Num());
	Colors.SetNumUninitialized(Points.Num());
	ParallelFor(Points.Num(), [&](int32 Index)
	{
		const FVector& Point = Points[Index];
		const FVector Out = Outward(Point);
		Vertices[Index] = World(Point);
		Normals[Index] = WorldDirection(Out);
		UVs[Index] = FVector2D(Point.X / UghShapes::ScreenWidth, Point.Y / UghShapes::ScreenHeight);
		Colors[Index] = Color(Point, Out);
	});

	Triangles.Reset(Quads.Num() * 6);
	for (const FUghNetQuad& Quad : Quads)
	{
		const int32* C = Quad.Corners;
		// split along the shorter diagonal; the engine draws a triangle whose corners turn clockwise seen from its front
		const int32 Skip = FVector::DistSquared(Points[C[0]], Points[C[2]]) > FVector::DistSquared(Points[C[1]], Points[C[3]]);
		const int32 A = C[Skip], B = C[(Skip + 1) % 4], M = C[(Skip + 2) % 4], D = C[(Skip + 3) % 4];
		const FVector Normal = FVector::CrossProduct(Vertices[B] - Vertices[A], Vertices[M] - Vertices[A]);
		if ((Normal | WorldDirection(Quad.Outward)) < 0)
		{
			Triangles.Append({ A, B, M, A, M, D });
		}
		else
		{
			Triangles.Append({ A, M, B, A, D, M });
		}
	}
}

UStaticMesh* FUghRockMesh::ToStaticMesh(UObject* Outer) const
{
	if (Vertices.IsEmpty())
	{
		return nullptr;
	}
	FMeshDescription Description;
	Describe(Description);
	return UghMeshes::FromDescription(Outer, Description, SlotName);
}

UStaticMesh* FUghRockMesh::ToStaticMesh(UObject* Outer, const TArray<const FUghRockMesh*>& Lods,
	const TArray<float>& ScreenSizes)
{
	TArray<FMeshDescription> Descriptions;
	Descriptions.SetNum(Lods.Num());
	TArray<const FMeshDescription*> Pointers;
	for (int32 Lod = 0; Lod < Lods.Num(); ++Lod)
	{
		if (Lods[Lod]->Vertices.IsEmpty())
		{
			return nullptr;
		}
		Lods[Lod]->Describe(Descriptions[Lod]);
		Pointers.Add(&Descriptions[Lod]);
	}
	return Pointers.IsEmpty() ? nullptr : UghMeshes::FromDescriptions(Outer, Pointers, SlotName, ScreenSizes);
}

void FUghRockMesh::Describe(FMeshDescription& Description) const
{
	FStaticMeshAttributes Attributes(Description);
	Attributes.Register();
	Description.ReserveNewVertices(Vertices.Num());
	Description.ReserveNewVertexInstances(Vertices.Num());
	Description.ReserveNewTriangles(Triangles.Num() / 3);
	const FPolygonGroupID Group = Description.CreatePolygonGroup();
	Attributes.GetPolygonGroupMaterialSlotNames()[Group] = SlotName;
	const TVertexAttributesRef<FVector3f> Positions = Attributes.GetVertexPositions();
	const TVertexInstanceAttributesRef<FVector3f> InstanceNormals = Attributes.GetVertexInstanceNormals();
	const TVertexInstanceAttributesRef<FVector3f> InstanceTangents = Attributes.GetVertexInstanceTangents();
	const TVertexInstanceAttributesRef<FVector2f> InstanceUVs = Attributes.GetVertexInstanceUVs();
	const TVertexInstanceAttributesRef<FVector4f> InstanceColors = Attributes.GetVertexInstanceColors();
	TArray<FVertexInstanceID> Instances;
	Instances.Reserve(Vertices.Num());
	for (int32 Index = 0; Index < Vertices.Num(); ++Index)
	{
		const FVertexID Vertex = Description.CreateVertex();
		Positions[Vertex] = FVector3f(Vertices[Index]);
		const FVertexInstanceID Instance = Instances.Add_GetRef(Description.CreateVertexInstance(Vertex));
		const FVector3f Normal(Normals[Index]);
		InstanceNormals[Instance] = Normal;
		// any tangent across the normal (the cliff's material works in the world)
		InstanceTangents[Instance] = FVector3f::CrossProduct(Normal, FMath::Abs(Normal.Z) < 0.9f ? FVector3f::UnitZ()
			: FVector3f::UnitX()).GetSafeNormal();
		InstanceUVs.Set(Instance, 0, FVector2f(UVs[Index]));
		// the build stores the colour as sRGB bytes: these become the bytes of Colors again
		InstanceColors[Instance] = FVector4f(FLinearColor(Colors[Index]));
	}
	for (int32 Index = 0; Index < Triangles.Num(); Index += 3)
	{
		Description.CreateTriangle(Group, { Instances[Triangles[Index]], Instances[Triangles[Index + 1]],
			Instances[Triangles[Index + 2]] });
	}
}

FColor FUghRockMesh::Shade(const FUghRockField& Field, const FVector& Point, const FVector& Outward)
{
	// open where the field outside keeps falling as on a flat surface, closed in a crevice or a hollow
	const double Near = FMath::Clamp(-Field.Sample(Point + Outward * NearLook) / NearLook, 0.0, 1.0);
	const double Far = FMath::Clamp(-Field.Sample(Point + Outward * FarLook) / FarLook, 0.0, 1.0);
	// closed too in the passage of a cave's entrance, the further in the more
	double Dark = 0;
	for (const FUghCavePortal& Portal : Field.GetPortals())
	{
		Dark = FMath::Max(Dark, Portal.Darkness(Point));
	}
	const double Deep = FMath::Clamp((Point.Z - FUghRockField::SlabHalf) / DeepAt, 0.0, 1.0);
	// below a top edge of the mask (air above it in the plane of the play): the grass hangs over it, further here
	double Lip = 0;
	if (Point.Z <= FUghRockField::SlabHalf)
	{
		const double Reach = FMath::Lerp(LipMin, LipMax, 0.5 + 0.5 * FMath::PerlinNoise2D(FVector2D(Point) * 0.15));
		for (int32 Up = 0; Up <= FMath::CeilToInt32(Reach); ++Up)
		{
			if (Field.Sample(FVector(Point.X, Point.Y - Up, 0)) < 0)
			{
				Lip = FMath::Max(1 - Up / Reach, 0.0);
				break;
			}
		}
	}
	return FColor(uint8(255 * (0.5 * Near + 0.5 * Far) * (1 - Dark)), uint8(255 * Deep), uint8(255 * Lip),
		uint8(255 * Patches(Point)));
}

double FUghRockMesh::Patches(const FVector& Point)
{
	return FMath::Clamp(0.5 + 0.5 * (0.65 * FMath::PerlinNoise3D(Point * PatchScale) +
		0.35 * FMath::PerlinNoise3D(Point * PatchScale * 3.1)), 0.0, 1.0);
}
