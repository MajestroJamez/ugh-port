#include "UghMakeAssetsCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionBumpOffset.h"
#include "Materials/MaterialExpressionCameraVectorWS.h"
#include "Materials/MaterialExpressionNoise.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionVertexNormalWS.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UghMaterialNodes.h"
#include "UghMaterials.h"

DEFINE_LOG_CATEGORY_STATIC(LogUghMakeAssets, Log, All);

using namespace UghMaterialNodes;

namespace
{
	/** How many metres one texture of each layer of the cliff covers (UghMaterials::CliffLayers). */
	constexpr float CliffSizes[] = { 4.5f, 3.f, 1.5f, 1.5f, 2.5f };
	static_assert(UE_ARRAY_COUNT(CliffSizes) == UE_ARRAY_COUNT(UghMaterials::CliffLayers));
	/** How far the relief of a texture set shifts its textures with the view (parallax), of its size. */
	constexpr float ParallaxRatio = 0.02f;

	/** The texture parameter Art (the engine's default texture until the game sets one). */
	UMaterialExpressionTextureSampleParameter2D* ArtTexture(UMaterial* Material)
	{
		return TextureSample(Material, UghMaterials::ArtParameter, SAMPLERTYPE_Color, DefaultColor);
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
			// each of a copy: the engine's DeleteAllMaterialExpressions removes from the list it walks (half are left)
			const TArray<UMaterialExpression*> Expressions(Material->GetExpressions());
			for (UMaterialExpression* Expression : Expressions)
			{
				UMaterialEditingLibrary::DeleteMaterialExpression(Material, Expression);
			}
		}
		else
		{
			Material = NewObject<UMaterial>(CreatePackage(Path), *Name, RF_Public | RF_Standalone);
			FAssetRegistryModule::AssetCreated(Material);
		}
		Material->SetMaterialUsage(MATUSAGE_InstancedStaticMeshes);   // the figures are instanced shapes
		Material->SetMaterialUsage(MATUSAGE_Nanite);   // clay on the imported meshes (the campfires' stones)
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
		bool (*Make)(UMaterial*);
	};
	const FRecipe Recipes[] = {
		{ UghMaterials::Clay, &MakeClay }, { UghMaterials::Rock, &MakeRock }, { UghMaterials::Cliff, &MakeCliff },
		{ UghMaterials::Water, &MakeWater }, { UghMaterials::Sprite, &MakeSprite },
		{ UghMaterials::Pbr, &MakePbr }, { UghMaterials::Scan, &MakeScan }, { UghMaterials::Sky, &MakeSky },
		{ UghMaterials::Rain, &MakeRain }, { UghMaterials::Splash, &MakeSplash }, { UghMaterials::Raindrop, &MakeRaindrop },
		{ UghMaterials::Flow, &MakeFlow }, { UghMaterials::Mist, &MakeMist },
		{ UghMaterials::Flame, &MakeFlame }, { UghMaterials::Sparks, &MakeSparks }, { UghMaterials::Smoke, &MakeSmoke },
		{ UghMaterials::Embers, &MakeEmbers }, { UghMaterials::Puff, &MakePuff },
		{ UghMaterials::Membrane, &MakeMembrane }, { UghMaterials::FigureHalo, &MakeFigureHalo },
		{ UghMaterials::Burst, &MakeBurst }, { UghMaterials::Bits, &MakeBits }, { UghMaterials::Glint, &MakeGlint } };
	bool bAllMade = true;
	for (const FRecipe& Recipe : Recipes)
	{
		UMaterial* Material = EmptyMaterial(Recipe.Path);
		bAllMade = Recipe.Make(Material) && Save(Material) && bAllMade;
	}
	return bAllMade ? 0 : 1;
}

bool UUghMakeAssetsCommandlet::MakeClay(UMaterial* Material)
{
	UMaterialExpression* Color = Vector(Material, UghMaterials::ColorParameter, FLinearColor(0.5f, 0.5f, 0.5f));
	UMaterialEditingLibrary::ConnectMaterialProperty(Times(Material, Color, Unevenness(Material)), TEXT(""), MP_BaseColor);
	UMaterialEditingLibrary::ConnectMaterialProperty(Constant(Material, 0.7f), TEXT(""), MP_Roughness);
	return true;
}

bool UUghMakeAssetsCommandlet::MakeRock(UMaterial* Material)
{
	UMaterialExpression* Art = ArtTexture(Material);
	UMaterialEditingLibrary::ConnectMaterialProperty(Times(Material, Art, Unevenness(Material)), TEXT(""), MP_BaseColor);
	UMaterialEditingLibrary::ConnectMaterialProperty(Constant(Material, 0.85f), TEXT(""), MP_Roughness);
	return true;
}

