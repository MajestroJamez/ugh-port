#include "UghExportElectricDreamsCommandlet.h"

#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "StaticMeshAttributes.h"
#include "UghElectricDreams.h"

DEFINE_LOG_CATEGORY_STATIC(LogUghExportElectricDreams, Log, All);

namespace
{
	/** Engine units (cm; X forward, Y right, Z up) to Blender's (m; Y mirrored). */
	FVector3f ToBlender(const FVector3f& Point) { return FVector3f(Point.X, -Point.Y, Point.Z); }

	/** A file of the export is there and newer than the asset's package. */
	bool IsCurrent(const FString& File, const UObject* Asset)
	{
		const FString Package = FPackageName::LongPackageNameToFilename(Asset->GetOutermost()->GetName(),
			FPackageName::GetAssetPackageExtension());
		const FDateTime Made = IFileManager::Get().GetTimeStamp(*File);
		return Made != FDateTime::MinValue() && Made >= IFileManager::Get().GetTimeStamp(*Package);
	}
}

UUghExportElectricDreamsCommandlet::UUghExportElectricDreamsCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UUghExportElectricDreamsCommandlet::Main(const FString& Params)
{
	FString Out;
	if (!FParse::Value(*Params, TEXT("Out="), Out))
	{
		UE_LOG(LogUghExportElectricDreams, Error, TEXT("no -Out=<folder>"));
		return 1;
	}
	bool bAll = true;
	for (const TCHAR* Path : UghElectricDreams::ForBlender)
	{
		UObject* Asset = LoadObject<UObject>(nullptr, *UghElectricDreams::ObjectPath(Path));
		if (UTexture* Texture = Cast<UTexture>(Asset))
		{
			// a surface's texture: to the folder named as the surface's (Surfaces/<name>/<texture>)
			const FString Folder = FPaths::ConvertRelativePathToFull(Out / FPaths::GetCleanFilename(FPaths::GetPath(Path)));
			const FString Png = Folder / Texture->GetName() + TEXT(".png");
			if (IsCurrent(Png, Texture))
			{
				continue;
			}
			IFileManager::Get().MakeDirectory(*Folder, true);
			FImage Image;
			const bool bWritten = Texture->Source.IsValid() && Texture->Source.GetMipImage(Image, 0) &&
				FImageUtils::SaveImageByExtension(*Png, Image);
			UE_LOG(LogUghExportElectricDreams, Display, TEXT("%s %s"), bWritten ? TEXT("exported") : TEXT("FAILED"), *Png);
			bAll = bWritten && bAll;
			continue;
		}
		const UStaticMesh* Mesh = Cast<UStaticMesh>(Asset);
		if (!Mesh)
		{
			UE_LOG(LogUghExportElectricDreams, Error, TEXT("no copy of %s (UghCopyElectricDreams first)"), Path);
			bAll = false;
			continue;
		}
		const FString Folder = FPaths::ConvertRelativePathToFull(Out / Mesh->GetName());
		const FString Obj = Folder / Mesh->GetName() + TEXT(".obj");
		if (IsCurrent(Obj, Mesh))
		{
			continue;
		}
		IFileManager::Get().MakeDirectory(*Folder, true);
		const bool bWritten = WriteMaterials(Mesh, Folder) && WriteObj(Mesh, Obj);   // the OBJ last: it says done
		UE_LOG(LogUghExportElectricDreams, Display, TEXT("%s %s"), bWritten ? TEXT("exported") : TEXT("FAILED"), *Obj);
		bAll = bWritten && bAll;
	}
	return bAll ? 0 : 1;
}

