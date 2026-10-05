// The materials of the campfires and torches, made by the commandlet UghMakeAssets: the flame, its sparks, its smoke,
// the burning wood.
#include "UghMakeAssetsCommandlet.h"

#include "Materials/Material.h"
#include "Materials/MaterialExpressionCameraPositionWS.h"
#include "Materials/MaterialExpressionCameraVectorWS.h"
#include "Materials/MaterialExpressionPerInstanceRandom.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialExpressionVertexNormalWS.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "UghFlames.h"
#include "UghMaterialNodes.h"
#include "UghMaterials.h"

using namespace UghMaterialNodes;

namespace
{
	/** The sparks: how high and how far aside they fly, how big (units); the smoke: how high, how big, how grey. */
	constexpr float SparkRise = 160.f, SparkSpread = 40.f, SparkSize = 1.6f;
	constexpr float SmokeRise = 260.f, SmokeSize = 70.f, SmokeOpacity = 0.07f;
	const FLinearColor SmokeColor(0.2f, 0.19f, 0.18f);

	/** How far a spark or a puff is in its life (0 .. 1), each its own (the vertices' and the pixels' alike). */
	UMaterialExpression* Age(UMaterial* Material, const TCHAR* Life)
	{
		return Custom(Material,
			FString::Printf(TEXT("float life = %s; return frac(fmod(Time, 1000.0) / life + Seed.x * 13.7);"), Life),
			CMOT_Float1, { { TEXT("Seed"), Coordinates(Material, 1) },
				{ TEXT("Time"), Add<UMaterialExpressionTime>(Material) } });
	}

	/** The custom node moving the quads of `Shader` (UghSparks.hlsl, UghSmoke.hlsl) by their Age. */
	UMaterialExpression* Moved(UMaterial* Material, const TCHAR* Shader, UMaterialExpression* Aged, float Rise,
		float Size, float Spread)
	{
		return Custom(Material, ShaderCode(Shader), CMOT_Float3, {
			{ TEXT("Position"), Add<UMaterialExpressionWorldPosition>(Material) },
			{ TEXT("UV"), Coordinates(Material, 0) },
			{ TEXT("Seed"), Coordinates(Material, 1) },
			{ TEXT("CameraPosition"), Add<UMaterialExpressionCameraPositionWS>(Material) },
			{ TEXT("Age"), Aged },
			{ UghMaterials::WindParameter, Scalar(Material, UghMaterials::WindParameter, 0.f) },
			{ UghMaterials::RiseParameter, Scalar(Material, UghMaterials::RiseParameter, Rise) },
			{ UghMaterials::SpreadParameter, Scalar(Material, UghMaterials::SpreadParameter, Spread) },
			{ UghMaterials::SizeParameter, Scalar(Material, UghMaterials::SizeParameter, Size) } });
	}
}

bool UUghMakeAssetsCommandlet::MakeFlame(UMaterial* Material)
{
	Material->BlendMode = BLEND_Additive;
	Material->SetShadingModel(MSM_Unlit);
	Material->TwoSided = true;
	UMaterialExpression* Facing = Custom(Material, TEXT("return abs(dot(Normal, ToCamera));"), CMOT_Float1, {
		{ TEXT("Normal"), Add<UMaterialExpressionVertexNormalWS>(Material) },
		{ TEXT("ToCamera"), Add<UMaterialExpressionCameraVectorWS>(Material) } });
	UMaterialExpression* Flipbook =   // the game gives each its flipbook (UghFlames)
		TextureObject(Material, UghMaterials::FlipbookParameter, SAMPLERTYPE_Color, DefaultColor);
	UMaterialExpression* Flame = Custom(Material, ShaderCode(TEXT("UghFlame.hlsl")), CMOT_Float3, {
		{ TEXT("UV"), Coordinates(Material, 0) },
		{ TEXT("Time"), Add<UMaterialExpressionTime>(Material) },
		{ TEXT("Seed"), Add<UMaterialExpressionPerInstanceRandom>(Material) },
		{ TEXT("Facing"), Facing },
		{ UghMaterials::FlipbookParameter, Flipbook },
		{ TEXT("Columns"), Constant(Material, UghFlames::Columns) },
		{ TEXT("Rows"), Constant(Material, UghFlames::Rows) },
		{ TEXT("Rate"), Constant(Material, UghFlames::FramesPerSecond) },
		{ UghMaterials::WindParameter, Scalar(Material, UghMaterials::WindParameter, 0.f) },
		{ UghMaterials::IntensityParameter, Scalar(Material, UghMaterials::IntensityParameter, 1.f) } });
	if (!Flame)
	{
		return false;
	}
	UMaterialEditingLibrary::ConnectMaterialProperty(Flame, TEXT(""), MP_EmissiveColor);
	return true;
}