bool UUghMakeAssetsCommandlet::MakeCliff(UMaterial* Material)
{
	Material->bTangentSpaceNormal = false;   // the code's normal is the world's
	UMaterialExpression* Shade = WithAlpha(Material, Add<UMaterialExpressionVertexColor>(Material));
	TArray<TPair<FName, UMaterialExpression*>> Inputs = {
		{ TEXT("Position"), Add<UMaterialExpressionWorldPosition>(Material) },
		{ TEXT("VertexNormal"), Add<UMaterialExpressionVertexNormalWS>(Material) },
		{ TEXT("ScreenUV"), Add<UMaterialExpressionTextureCoordinate>(Material) },
		{ TEXT("Shade"), Shade },
		{ UghMaterials::ArtParameter,
			TextureObject(Material, UghMaterials::ArtParameter, SAMPLERTYPE_Color, DefaultColor) },
		{ UghMaterials::WaterLevelParameter, Scalar(Material, UghMaterials::WaterLevelParameter, -1e6f) } };
	for (int32 Layer = 0; Layer < UE_ARRAY_COUNT(UghMaterials::CliffLayers); ++Layer)
	{
		const FString Name = UghMaterials::CliffLayers[Layer];
		for (const TCHAR* Map : UghMaterials::CliffMaps)
		{
			const bool bNormal = FCString::Strcmp(Map, UghMaterials::NormalParameter) == 0;
			const bool bColor = FCString::Strcmp(Map, UghMaterials::BaseColorParameter) == 0;
			Inputs.Add({ *(Name + Map), TextureObject(Material, Name + Map,
				bNormal ? SAMPLERTYPE_Normal : bColor ? SAMPLERTYPE_Color : SAMPLERTYPE_Masks,
				bNormal ? DefaultNormal : bColor ? DefaultColor : DefaultMasks) });
		}
		Inputs.Add({ *(Name + UghMaterials::SizeParameter),
			Scalar(Material, *(Name + UghMaterials::SizeParameter), CliffSizes[Layer]) });
		Inputs.Add({ *(Name + UghMaterials::HeightMaskParameter),
			Vector(Material, *(Name + UghMaterials::HeightMaskParameter), FLinearColor(1, 0, 0, 0)) });
	}
	UMaterialExpressionCustom* Cliff = Custom(Material, ShaderCode(TEXT("UghCliff.hlsl")), CMOT_Float3, Inputs,
		{ { TEXT("CliffNormal"), CMOT_Float3 }, { TEXT("CliffRough"), CMOT_Float1 },
			{ TEXT("CliffOcclusion"), CMOT_Float1 } });
	if (!Cliff)
	{
		return false;
	}
	UMaterialEditingLibrary::ConnectMaterialProperty(Cliff, TEXT("return"), MP_BaseColor);
	UMaterialEditingLibrary::ConnectMaterialProperty(Cliff, TEXT("CliffNormal"), MP_Normal);
	UMaterialEditingLibrary::ConnectMaterialProperty(Cliff, TEXT("CliffRough"), MP_Roughness);
	UMaterialEditingLibrary::ConnectMaterialProperty(Cliff, TEXT("CliffOcclusion"), MP_AmbientOcclusion);
	return true;
}

bool UUghMakeAssetsCommandlet::MakeSprite(UMaterial* Material)
{
	Material->BlendMode = BLEND_Masked;
	Material->SetShadingModel(MSM_Unlit);
	Material->TwoSided = false;   // a card's back would show its sprite mirrored through what is cut out in front
	UMaterialExpression* Art = ArtTexture(Material);
	UMaterialEditingLibrary::ConnectMaterialProperty(Art, TEXT("RGB"), MP_EmissiveColor);
	UMaterialEditingLibrary::ConnectMaterialProperty(Art, TEXT("A"), MP_OpacityMask);
	return true;
}

