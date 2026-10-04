#include "UghImportAssetsCommandlet.h"

#include "Algo/Find.h"
#include "AssetCompilingManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Texture2D.h"
#include "HAL/FileManager.h"
#include "InterchangeManager.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectIterator.h"
#include "UghAssetManifest.h"
#include "UghAssets.h"
#include "UghMaterials.h"
#include "UghNaniteMaterials.h"

DEFINE_LOG_CATEGORY_STATIC(LogUghImportAssets, Log, All);

namespace
{
	/** Raise it when the import changes: every asset is imported again. */
	constexpr int32 ImportVersion = 5;

	/** A map of a texture set: its texture's settings and the parameters of UghMaterials::Pbr it fills. */
	struct FMapRole
	{
		const TCHAR* Role;
		TextureCompressionSettings Compression;
		bool bSRGB;
		TArray<const TCHAR*> Parameters;
	};
	const FMapRole MapRoles[] = {
		{ TEXT("color"), TC_Default, true, { UghMaterials::BaseColorParameter } },
		{ TEXT("normal"), TC_Normalmap, false, { UghMaterials::NormalParameter } },
		// a packed map: ambient occlusion (red), roughness (green), metalness (blue)
		{ TEXT("arm"), TC_Masks, false, { UghMaterials::OcclusionParameter, UghMaterials::RoughnessParameter } },
		{ TEXT("roughness"), TC_Masks, false, { UghMaterials::RoughnessParameter } },
		{ TEXT("ao"), TC_Masks, false, { UghMaterials::OcclusionParameter } },
		{ TEXT("height"), TC_Grayscale, false, { UghMaterials::HeightParameter } } };   // uncompressed grey

	/** A new material instance `Name` in the content folder `Folder` (the import deleted the one before). */
	UMaterialInstanceConstant* MaterialInstance(const FString& Folder, const FString& Name)
	{
		UMaterialInstanceConstant* Instance =
			NewObject<UMaterialInstanceConstant>(CreatePackage(*(Folder / Name)), *Name, RF_Public | RF_Standalone);
		FAssetRegistryModule::AssetCreated(Instance);
		return Instance;
	}
}

UUghImportAssetsCommandlet::UUghImportAssetsCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UUghImportAssetsCommandlet::Main(const FString& Params)
{
	const FString Manifest = FPaths::ProjectDir() / TEXT("Assets.json");
	TArray<FUghManifestAsset> Assets;
	FString Error;
	if (!UghAssetManifest::Read(Manifest, FPaths::ProjectDir() / TEXT("../.."), Assets, Error))
	{
		UE_LOG(LogUghImportAssets, Error, TEXT("%s"), *Error);
		return 1;
	}
	const bool bForce = FParse::Param(*Params, TEXT("Force"));
	int32 Imported = 0, Current = 0, Skipped = 0, Failed = 0;
	for (const FUghManifestAsset& Asset : Assets)
	{
		const TArray<FString> Missing =
			Asset.Files().FilterByPredicate([](const FString& File) { return !FPaths::FileExists(File); });
		if (!Missing.IsEmpty())
		{
			UE_LOG(LogUghImportAssets, Display, TEXT("skipped %s: no %s (fetch-assets.ps1 downloads it)"), *Asset.Id,
				*Missing[0]);
			++Skipped;
			continue;
		}
		const FString Folder = UghAssets::Folder(Asset.Id);
		const FString Stamp = FPackageName::LongPackageNameToFilename(Folder / TEXT("Import"), TEXT(".stamp"));
		const FString Wanted = Fingerprint(Asset);
		FString Last;
		if (!bForce && FFileHelper::LoadFileToString(Last, *Stamp) && Last == Wanted)
		{
			++Current;
			continue;
		}
		UE_LOG(LogUghImportAssets, Display, TEXT("importing %s (%s) to %s"), *Asset.Id, *Asset.Kind, *Folder);
		// anew: a reimport over the last one would keep what the files no longer have (a texture, a parameter)
		TArray<FString> Before;
		IFileManager::Get().FindFilesRecursive(Before, *FPaths::GetPath(Stamp), TEXT("*.uasset"), true, false);
		IFileManager::Get().DeleteDirectory(*FPaths::GetPath(Stamp), false, true);
		FAssetRegistryModule::GetRegistry().ScanModifiedAssetFiles(Before);   // they are gone
		if (Import(Asset) && SaveFolder(Folder) && FFileHelper::SaveStringToFile(Wanted, *Stamp))
		{
			++Imported;
		}
		else
		{
			UE_LOG(LogUghImportAssets, Error, TEXT("FAILED to import %s"), *Asset.Id);
			++Failed;
		}
	}
	UE_LOG(LogUghImportAssets, Display, TEXT("%d imported, %d up to date, %d not downloaded, %d failed"),
		Imported, Current, Skipped, Failed);
	return Failed == 0 ? 0 : 1;
}

