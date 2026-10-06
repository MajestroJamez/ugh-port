#include "UghMeshes.h"

#include "Engine/StaticMesh.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"

UStaticMesh* UghMeshes::Build(UObject* Outer, const TArray<FVertex>& Vertices, const TArray<int32>& Triangles)
{
	FMeshDescription Description;
	FStaticMeshAttributes Attributes(Description);
	Attributes.Register();
	Attributes.GetVertexInstanceUVs().SetNumChannels(2);
	const FName Slot = TEXT("Mesh");
	const FPolygonGroupID Group = Description.CreatePolygonGroup();
	Attributes.GetPolygonGroupMaterialSlotNames()[Group] = Slot;
	const TVertexAttributesRef<FVector3f> Positions = Attributes.GetVertexPositions();
	const TVertexInstanceAttributesRef<FVector3f> Normals = Attributes.GetVertexInstanceNormals();
	const TVertexInstanceAttributesRef<FVector3f> Tangents = Attributes.GetVertexInstanceTangents();
	const TVertexInstanceAttributesRef<FVector2f> UVs = Attributes.GetVertexInstanceUVs();
	TArray<FVertexInstanceID> Instances;
	for (const FVertex& Vertex : Vertices)
	{
		const FVertexID Point = Description.CreateVertex();
		Positions[Point] = FVector3f(Vertex.Position);
		const FVertexInstanceID Instance = Description.CreateVertexInstance(Point);
		Normals[Instance] = Vertex.Normal;
		Tangents[Instance] = Vertex.Tangent;
		UVs.Set(Instance, 0, Vertex.UV0);
		UVs.Set(Instance, 1, Vertex.UV1);
		Instances.Add(Instance);
	}
	for (int32 T = 0; T + 2 < Triangles.Num(); T += 3)
	{
		int32 A = Triangles[T], B = Triangles[T + 1], C = Triangles[T + 2];
		const FVector& PA = Vertices[A].Position;
		const FVector Front = -FVector::CrossProduct(Vertices[B].Position - PA, Vertices[C].Position - PA);
		const FVector3f Wanted = Vertices[A].Normal + Vertices[B].Normal + Vertices[C].Normal;
		if (FVector::DotProduct(Front, FVector(Wanted)) < 0)
		{
			Swap(B, C);
		}
		Description.CreateTriangle(Group, { Instances[A], Instances[B], Instances[C] });
	}
	return UghMeshes::FromDescription(Outer, Description, Slot);
}

UStaticMesh* UghMeshes::Quads(UObject* Outer, const TArray<FQuad>& Quads)
{
	TArray<FVertex> Vertices;
	TArray<int32> Triangles;
	const FVector2f Corners[] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };
	for (const FQuad& Quad : Quads)
	{
		const int32 First = Vertices.Num();
		for (const FVector2f& UV : Corners)
		{
			const FVector At = Quad.Middle + FVector((UV.X - 0.5) * Quad.Size.X, 0, (0.5 - UV.Y) * Quad.Size.Y);
			// towards the camera (world y is minus the depth)
			Vertices.Add({ At, FVector3f::UnitY(), FVector3f::UnitX(), UV, FVector2f(Quad.Seed) });
		}
		Triangles.Append({ First, First + 2, First + 1, First, First + 3, First + 2 });
	}
	return Build(Outer, Vertices, Triangles);
}

UStaticMesh* UghMeshes::FromDescription(UObject* Outer, FMeshDescription& Description, FName Slot)
{
	UStaticMesh* Mesh = NewObject<UStaticMesh>(Outer);
	FStaticMaterial Material(nullptr, Slot);
	Material.UVChannelData = FMeshUVChannelInfo(1.f);
	Mesh->GetStaticMaterials().Add(Material);
	UStaticMesh::FBuildMeshDescriptionsParams Params;
	Params.bFastBuild = true;
	Params.bCommitMeshDescription = false;
	Params.bMarkPackageDirty = false;
	Mesh->BuildFromMeshDescriptions({ &Description }, Params);
	return Mesh;
}