bool UUghMakeAssetsCommandlet::MakePbr(UMaterial* Material)
{
	// the instances (UghImportAssets) give the textures; the engine's defaults are of the same sampler types
	UMaterialExpression* Tiled = Times(Material, Add<UMaterialExpressionTextureCoordinate>(Material),
		Scalar(Material, UghMaterials::TilingParameter, 1.f));
	UMaterialExpression* Relief = Custom(Material, TEXT("return Texture2DSample(Height, HeightSampler, UV).r;"),
		CMOT_Float1, { { TEXT("Height"), TextureObject(Material, UghMaterials::HeightParameter, SAMPLERTYPE_Masks,
			DefaultMasks) }, { TEXT("UV"), Tiled } });
	UMaterialExpressionBumpOffset* Coordinates = Add<UMaterialExpressionBumpOffset>(Material);
	Coordinates->HeightRatio = ParallaxRatio;
	UMaterialEditingLibrary::ConnectMaterialExpressions(Tiled, TEXT(""), Coordinates, TEXT("Coordinate"));
	UMaterialEditingLibrary::ConnectMaterialExpressions(Relief, TEXT(""), Coordinates, TEXT("Height"));
	auto Texture = [&](const TCHAR* Name, EMaterialSamplerType Sampler, const TCHAR* Default)
	{
		UMaterialExpressionTextureSampleParameter2D* Sample = TextureSample(Material, Name, Sampler, Default);
		UMaterialEditingLibrary::ConnectMaterialExpressions(Coordinates, TEXT(""), Sample, TEXT("UVs"));
		return Sample;
	};
	UMaterialEditingLibrary::ConnectMaterialProperty(
		Texture(UghMaterials::BaseColorParameter, SAMPLERTYPE_Color, DefaultColor), TEXT("RGB"), MP_BaseColor);
	UMaterialEditingLibrary::ConnectMaterialProperty(
		Texture(UghMaterials::NormalParameter, SAMPLERTYPE_Normal, DefaultNormal), TEXT("RGB"), MP_Normal);
	UMaterialEditingLibrary::ConnectMaterialProperty(
		Texture(UghMaterials::RoughnessParameter, SAMPLERTYPE_Masks, DefaultMasks), TEXT("G"), MP_Roughness);
	UMaterialEditingLibrary::ConnectMaterialProperty(
		Texture(UghMaterials::OcclusionParameter, SAMPLERTYPE_Masks, DefaultMasks), TEXT("R"), MP_AmbientOcclusion);
	return true;
}

bool UUghMakeAssetsCommandlet::MakeScan(UMaterial* Material)
{
	auto Texture = [&](const TCHAR* Name, EMaterialSamplerType Sampler, const TCHAR* Default)
	{
		return TextureSample(Material, Name, Sampler, Default);
	};
	UMaterialExpression* Greyed = Custom(Material, TEXT("float grey = dot(Color, float3(0.3, 0.59, 0.11));\n")
		TEXT("float3 c = lerp(grey.xxx, Color, Saturation) * Tint;\n")
		TEXT("return lerp(c, grey * float3(0.45, 0.6, 0.25), Moss * smoothstep(0.55, 0.9, Up.z));"), CMOT_Float3, {
		{ TEXT("Color"), Texture(UghMaterials::BaseColorParameter, SAMPLERTYPE_Color, DefaultColor) },
		{ TEXT("Up"), Add<UMaterialExpressionVertexNormalWS>(Material) },
		{ TEXT("Tint"), Vector(Material, UghMaterials::ColorParameter, FLinearColor::White) },
		{ TEXT("Saturation"), Scalar(Material, UghMaterials::SaturationParameter, 1.f) },
		{ TEXT("Moss"), Scalar(Material, UghMaterials::MossParameter, 0.f) } });
	UMaterialEditingLibrary::ConnectMaterialProperty(Greyed, TEXT(""), MP_BaseColor);
	UMaterialEditingLibrary::ConnectMaterialProperty(
		Texture(UghMaterials::NormalParameter, SAMPLERTYPE_Normal, DefaultNormal), TEXT("RGB"), MP_Normal);
	UMaterialEditingLibrary::ConnectMaterialProperty(
		Texture(UghMaterials::RoughnessParameter, SAMPLERTYPE_Masks, DefaultMasks), TEXT("G"), MP_Roughness);
	return true;
}

bool UUghMakeAssetsCommandlet::MakeSky(UMaterial* Material)
{
	Material->SetShadingModel(MSM_Unlit);
	Material->TwoSided = true;   // seen from inside its dome
	Material->bIsSky = true;
	UMaterialExpression* Sky = Custom(Material, ShaderCode(TEXT("UghSky.hlsl")), CMOT_Float3, {
		{ TEXT("ToCamera"), Add<UMaterialExpressionCameraVectorWS>(Material) },
		{ UghMaterials::SkyParameter,
			TextureObject(Material, UghMaterials::SkyParameter, SAMPLERTYPE_Color, DefaultCube) },
		{ TEXT("Seen"), Scalar(Material, UghMaterials::SkySeenParameter, 1.f) } });
	if (!Sky)
	{
		return false;
	}
	UMaterialExpression* Tinted = Times(Material, Sky, Vector(Material, UghMaterials::ColorParameter, FLinearColor::White));
	UMaterialEditingLibrary::ConnectMaterialProperty(
		Times(Material, Tinted, Scalar(Material, UghMaterials::IntensityParameter, 1.f)), TEXT(""), MP_EmissiveColor);
	return true;
}
