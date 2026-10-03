#include "UghMakeAssetsCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Texture2D.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionNoise.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UghMaterials.h"

DEFINE_LOG_CATEGORY_STATIC(LogUghMakeAssets, Log, All);

namespace
{
	template <typename TExpression>
	TExpression* Add(UMaterial* Material)
	{
		return Cast<TExpression>(UMaterialEditingLibrary::CreateMaterialExpression(Material, TExpression::StaticClass()));
	}

	UMaterialExpressionConstant* Constant(UMaterial* Material, float Value)
	{
		UMaterialExpressionConstant* Expression = Add<UMaterialExpressionConstant>(Material);
		Expression->R = Value;
		return Expression;
	}

	UMaterialExpressionVectorParameter* Vector(UMaterial* Material, const TCHAR* Name, const FLinearColor& Default)
	{
		UMaterialExpressionVectorParameter* Expression = Add<UMaterialExpressionVectorParameter>(Material);
		Expression->ParameterName = Name;
		Expression->DefaultValue = Default;
		return Expression;
	}

	UMaterialExpressionScalarParameter* Scalar(UMaterial* Material, const TCHAR* Name, float Default)
	{
		UMaterialExpressionScalarParameter* Expression = Add<UMaterialExpressionScalarParameter>(Material);
		Expression->ParameterName = Name;
		Expression->DefaultValue = Default;
		return Expression;
	}

	/** The texture parameter Art (the engine's default texture until the game sets one). */
	UMaterialExpressionTextureSampleParameter2D* ArtTexture(UMaterial* Material)
	{
		UMaterialExpressionTextureSampleParameter2D* Art = Add<UMaterialExpressionTextureSampleParameter2D>(Material);
		Art->ParameterName = UghMaterials::ArtParameter;
		Art->Texture = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EngineResources/DefaultTexture.DefaultTexture"));
		return Art;
	}

	/** A gentle unevenness of hand-made clay: a factor around 1 from noise in world space. */
	UMaterialExpressionNoise* Unevenness(UMaterial* Material)
	{
		UMaterialExpressionNoise* Noise = Add<UMaterialExpressionNoise>(Material);
		Noise->Scale = 0.02f;
		Noise->Quality = 1;
		Noise->OutputMin = 0.8f;
		Noise->OutputMax = 1.1f;
		return Noise;
	}

	UMaterialExpression* Times(UMaterial* Material, UMaterialExpression* A, UMaterialExpression* B)
	{
		UMaterialExpressionMultiply* Multiply = Add<UMaterialExpressionMultiply>(Material);
		UMaterialEditingLibrary::ConnectMaterialExpressions(A, TEXT(""), Multiply, TEXT("A"));
		UMaterialEditingLibrary::ConnectMaterialExpressions(B, TEXT(""), Multiply, TEXT("B"));
		return Multiply;
	}

	/** The material at `Path` without any expressions: the one saved before, or a new one in a package of its own. */
	UMaterial* EmptyMaterial(const TCHAR* Path)
	{
		const FString Name = FPackageName::GetShortName(Path);
		UMaterial* Material = nullptr;
		if (FPackageName::DoesPackageExist(Path))
		{
			Material = FindObject<UMaterial>(LoadPackage(nullptr, Path, LOAD_None), *Name);
		}
		if (Material)
		{
			UMaterialEditingLibrary::DeleteAllMaterialExpressions(Material);
		}
		else
		{
			Material = NewObject<UMaterial>(CreatePackage(Path), *Name, RF_Public | RF_Standalone);
			FAssetRegistryModule::AssetCreated(Material);
		}
		Material->SetMaterialUsage(MATUSAGE_InstancedStaticMeshes);   // the figures are instanced shapes
		return Material;
	}

	bool Save(UMaterial* Material)
	{
		UMaterialEditingLibrary::RecompileMaterial(Material);
		UPackage* Package = Material->GetOutermost();
		const FString File = FPackageName::LongPackageNameToFilename(Package->GetName(),
			FPackageName::GetAssetPackageExtension());
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		const bool bSaved = UPackage::SavePackage(Package, Material, *File, Args);
		UE_LOG(LogUghMakeAssets, Display, TEXT("%s %s"), bSaved ? TEXT("saved") : TEXT("FAILED to save"), *File);
		return bSaved;
	}
}