bool UUghMakeAssetsCommandlet::MakeSparks(UMaterial* Material)
{
	Material->BlendMode = BLEND_Additive;
	Material->SetShadingModel(MSM_Unlit);
	Material->TwoSided = true;
	UMaterialExpression* Aged = Age(Material, TEXT("0.8 + Seed.y * 1.4"));
	UMaterialExpression* Move = Moved(Material, TEXT("UghSparks.hlsl"), Aged, SparkRise, SparkSize, SparkSpread);
	// a soft speck, yellow when it leaves the flame, cooling to a dull red, twinkling
	UMaterialExpression* Speck = Custom(Material, TEXT("float r = length(UV - 0.5) * 2;\n")
		TEXT("float twinkle = 0.6 + 0.4 * sin(Age * 40.0 + Seed.x * 90.0);\n")
		TEXT("float3 c = lerp(float3(1.0, 0.6, 0.18), float3(0.9, 0.12, 0.01), saturate(Age * 1.4));\n")
		TEXT("return c * pow(saturate(1 - r), 1.5) * pow(1 - Age, 1.3) * twinkle * Intensity;"), CMOT_Float3, {
		{ TEXT("UV"), Coordinates(Material, 0) }, { TEXT("Seed"), Coordinates(Material, 1) }, { TEXT("Age"), Aged },
		{ UghMaterials::IntensityParameter, Scalar(Material, UghMaterials::IntensityParameter, 8.f) } });
	if (!Aged || !Move || !Speck)
	{
		return false;
	}
	UMaterialEditingLibrary::ConnectMaterialProperty(Move, TEXT(""), MP_WorldPositionOffset);
	UMaterialEditingLibrary::ConnectMaterialProperty(Speck, TEXT(""), MP_EmissiveColor);
	return true;
}

bool UUghMakeAssetsCommandlet::MakeSmoke(UMaterial* Material)
{
	Material->BlendMode = BLEND_Translucent;
	Material->SetShadingModel(MSM_DefaultLit);
	Material->TranslucencyLightingMode = TLM_SurfacePerPixelLighting;   // lit by its own fire too
	Material->TwoSided = true;
	UMaterialExpression* Aged = Age(Material, TEXT("2.2 + Seed.y * 1.6"));
	UMaterialExpression* Move = Moved(Material, TEXT("UghSmoke.hlsl"), Aged, SmokeRise, SmokeSize, 0.f);
	// a soft wisp (broken by its seed), coming slowly, thinning out as it grows
	UMaterialExpression* Wisp = Custom(Material, TEXT("float2 d = UV - 0.5;\n")
		TEXT("float r = length(d) * 2 + 0.25 * sin(atan2(d.y, d.x) * 3 + Seed.x * 30);\n")
		TEXT("return pow(saturate(1 - r), 1.8) * smoothstep(0, 0.25, Age) * pow(1 - Age, 1.5);"), CMOT_Float1, {
		{ TEXT("UV"), Coordinates(Material, 0) }, { TEXT("Seed"), Coordinates(Material, 1) }, { TEXT("Age"), Aged } });
	if (!Aged || !Move || !Wisp)
	{
		return false;
	}
	UMaterialEditingLibrary::ConnectMaterialProperty(Move, TEXT(""), MP_WorldPositionOffset);
	UMaterialEditingLibrary::ConnectMaterialProperty(Vector(Material, UghMaterials::ColorParameter, SmokeColor),
		TEXT(""), MP_BaseColor);
	UMaterialEditingLibrary::ConnectMaterialProperty(Constant(Material, 1.f), TEXT(""), MP_Roughness);
	UMaterialEditingLibrary::ConnectMaterialProperty(
		Times(Material, Wisp, Scalar(Material, UghMaterials::OpacityParameter, SmokeOpacity)), TEXT(""), MP_Opacity);
	return true;
}

bool UUghMakeAssetsCommandlet::MakeEmbers(UMaterial* Material)
{
	UMaterialExpressionCustom* Embers = Custom(Material, ShaderCode(TEXT("UghEmbers.hlsl")), CMOT_Float3, {
		{ TEXT("Position"), Add<UMaterialExpressionWorldPosition>(Material) },
		{ TEXT("Normal"), Add<UMaterialExpressionVertexNormalWS>(Material) },
		{ TEXT("Time"), Add<UMaterialExpressionTime>(Material) },
		{ UghMaterials::GlowParameter, Scalar(Material, UghMaterials::GlowParameter, 1.f) },
		{ UghMaterials::UndersideParameter, Scalar(Material, UghMaterials::UndersideParameter, 0.f) } },
		{ { TEXT("EmbersLight"), CMOT_Float3 }, { TEXT("EmbersRough"), CMOT_Float1 } });
	if (!Embers)
	{
		return false;
	}
	UMaterialEditingLibrary::ConnectMaterialProperty(Embers, TEXT("return"), MP_BaseColor);
	UMaterialEditingLibrary::ConnectMaterialProperty(Embers, TEXT("EmbersLight"), MP_EmissiveColor);
	UMaterialEditingLibrary::ConnectMaterialProperty(Embers, TEXT("EmbersRough"), MP_Roughness);
	return true;
}
