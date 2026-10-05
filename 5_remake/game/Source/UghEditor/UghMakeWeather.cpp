// The materials of the water and the weather, made by the commandlet UghMakeAssets.
#include "UghMakeAssetsCommandlet.h"

#include "Materials/Material.h"
#include "Materials/MaterialExpressionCameraPositionWS.h"
#include "Materials/MaterialExpressionPixelDepth.h"
#include "Materials/MaterialExpressionSingleLayerWaterMaterialOutput.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialExpressionVertexNormalWS.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "UghMaterialNodes.h"
#include "UghMaterials.h"

using namespace UghMaterialNodes;

namespace
{
	/** Water refracts the view (index of refraction). */
	constexpr float WaterIor = 1.33f;
	/** How much the water scatters light forwards (phase function g). */
	constexpr float WaterPhaseG = 0.2f;

	/**
	 * The depth of the scene behind the water (the engine's class is not exported: made by its name). None when the
	 * engine has no such expression.
	 */
	UMaterialExpression* SceneDepthWithoutWater(UMaterial* Material)
	{
		const UClass* Class = FindObject<UClass>(nullptr, TEXT("/Script/Engine.MaterialExpressionSceneDepthWithoutWater"));
		return Class ? UMaterialEditingLibrary::CreateMaterialExpression(Material, const_cast<UClass*>(Class)) : nullptr;
	}

	/** Texture coordinates of channel `Index`. */
	UMaterialExpression* Coordinates(UMaterial* Material, int32 Index)
	{
		UMaterialExpressionTextureCoordinate* Expression = Add<UMaterialExpressionTextureCoordinate>(Material);
		Expression->CoordinateIndex = Index;
		return Expression;
	}

	/** Translucent and unlit: Color times `Cover` (0 .. 1) times Opacity shines over what is behind. */
	void Glow(UMaterial* Material, UMaterialExpression* Cover, const FLinearColor& Color, float Opacity)
	{
		Material->BlendMode = BLEND_Translucent;
		Material->SetShadingModel(MSM_Unlit);
		Material->TwoSided = true;
		UMaterialEditingLibrary::ConnectMaterialProperty(Vector(Material, UghMaterials::ColorParameter, Color),
			TEXT(""), MP_EmissiveColor);
		UMaterialEditingLibrary::ConnectMaterialProperty(
			Times(Material, Cover, Scalar(Material, UghMaterials::OpacityParameter, Opacity)), TEXT(""), MP_Opacity);
	}
}

bool UUghMakeAssetsCommandlet::MakeWater(UMaterial* Material)
{
	Material->BlendMode = BLEND_Opaque;   // (the material saved before may say otherwise)
	Material->SetShadingModel(MSM_SingleLayerWater);
	Material->bTangentSpaceNormal = false;   // the code's normal is the world's
	Material->SetUsageByFlag(MATUSAGE_Nanite, false);   // Nanite draws no water (a cube of the engine)
	UMaterialExpression* BehindDepth = SceneDepthWithoutWater(Material);
	if (!BehindDepth)
	{
		return false;
	}
	TArray<TPair<FName, UMaterialExpression*>> Inputs = {
		{ TEXT("Position"), Add<UMaterialExpressionWorldPosition>(Material) },
		{ TEXT("VertexNormal"), Add<UMaterialExpressionVertexNormalWS>(Material) },
		{ TEXT("CameraPosition"), Add<UMaterialExpressionCameraPositionWS>(Material) },
		{ TEXT("PixelDepth"), Add<UMaterialExpressionPixelDepth>(Material) },
		{ TEXT("BehindDepth"), BehindDepth },
		{ TEXT("Time"), Add<UMaterialExpressionTime>(Material) },
		{ UghMaterials::WaterLevelParameter, Scalar(Material, UghMaterials::WaterLevelParameter, 0.f) },
		{ UghMaterials::RainParameter, Scalar(Material, UghMaterials::RainParameter, 0.f) },
		{ UghMaterials::WindParameter, Scalar(Material, UghMaterials::WindParameter, 0.f) },
		{ UghMaterials::CausticsParameter, Scalar(Material, UghMaterials::CausticsParameter, 1.f) },
		{ UghMaterials::SunParameter, Vector(Material, UghMaterials::SunParameter, FLinearColor(0.3f, -0.6f, -0.7f)) } };
	for (const TCHAR* Ring : UghMaterials::RingParameters)
	{
		Inputs.Add({ Ring, WithAlpha(Material, Vector(Material, Ring, FLinearColor(0, 0, 0, 0))) });
	}
	// the outputs after "return" (the foam's colour), in this order
	enum EOutput : int32 { Normal = 1, Opacity, Rough, Specular, Scattering, Absorption, Behind };
	UMaterialExpressionCustom* Water = Custom(Material, ShaderCode(TEXT("UghWater.hlsl")), CMOT_Float3, Inputs,
		{ { TEXT("WaterNormal"), CMOT_Float3 }, { TEXT("WaterOpacity"), CMOT_Float1 }, { TEXT("WaterRough"), CMOT_Float1 },
			{ TEXT("WaterSpecular"), CMOT_Float1 }, { TEXT("Scattering"), CMOT_Float3 },
			{ TEXT("Absorption"), CMOT_Float3 }, { TEXT("Behind"), CMOT_Float3 } });
	if (!Water)
	{
		return false;
	}
	UMaterialEditingLibrary::ConnectMaterialProperty(Water, TEXT("return"), MP_BaseColor);
	UMaterialEditingLibrary::ConnectMaterialProperty(Water, TEXT("WaterNormal"), MP_Normal);
	UMaterialEditingLibrary::ConnectMaterialProperty(Water, TEXT("WaterOpacity"), MP_Opacity);
	UMaterialEditingLibrary::ConnectMaterialProperty(Water, TEXT("WaterRough"), MP_Roughness);
	UMaterialEditingLibrary::ConnectMaterialProperty(Water, TEXT("WaterSpecular"), MP_Specular);
	UMaterialEditingLibrary::ConnectMaterialProperty(Constant(Material, WaterIor), TEXT(""), MP_Refraction);
	UMaterialExpressionSingleLayerWaterMaterialOutput* Volume =
		Add<UMaterialExpressionSingleLayerWaterMaterialOutput>(Material);
	Volume->ScatteringCoefficients.Connect(Scattering, Water);
	Volume->AbsorptionCoefficients.Connect(Absorption, Water);
	Volume->PhaseG.Connect(0, Constant(Material, WaterPhaseG));
	Volume->ColorScaleBehindWater.Connect(Behind, Water);
	return true;
}

