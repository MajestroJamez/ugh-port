#include "UghAssetManifest.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

TArray<FString> FUghManifestAsset::Files() const
{
	TArray<FString> Files;
	for (const TPair<FString, FString>& Map : Maps)
	{
		Files.Add(Folder / Map.Value);
	}
	for (const FString& File : Import)
	{
		Files.Add(Folder / File);
	}
	return Files;
}

bool UghAssetManifest::Read(const FString& Path, const FString& RepositoryDir, TArray<FUghManifestAsset>& OutAssets,
	FString& OutError)
{
	FString Text;
	TSharedPtr<FJsonObject> Manifest;
	if (!FFileHelper::LoadFileToString(Text, *Path) ||
		!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Manifest) || !Manifest.IsValid())
	{
		OutError = FString::Printf(TEXT("%s cannot be read as JSON"), *Path);
		return false;
	}
	const FString Root = FPaths::ConvertRelativePathToFull(RepositoryDir / Manifest->GetStringField(TEXT("folder")));
	OutAssets.Reset();
	for (const TSharedPtr<FJsonValue>& Value : Manifest->GetArrayField(TEXT("assets")))
	{
		const TSharedPtr<FJsonObject>& Entry = Value->AsObject();
		FUghManifestAsset Asset;
		if (!Entry.IsValid() || !Entry->TryGetStringField(TEXT("id"), Asset.Id) ||
			!Entry->TryGetStringField(TEXT("kind"), Asset.Kind) || !Entry->HasTypedField<EJson::String>(TEXT("path")))
		{
			OutError = FString::Printf(TEXT("%s: an asset without id, kind or path"), *Path);
			return false;
		}
		if (Asset.Kind == TEXT("local"))
		{
			continue;   // only an input of a Blender script, nothing to import
		}
		Asset.Folder = Root / Entry->GetStringField(TEXT("path"));
		const TSharedPtr<FJsonObject>* Maps = nullptr;
		if (Entry->TryGetObjectField(TEXT("maps"), Maps))
		{
			for (const TPair<FString, TSharedPtr<FJsonValue>>& Map : (*Maps)->Values)
			{
				Asset.Maps.Add(Map.Key, Map.Value->AsString());
			}
		}
		const TArray<TSharedPtr<FJsonValue>>* Import = nullptr;
		if (Entry->TryGetArrayField(TEXT("import"), Import))
		{
			for (const TSharedPtr<FJsonValue>& File : *Import)
			{
				Asset.Import.Add(File->AsString());
			}
		}
		OutAssets.Add(MoveTemp(Asset));
	}
	return true;
}