UUghMakeAssetsCommandlet::UUghMakeAssetsCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UUghMakeAssetsCommandlet::Main(const FString& Params)
{
	struct FRecipe
	{
		const TCHAR* Path;
		void (*Make)(UMaterial*);
	};
	const FRecipe Recipes[] = {
		{ UghMaterials::Clay, &MakeClay }, { UghMaterials::Rock, &MakeRock },
		{ UghMaterials::Water, &MakeWater }, { UghMaterials::Fire, &MakeFire }, { UghMaterials::Sprite, &MakeSprite } };
	bool bAllSaved = true;
	for (const FRecipe& Recipe : Recipes)
	{
		UMaterial* Material = EmptyMaterial(Recipe.Path);
		Recipe.Make(Material);
		bAllSaved = Save(Material) && bAllSaved;
	}
	return bAllSaved ? 0 : 1;
}

void UUghMakeAssetsCommandlet::MakeClay(UMaterial* Material)
{
	UMaterialExpression* Color = Vector(Material, UghMaterials::ColorParameter, FLinearColor(0.5f, 0.5f, 0.5f));
	UMaterialEditingLibrary::ConnectMaterialProperty(Times(Material, Color, Unevenness(Material)), TEXT(""), MP_BaseColor);
	UMaterialEditingLibrary::ConnectMaterialProperty(Constant(Material, 0.7f), TEXT(""), MP_Roughness);
}

void UUghMakeAssetsCommandlet::MakeRock(UMaterial* Material)
{
	UMaterialExpression* Art = ArtTexture(Material);
	UMaterialEditingLibrary::ConnectMaterialProperty(Times(Material, Art, Unevenness(Material)), TEXT(""), MP_BaseColor);
	UMaterialEditingLibrary::ConnectMaterialProperty(Constant(Material, 0.85f), TEXT(""), MP_Roughness);
}

void UUghMakeAssetsCommandlet::MakeWater(UMaterial* Material)
{
	Material->BlendMode = BLEND_Translucent;
	Material->TranslucencyLightingMode = TLM_SurfacePerPixelLighting;
	UMaterialEditingLibrary::ConnectMaterialProperty(
		Vector(Material, UghMaterials::ColorParameter, FLinearColor(0.02f, 0.15f, 0.2f)), TEXT(""), MP_BaseColor);
	UMaterialEditingLibrary::ConnectMaterialProperty(
		Scalar(Material, UghMaterials::OpacityParameter, 0.6f), TEXT(""), MP_Opacity);
	UMaterialEditingLibrary::ConnectMaterialProperty(Constant(Material, 0.05f), TEXT(""), MP_Roughness);
}

void UUghMakeAssetsCommandlet::MakeFire(UMaterial* Material)
{
	Material->BlendMode = BLEND_Additive;
	Material->SetShadingModel(MSM_Unlit);
	UMaterialExpression* Color = Vector(Material, UghMaterials::ColorParameter, FLinearColor(1.f, 0.4f, 0.1f));
	UMaterialExpression* Intensity = Scalar(Material, UghMaterials::IntensityParameter, 20.f);
	UMaterialEditingLibrary::ConnectMaterialProperty(Times(Material, Color, Intensity), TEXT(""), MP_EmissiveColor);
}

void UUghMakeAssetsCommandlet::MakeSprite(UMaterial* Material)
{
	Material->BlendMode = BLEND_Masked;
	Material->SetShadingModel(MSM_Unlit);
	Material->TwoSided = true;
	UMaterialExpression* Art = ArtTexture(Material);
	UMaterialEditingLibrary::ConnectMaterialProperty(Art, TEXT("RGB"), MP_EmissiveColor);
	UMaterialEditingLibrary::ConnectMaterialProperty(Art, TEXT("A"), MP_OpacityMask);
}