bool UUghMakeAssetsCommandlet::MakeRain(UMaterial* Material)
{
	UMaterialExpression* Fall = Custom(Material, ShaderCode(TEXT("UghRain.hlsl")), CMOT_Float3, {
		{ TEXT("Position"), Add<UMaterialExpressionWorldPosition>(Material) },
		{ TEXT("UV"), Coordinates(Material, 0) },
		{ TEXT("Seed"), Coordinates(Material, 1) },
		{ TEXT("CameraPosition"), Add<UMaterialExpressionCameraPositionWS>(Material) },
		{ TEXT("Time"), Add<UMaterialExpressionTime>(Material) },
		{ UghMaterials::WindParameter, Scalar(Material, UghMaterials::WindParameter, 0.f) },
		{ UghMaterials::WaterLevelParameter, Scalar(Material, UghMaterials::WaterLevelParameter, 0.f) },
		{ UghMaterials::TopParameter, Scalar(Material, UghMaterials::TopParameter, 2000.f) },
		{ UghMaterials::BottomParameter, Scalar(Material, UghMaterials::BottomParameter, 0.f) },
		{ UghMaterials::LeftParameter, Scalar(Material, UghMaterials::LeftParameter, 0.f) },
		{ UghMaterials::RightParameter, Scalar(Material, UghMaterials::RightParameter, 3200.f) },
		{ UghMaterials::SpeedParameter, Scalar(Material, UghMaterials::SpeedParameter, 2000.f) },
		{ UghMaterials::LengthParameter, Scalar(Material, UghMaterials::LengthParameter, 50.f) },
		{ UghMaterials::WidthParameter, Scalar(Material, UghMaterials::WidthParameter, 2.f) } });
	// a streak: thin at its sides, brightest near its head (v 0), fading towards its tail
	UMaterialExpression* Streak = Custom(Material,
		TEXT("return pow(saturate(1 - abs(UV.x * 2 - 1)), 1.5) * saturate(UV.y * 8) * (1 - UV.y * 0.8);"), CMOT_Float1,
		{ { TEXT("UV"), Coordinates(Material, 0) } });
	if (!Fall || !Streak)
	{
		return false;
	}
	UMaterialEditingLibrary::ConnectMaterialProperty(Fall, TEXT(""), MP_WorldPositionOffset);
	Glow(Material, Streak, FLinearColor(0.6f, 0.65f, 0.7f), 0.45f);
	return true;
}

bool UUghMakeAssetsCommandlet::MakeSplash(UMaterial* Material)
{
	// on the water: moved up from the world's z 0 to its surface
	UMaterialExpression* Seed = Coordinates(Material, 1);
	UMaterialExpression* Lift = Custom(Material, TEXT("return float3(0, 0, Seed.y * WaterLevel);"), CMOT_Float3,
		{ { TEXT("Seed"), Seed },
			{ UghMaterials::WaterLevelParameter, Scalar(Material, UghMaterials::WaterLevelParameter, 0.f) } });
	UMaterialExpression* Splash = Custom(Material, ShaderCode(TEXT("UghSplash.hlsl")), CMOT_Float1, {
		{ TEXT("UV"), Coordinates(Material, 0) },
		{ TEXT("Seed"), Custom(Material, TEXT("return Seed.x;"), CMOT_Float1, { { TEXT("Seed"), Seed } }) },
		{ TEXT("Time"), Add<UMaterialExpressionTime>(Material) } });
	if (!Lift || !Splash)
	{
		return false;
	}
	UMaterialEditingLibrary::ConnectMaterialProperty(Lift, TEXT(""), MP_WorldPositionOffset);
	Glow(Material, Splash, FLinearColor(0.6f, 0.65f, 0.7f), 0.5f);
	return true;
}

bool UUghMakeAssetsCommandlet::MakeRaindrop(UMaterial* Material)
{
	// u along its way (its head at 1), v across
	UMaterialExpression* Streak = Custom(Material,
		TEXT("return pow(saturate(1 - abs(UV.y * 2 - 1)), 1.5) * UV.x * UV.x * saturate((1 - UV.x) * 10);"),
		CMOT_Float1, { { TEXT("UV"), Coordinates(Material, 0) } });
	Glow(Material, Streak, FLinearColor(0.7f, 0.75f, 0.8f), 0.6f);
	return true;
}
