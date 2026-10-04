#include "UghNaniteMaterials.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectIterator.h"
#include "UghAssets.h"

DEFINE_LOG_CATEGORY_STATIC(LogUghNaniteMaterials, Log, All);

namespace
{
	bool IsOurs(const UObject* Object)
	{
		return Object->GetOutermost()->GetName().StartsWith(TEXT("/Game/"));
	}

	/** The copy of the engine's material `Engine` (and of its parents) that allows Nanite meshes, saved. */
	UMaterialInterface* NaniteCopy(UMaterialInterface* Engine)
	{
		const FString Path = FString(UghAssets::ImportedRoot) / TEXT("_Masters") / Engine->GetName();
		const FString Object = Path + TEXT(".") + Engine->GetName();
		if (UMaterialInterface* Done = LoadObject<UMaterialInterface>(nullptr, *Object, nullptr, LOAD_NoWarn | LOAD_Quiet))
		{
			return Done;
		}
		UPackage* Package = CreatePackage(*Path);
		UMaterialInterface* Copy = DuplicateObject<UMaterialInterface>(Engine, Package, Engine->GetFName());
		Copy->SetFlags(RF_Public | RF_Standalone);
		if (UMaterialInstanceConstant* Instance = Cast<UMaterialInstanceConstant>(Copy))
		{
			UMaterialInterface* Parent = Instance->Parent;
			Instance->SetParentEditorOnly(Parent && !IsOurs(Parent) ? NaniteCopy(Parent) : Parent);
		}
		else if (UMaterial* Material = Cast<UMaterial>(Copy))
		{
			Material->SetMaterialUsage(MATUSAGE_Nanite);
		}
		Copy->PostEditChange();
		FAssetRegistryModule::AssetCreated(Copy);
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		const FString File = FPackageName::LongPackageNameToFilename(Path, FPackageName::GetAssetPackageExtension());
		const bool bSaved = UPackage::SavePackage(Package, Copy, *File, Args);
		UE_LOG(LogUghNaniteMaterials, Display, TEXT("%s %s"), bSaved ? TEXT("saved") : TEXT("FAILED to save"), *File);
		return Copy;
	}
}

void UghNaniteMaterials::Allow(const FString& Folder)
{
	TArray<UMaterialInterface*> Imported;   // first: the copies are new objects
	for (TObjectIterator<UMaterialInterface> It; It; ++It)
	{
		if (It->GetOutermost()->GetName().StartsWith(Folder + TEXT("/")))
		{
			Imported.Add(*It);
		}
	}
	for (UMaterialInterface* Each : Imported)
	{
		if (UMaterial* Material = Cast<UMaterial>(Each))
		{
			Material->SetMaterialUsage(MATUSAGE_Nanite);
		}
		else if (UMaterialInstanceConstant* Instance = Cast<UMaterialInstanceConstant>(Each))
		{
			UMaterialInterface* Parent = Instance->Parent;
			if (Parent && !IsOurs(Parent) && !Parent->GetMaterial()->bUsedWithNanite)
			{
				Instance->SetParentEditorOnly(NaniteCopy(Parent));
				Instance->PostEditChange();
			}
		}
		Each->MarkPackageDirty();
	}
}
