// The materials of the springs' water, made by the commandlet UghMakeAssets: flowing water, a waterfall's mist.
#include "UghMakeAssetsCommandlet.h"

#include "Materials/Material.h"
#include "Materials/MaterialExpressionCameraPositionWS.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "UghMaterialNodes.h"
#include "UghMaterials.h"

using namespace UghMaterialNodes;

namespace
{
	/** The mist: how white its puffs are, how much of what is behind they cover at most; how high and big (units). */
	const FLinearColor MistColor(0.85f, 0.88f, 0.9f);
	constexpr float MistOpacity = 0.6f, MistRise = 110.f, MistSize = 50.f;

	/** Translucent and lit (the light reaching the point, `Lighting`), seen from both sides. */
	void Translucent(UMaterial* Material, ETranslucencyLightingMode Lighting)
	{
		Material->BlendMode = BLEND_Translucent;
		Material->SetShadingModel(MSM_DefaultLit);
		Material->TranslucencyLightingMode = Lighting;
		Material->TwoSided = true;
	}
}

bool UUghMakeAssetsCommandlet::MakeFlow(UMaterial* Material)
{
	Translucent(Material, TLM_SurfacePerPixelLighting);
	UMaterialExpressionCustom* Flow = Custom(Material, ShaderCode(TEXT("UghFlow.hlsl")), CMOT_Float3, {
		{ TEXT("UV"), Coordinates(Material, 0) },
		{ TEXT("Flow"), Coordinates(Material, 1) },
		{ TEXT("Time"), Add<UMaterialExpressionTime>(Material) },
		{ TEXT("Position"), Add<UMaterialExpressionWorldPosition>(Material) },
		{ UghMaterials::WaterLevelParameter, Scalar(Material, UghMaterials::WaterLevelParameter, -1e6f) } },
		{ { TEXT("FlowNormal"), CMOT_Float3 }, { TEXT("FlowOpacity"), CMOT_Float1 }, { TEXT("FlowRough"), CMOT_Float1 } });
	if (!Flow)
	{
		return false;
	}
	UMaterialEditingLibrary::ConnectMaterialProperty(Flow, TEXT("return"), MP_BaseColor);
	UMaterialEditingLibrary::ConnectMaterialProperty(Flow, TEXT("FlowNormal"), MP_Normal);
	UMaterialEditingLibrary::ConnectMaterialProperty(Flow, TEXT("FlowOpacity"), MP_Opacity);
	UMaterialEditingLibrary::ConnectMaterialProperty(Flow, TEXT("FlowRough"), MP_Roughness);
	return true;
}

bool UUghMakeAssetsCommandlet::MakeMist(UMaterial* Material)
{
	Translucent(Material, TLM_SurfacePerPixelLighting);   // (the lighting volume does not reach the cliff)
	UMaterialExpression* Seed = Coordinates(Material, 1);
	// how far a puff is in its life (0 .. 1), each its own (the vertices' and the pixels' alike)
	UMaterialExpression* Age = Custom(Material,
		TEXT("float life = 1.4 + Seed.y * 1.2; return frac(fmod(Time, 1000.0) / life + Seed.x * 13.7);"), CMOT_Float1,
		{ { TEXT("Seed"), Seed }, { TEXT("Time"), Add<UMaterialExpressionTime>(Material) } });
	UMaterialExpression* Move = Custom(Material, ShaderCode(TEXT("UghMist.hlsl")), CMOT_Float3, {
		{ TEXT("Position"), Add<UMaterialExpressionWorldPosition>(Material) },
		{ TEXT("UV"), Coordinates(Material, 0) },
		{ TEXT("Seed"), Seed },
		{ TEXT("CameraPosition"), Add<UMaterialExpressionCameraPositionWS>(Material) },
		{ TEXT("Age"), Age },
		{ UghMaterials::WaterLevelParameter, Scalar(Material, UghMaterials::WaterLevelParameter, 0.f) },
		{ UghMaterials::RiseParameter, Scalar(Material, UghMaterials::RiseParameter, MistRise) },
		{ UghMaterials::SizeParameter, Scalar(Material, UghMaterials::SizeParameter, MistSize) } });
	// a soft puff, coming quickly, fading slowly
	UMaterialExpression* Puff = Custom(Material,
		TEXT("return pow(saturate(1 - length(UV - 0.5) * 2), 1.6) * smoothstep(0, 0.12, Age) * pow(1 - Age, 1.3);"),
		CMOT_Float1, { { TEXT("UV"), Coordinates(Material, 0) }, { TEXT("Age"), Age } });
	if (!Age || !Move || !Puff)
	{
		return false;
	}
	UMaterialEditingLibrary::ConnectMaterialProperty(Move, TEXT(""), MP_WorldPositionOffset);
	UMaterialEditingLibrary::ConnectMaterialProperty(Vector(Material, UghMaterials::ColorParameter, MistColor),
		TEXT(""), MP_BaseColor);
	UMaterialEditingLibrary::ConnectMaterialProperty(
		Times(Material, Puff, Scalar(Material, UghMaterials::OpacityParameter, MistOpacity)), TEXT(""), MP_Opacity);
	return true;
}