bool UUghExportElectricDreamsCommandlet::WriteObj(const UStaticMesh* Mesh, const FString& File)
{
	const FMeshDescription* Description = Mesh->GetMeshDescription(0);
	if (!Description)
	{
		UE_LOG(LogUghExportElectricDreams, Error, TEXT("%s has no source mesh"), *Mesh->GetName());
		return false;
	}
	FStaticMeshConstAttributes Attributes(*Description);
	const TVertexAttributesConstRef<FVector3f> Positions = Attributes.GetVertexPositions();
	const TVertexInstanceAttributesConstRef<FVector3f> Normals = Attributes.GetVertexInstanceNormals();
	const TVertexInstanceAttributesConstRef<FVector2f> Uvs = Attributes.GetVertexInstanceUVs();
	const TPolygonGroupAttributesConstRef<FName> Slots = Attributes.GetPolygonGroupMaterialSlotNames();
	TUniquePtr<FArchive> Writer(IFileManager::Get().CreateFileWriter(*File));
	if (!Writer)
	{
		return false;
	}
	auto Line = [&Writer](const FString& Text)
	{
		const FTCHARToUTF8 Utf8(*(Text + TEXT("\n")));
		Writer->Serialize(const_cast<ANSICHAR*>(Utf8.Get()), Utf8.Length());
	};
	Line(FString::Printf(TEXT("# %s of Epic's Electric Dreams sample (UghExportElectricDreams)"), *Mesh->GetPathName()));
	TMap<FVertexID, int32> VertexIndex;
	for (const FVertexID Vertex : Description->Vertices().GetElementIDs())
	{
		const FVector3f At = ToBlender(Positions[Vertex]) / 100;
		VertexIndex.Add(Vertex, VertexIndex.Num() + 1);
		Line(FString::Printf(TEXT("v %.5f %.5f %.5f"), At.X, At.Y, At.Z));
	}
	TMap<FVertexInstanceID, int32> InstanceIndex;
	for (const FVertexInstanceID Instance : Description->VertexInstances().GetElementIDs())
	{
		const FVector3f Normal = ToBlender(Normals[Instance]);
		const FVector2f Uv = Uvs.Get(Instance, 0);
		InstanceIndex.Add(Instance, InstanceIndex.Num() + 1);
		Line(FString::Printf(TEXT("vt %.6f %.6f"), Uv.X, 1 - Uv.Y));
		Line(FString::Printf(TEXT("vn %.4f %.4f %.4f"), Normal.X, Normal.Y, Normal.Z));
	}
	for (const FPolygonGroupID Group : Description->PolygonGroups().GetElementIDs())
	{
		Line(FString::Printf(TEXT("g %s\nusemtl %s"), *Slots[Group].ToString(), *Slots[Group].ToString()));
		for (const FTriangleID Triangle : Description->Triangles().GetElementIDs())
		{
			if (Description->GetTrianglePolygonGroup(Triangle) != Group)
			{
				continue;
			}
			const TArrayView<const FVertexInstanceID> Corners = Description->GetTriangleVertexInstances(Triangle);
			FString Face = TEXT("f");
			for (const FVertexInstanceID Corner : Corners)   // the engine's clockwise is counter-clockwise mirrored
			{
				const int32 V = VertexIndex[Description->GetVertexInstanceVertex(Corner)];
				const int32 I = InstanceIndex[Corner];
				Face += FString::Printf(TEXT(" %d/%d/%d"), V, I, I);
			}
			Line(Face);
		}
	}
	return Writer->Close();
}

bool UUghExportElectricDreamsCommandlet::WriteMaterials(const UStaticMesh* Mesh, const FString& Folder)
{
	TSharedRef<FJsonObject> List = MakeShared<FJsonObject>();
	bool bAll = true;
	for (const FStaticMaterial& Slot : Mesh->GetStaticMaterials())
	{
		const UMaterialInterface* Material = Slot.MaterialInterface;
		if (!Material)
		{
			continue;
		}
		TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
		Entry->SetStringField(TEXT("material"), Material->GetPathName());
		TArray<FMaterialParameterInfo> Infos;
		TArray<FGuid> Ids;
		Material->GetAllTextureParameterInfo(Infos, Ids);
		TSharedRef<FJsonObject> Textures = MakeShared<FJsonObject>();
		for (const FMaterialParameterInfo& Info : Infos)
		{
			UTexture* Texture = nullptr;
			FImage Image;
			if (!Material->GetTextureParameterValue(Info, Texture) || !Texture || !Texture->Source.IsValid() ||
				!Texture->Source.GetMipImage(Image, 0))
			{
				continue;
			}
			const FString Name = FString::Printf(TEXT("%s_%s.png"), *Slot.MaterialSlotName.ToString(),
				*Info.Name.ToString().Replace(TEXT(" "), TEXT("")));
			if (!FImageUtils::SaveImageByExtension(*(Folder / Name), Image))
			{
				UE_LOG(LogUghExportElectricDreams, Error, TEXT("could not write %s"), *(Folder / Name));
				bAll = false;
			}
			Textures->SetStringField(Info.Name.ToString(), Name);
		}
		Entry->SetObjectField(TEXT("textures"), Textures);
		TSharedRef<FJsonObject> Numbers = MakeShared<FJsonObject>();
		Infos.Reset();
		Material->GetAllScalarParameterInfo(Infos, Ids);
		for (const FMaterialParameterInfo& Info : Infos)
		{
			float Value = 0;
			if (Material->GetScalarParameterValue(Info, Value))
			{
				Numbers->SetNumberField(Info.Name.ToString(), Value);
			}
		}
		Infos.Reset();
		Material->GetAllVectorParameterInfo(Infos, Ids);
		for (const FMaterialParameterInfo& Info : Infos)
		{
			FLinearColor Value;
			if (Material->GetVectorParameterValue(Info, Value))
			{
				Numbers->SetStringField(Info.Name.ToString(), Value.ToString());
			}
		}
		Entry->SetObjectField(TEXT("parameters"), Numbers);
		List->SetObjectField(Slot.MaterialSlotName.ToString(), Entry);
	}
	FString Json;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Json);
	FJsonSerializer::Serialize(List, Writer);
	return FFileHelper::SaveStringToFile(Json, *(Folder / Mesh->GetName() + TEXT(".json"))) && bAll;
}
