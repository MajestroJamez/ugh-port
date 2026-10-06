#include "UghElectricDreamsRepair.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionBreakMaterialAttributes.h"
#include "Materials/MaterialExpressionSubstrate.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

DEFINE_LOG_CATEGORY_STATIC(LogUghElectricDreamsRepair, Log, All);

namespace
{
	/** Cuts the Anisotropy inputs of `Material`'s legacy conversions fed by a break of attributes; whether it cut one. */
	bool CutAnisotropy(UMaterial* Material)
	{
		bool bCut = false;
		for (UMaterialExpression* Expression : Material->GetExpressions())
		{
			UMaterialExpressionSubstrateShadingModels* Conversion = Cast<UMaterialExpressionSubstrateShadingModels>(Expression);
			if (Conversion && Cast<UMaterialExpressionBreakMaterialAttributes>(Conversion->Anisotropy.Expression))
			{
				Conversion->Anisotropy.Expression = nullptr;
				bCut = true;
			}
		}
		return bCut;
	}
}

bool UghElectricDreamsRepair::Materials(const FString& Root)
{
	TArray<FAssetData> Found;
	FAssetRegistryModule::GetRegistry().GetAssetsByPath(FName(*Root), Found, true);
	bool bAll = true;
	for (const FAssetData& Asset : Found)
	{
		if (Asset.AssetClassPath != UMaterial::StaticClass()->GetClassPathName())
		{
			continue;
		}
		UMaterial* Material = Cast<UMaterial>(Asset.GetAsset());
		if (!Material || !CutAnisotropy(Material))
		{
			continue;
		}
		UMaterialEditingLibrary::RecompileMaterial(Material);
		UPackage* Package = Material->GetOutermost();
		const FString File = FPackageName::LongPackageNameToFilename(Package->GetName(),
			FPackageName::GetAssetPackageExtension());
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		const bool bSaved = UPackage::SavePackage(Package, Material, *File, Args);
		UE_LOG(LogUghElectricDreamsRepair, Display, TEXT("anisotropy cut: %s %s"), *File,
			bSaved ? TEXT("saved") : TEXT("NOT SAVED"));
		bAll &= bSaved;
	}
	return bAll;
}
