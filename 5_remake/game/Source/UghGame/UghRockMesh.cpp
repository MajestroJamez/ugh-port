#include "UghRockMesh.h"

#include "Async/ParallelFor.h"
#include "Engine/StaticMesh.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "UghRockFeatures.h"
#include "UghSurfaceNets.h"
#include "ugh_logic.h"

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
	/** The static mesh's only material slot. */
	const FName SlotName(TEXT("Rock"));
}

void FUghRockMesh::Build(const ugh_logic* Logic, TConstArrayView<FColor> Art)
{
	Vertices.Reset();
	Normals.Reset();
	UVs.Reset();
	Colors.Reset();
	Triangles.Reset();
	if (ugh_logic_pad_count(Logic) == 0)
	{
		return;   // no level yet
	}
	const double Started = FPlatformTime::Seconds();
	FUghRockField Field;
	Field.Build(Logic, Art, UghRockFeatures::Plan(Logic));
	TArray<FVector> Points;
	TArray<FUghNetQuad> Quads;
	UghSurfaceNets::Build(Field, Points, Quads);

	Vertices.SetNumUninitialized(Points.Num());
	Normals.SetNumUninitialized(Points.Num());
	UVs.SetNumUninitialized(Points.Num());
	Colors.SetNumUninitialized(Points.Num());
	ParallelFor(Points.Num(), [&](int32 Index)
	{
		const FVector& Point = Points[Index];
		const FVector Outward = -Field.Gradient(Point).GetSafeNormal();
		Vertices[Index] = World(Point);
		Normals[Index] = WorldDirection(Outward);
		UVs[Index] = FVector2D(Point.X / UghShapes::ScreenWidth, Point.Y / UghShapes::ScreenHeight);
		Colors[Index] = Shade(Field, Point, Outward);
	});

	Triangles.Reserve(Quads.Num() * 6);
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
	UE_LOG(LogTemp, Display, TEXT("UGH rock: %d vertices, %d triangles in %.0f ms"), Vertices.Num(),
		Triangles.Num() / 3, (FPlatformTime::Seconds() - Started) * 1000);
}

UStaticMesh* FUghRockMesh::ToStaticMesh(UObject* Outer) const
{
	if (Vertices.IsEmpty())
	{
		return nullptr;
	}
	FMeshDescription Description;
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
	UStaticMesh* Mesh = NewObject<UStaticMesh>(Outer);
	Mesh->GetStaticMaterials().Add(FStaticMaterial(nullptr, SlotName));
	UStaticMesh::FBuildMeshDescriptionsParams Params;
	Params.bFastBuild = true;   // at run time
	Params.bCommitMeshDescription = false;
	Params.bMarkPackageDirty = false;
	Mesh->BuildFromMeshDescriptions({ &Description }, Params);
	return Mesh;
}

FColor FUghRockMesh::Shade(const FUghRockField& Field, const FVector& Point, const FVector& Outward)
{
	// open where the field outside keeps falling as on a flat surface, closed in a crevice or a hollow
	const double Near = FMath::Clamp(-Field.Sample(Point + Outward * NearLook) / NearLook, 0.0, 1.0);
	const double Far = FMath::Clamp(-Field.Sample(Point + Outward * FarLook) / FarLook, 0.0, 1.0);
	const double Deep = FMath::Clamp((Point.Z - FUghRockField::SlabHalf) / DeepAt, 0.0, 1.0);
	return FColor(uint8(255 * (0.5 * Near + 0.5 * Far)), uint8(255 * Deep), 0, 255);
}
