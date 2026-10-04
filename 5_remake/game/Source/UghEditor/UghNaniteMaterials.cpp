#include "UghNaniteMaterials.h"

#include "Algo/AllOf.h"
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
	/** What the imported models' materials are used with (the decorations are instanced). */
	const EMaterialUsage Usages[] = { MATUSAGE_Nanite, MATUSAGE_SkeletalMesh, MATUSAGE_InstancedStaticMeshes };

	bool IsOurs(const UObject* Object)
	{
		return Object->GetOutermost()->GetName().StartsWith(TEXT("/Game/"));
	}

	bool AllowsAll(const UMaterial* Material)
	{
		return Algo::AllOf(Usages, [&](EMaterialUsage Usage) { return Material->GetUsageByFlag(Usage); });
	}

	/** Gives `Material` every usage of Usages; true when it lacked one. */
	bool AllowAll(UMaterial* Material)
	{
		bool bChanged = false;
		for (EMaterialUsage Usage : Usages)
		{
			if (!Material->GetUsageByFlag(Usage))
			{
				Material->SetMaterialUsage(Usage);
				bChanged = true;
			}
		}
		return bChanged;
	}

	/**
	 * Nanite draws no translucency: an instance that blends (a glTF material with alphaMode BLEND: grass, leaves) is
	 * cut out by its alpha instead. True when it blended.
	 */
	bool CutOut(UMaterialInstanceConstant* Instance)
	{
		FMaterialInstanceBasePropertyOverrides& Overrides = Instance->BasePropertyOverrides;
		if (!Overrides.bOverride_BlendMode || Overrides.BlendMode != BLEND_Translucent)
		{
			return false;
		}
		Overrides.BlendMode = BLEND_Masked;
		Instance->UpdateOverridableBaseProperties();
		return true;
	}

	void Save(UMaterialInterface* Copy)
	{
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		UPackage* Package = Copy->GetOutermost();
		const FString File =
			FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
		const bool bSaved = UPackage::SavePackage(Package, Copy, *File, Args);
		UE_LOG(LogUghNaniteMaterials, Display, TEXT("%s %s"), bSaved ? TEXT("saved") : TEXT("FAILED to save"), *File);
	}

	/** The copy of the engine's material `Engine` (and of its parents) that allows Usages, saved. */
	UMaterialInterface* AllowingCopy(UMaterialInterface* Engine)
	{
		const FString Path = FString(UghAssets::ImportedRoot) / TEXT("_Masters") / Engine->GetName();
		const FString Object = Path + TEXT(".") + Engine->GetName();
		if (UMaterialInterface* Done = LoadObject<UMaterialInterface>(nullptr, *Object, nullptr, LOAD_NoWarn | LOAD_Quiet))
		{
			UMaterial* Root = Done->GetMaterial();   // the material at the root of the copied chain
			if (IsOurs(Root) && AllowAll(Root))
			{
				Root->PostEditChange();
				Save(Root);
			}
			UMaterialInstanceConstant* Instance = Cast<UMaterialInstanceConstant>(Done);
			if (Instance && CutOut(Instance))   // a copy from before the cut-out
			{
				Instance->PostEditChange();
				Save(Instance);
			}
			return Done;
		}
		UPackage* Package = CreatePackage(*Path);
		UMaterialInterface* Copy = DuplicateObject<UMaterialInterface>(Engine, Package, Engine->GetFName());
		Copy->SetFlags(RF_Public | RF_Standalone);
		if (UMaterialInstanceConstant* Instance = Cast<UMaterialInstanceConstant>(Copy))
		{
			CutOut(Instance);
			UMaterialInterface* Parent = Instance->Parent;
			Instance->SetParentEditorOnly(Parent && !IsOurs(Parent) ? AllowingCopy(Parent) : Parent);
		}
		else if (UMaterial* Material = Cast<UMaterial>(Copy))
		{
			AllowAll(Material);
		}
		Copy->PostEditChange();
		FAssetRegistryModule::AssetCreated(Copy);
		Save(Copy);
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
			AllowAll(Material);
		}
		else if (UMaterialInstanceConstant* Instance = Cast<UMaterialInstanceConstant>(Each))
		{
			UMaterialInterface* Parent = Instance->Parent;
			const bool bReparent = Parent && !IsOurs(Parent) && !AllowsAll(Parent->GetMaterial());
			if (bReparent)
			{
				Instance->SetParentEditorOnly(AllowingCopy(Parent));
			}
			if (CutOut(Instance) || bReparent)
			{
				Instance->PostEditChange();
			}
		}
		Each->MarkPackageDirty();
	}
}