bool UUghImportAssetsCommandlet::Import(const FUghManifestAsset& Asset)
{
	if (Asset.Kind == TEXT("texture"))
	{
		return ImportTextureSet(Asset);
	}
	const bool bFiles = Asset.Kind == TEXT("model") || Asset.Kind == TEXT("hdri") || Asset.Kind == TEXT("generated");
	if (!bFiles || Asset.Import.IsEmpty())
	{
		UE_LOG(LogUghImportAssets, Error, TEXT("%s: a %s with nothing to import"), *Asset.Id, *Asset.Kind);
		return false;
	}
	bool bImported = true;
	for (const FString& File : Asset.Import)
	{
		bImported = !ImportFile(Asset.Folder / File, UghAssets::Folder(Asset.Id)).IsEmpty() && bImported;
	}
	return bImported;
}

bool UUghImportAssetsCommandlet::ImportTextureSet(const FUghManifestAsset& Asset)
{
	const FString Folder = UghAssets::Folder(Asset.Id);
	UMaterialInstanceConstant* Instance = MaterialInstance(Folder, UghAssets::MaterialPrefix + Asset.Id);
	const FString Pbr = FString::Printf(TEXT("%s.%s"), UghMaterials::Pbr, *FPackageName::GetShortName(UghMaterials::Pbr));
	UMaterialInterface* Parent = LoadObject<UMaterialInterface>(nullptr, *Pbr);
	if (!Parent)
	{
		UE_LOG(LogUghImportAssets, Error, TEXT("no material %s: UghMakeAssets makes it"), UghMaterials::Pbr);
		return false;
	}
	Instance->SetParentEditorOnly(Parent);
	for (const TPair<FString, FString>& Map : Asset.Maps)
	{
		const FMapRole* Role =
			Algo::FindByPredicate(MapRoles, [&](const FMapRole& Known) { return Map.Key == Known.Role; });
		UTexture2D* Texture = nullptr;
		for (UObject* Object : ImportFile(Asset.Folder / Map.Value, Folder))
		{
			Texture = Texture ? Texture : Cast<UTexture2D>(Object);
		}
		if (!Role || !Texture)
		{
			UE_LOG(LogUghImportAssets, Error, TEXT("%s: map %s (%s) is no texture of a known role"), *Asset.Id,
				*Map.Key, *Map.Value);
			return false;
		}
		Texture->CompressionSettings = Role->Compression;
		Texture->SRGB = Role->bSRGB;
		Texture->PostEditChange();
		Texture->MarkPackageDirty();
		for (const TCHAR* Parameter : Role->Parameters)
		{
			Instance->SetTextureParameterValueEditorOnly(FMaterialParameterInfo(Parameter), Texture);
		}
	}
	Instance->PostEditChange();
	Instance->MarkPackageDirty();
	return true;
}

TArray<UObject*> UUghImportAssetsCommandlet::ImportFile(const FString& File, const FString& Folder)
{
	FImportAssetParameters Parameters;
	Parameters.bIsAutomated = true;
	Parameters.bReplaceExisting = true;
	TArray<UObject*> Objects;
	UInterchangeManager& Interchange = UInterchangeManager::GetInterchangeManager();
	const bool bImported =
		Interchange.ImportAsset(Folder, UInterchangeManager::CreateSourceData(File), Parameters, Objects);
	if (!bImported || Objects.IsEmpty())
	{
		UE_LOG(LogUghImportAssets, Error, TEXT("Interchange could not import %s"), *File);
		return {};
	}
	return Objects;
}

bool UUghImportAssetsCommandlet::SaveFolder(const FString& Folder)
{
	UghNaniteMaterials::Allow(Folder);
	FAssetCompilingManager::Get().FinishAllCompilation();   // the Nanite meshes and textures are built before saving
	bool bAllSaved = true;
	for (TObjectIterator<UPackage> It; It; ++It)
	{
		UPackage* Package = *It;
		if (!Package->IsDirty() || !Package->GetName().StartsWith(Folder + TEXT("/")))
		{
			continue;
		}
		const FString File =
			FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		const bool bSaved = UPackage::SavePackage(Package, Package->FindAssetInPackage(), *File, Args);
		UE_LOG(LogUghImportAssets, Display, TEXT("%s %s"), bSaved ? TEXT("saved") : TEXT("FAILED to save"), *File);
		bAllSaved = bSaved && bAllSaved;
	}
	return bAllSaved;
}

FString UUghImportAssetsCommandlet::Fingerprint(const FUghManifestAsset& Asset)
{
	FString Fingerprint = FString::Printf(TEXT("import %d\n"), ImportVersion);
	for (const FString& File : Asset.Files())
	{
		const FFileStatData Stat = IFileManager::Get().GetStatData(*File);
		Fingerprint += FString::Printf(TEXT("%s %lld %s\n"), *FPaths::GetCleanFilename(File), Stat.FileSize,
			*Stat.ModificationTime.ToString());
	}
	return Fingerprint;
}
