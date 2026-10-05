// The materials of the figures, made by the commandlet UghMakeAssets: the blower's snort of dust, the flyer's wings.
#include "UghMakeAssetsCommandlet.h"

#include "Materials/Material.h"
#include "Materials/MaterialExpressionCameraPositionWS.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "UghMaterialNodes.h"
#include "UghMaterials.h"

using namespace UghMaterialNodes;

namespace
{
	/** The snort: how dusty, how much of what is behind it covers at most, how far it blows and how big (units). */
	const FLinearColor PuffColor(0.5f, 0.43f, 0.34f);
	constexpr float PuffOpacity = 0.5f, PuffReach = 300.f, PuffSize = 90.f;
	/** The wings: how the light coming through them is tinted (warm: blood in the thin skin), how rough they are. */
	const FLinearColor Shine(1.5f, 0.8f, 0.5f);
	constexpr float MembraneRoughness = 0.6f;
}

bool UUghMakeAssetsCommandlet::MakePuff(UMaterial* Material)
{
	Material->BlendMode = BLEND_Translucent;
	Material->SetShadingModel(MSM_DefaultLit);
	Material->TranslucencyLightingMode = TLM_SurfacePerPixelLighting;
	Material->TwoSided = true;
	UMaterialExpression* Seed = Coordinates(Material, 1);
	// how far a puff is in its life (0 .. 1): the puffs leave one after another during the snort (Blow 0 .. 1)
	UMaterialExpression* Age = Custom(Material, TEXT("return saturate((Blow - Seed.y * 0.45) / 0.55);"), CMOT_Float1,
		{ { TEXT("Seed"), Seed }, { UghMaterials::BlowParameter, Scalar(Material, UghMaterials::BlowParameter, 0.f) } });
	UMaterialExpression* Move = Custom(Material, ShaderCode(TEXT("UghPuff.hlsl")), CMOT_Float3, {
		{ TEXT("Position"), Add<UMaterialExpressionWorldPosition>(Material) },
		{ TEXT("UV"), Coordinates(Material, 0) },
		{ TEXT("Seed"), Seed },
		{ TEXT("CameraPosition"), Add<UMaterialExpressionCameraPositionWS>(Material) },
		{ TEXT("Age"), Age },
		{ UghMaterials::DirectionParameter, Vector(Material, UghMaterials::DirectionParameter, FLinearColor(-1, 0, 0)) },
		{ UghMaterials::ReachParameter, Scalar(Material, UghMaterials::ReachParameter, PuffReach) },
		{ UghMaterials::SizeParameter, Scalar(Material, UghMaterials::SizeParameter, PuffSize) } });
	// a soft ragged puff, there at once, thinning out as it spreads
	UMaterialExpression* Puff = Custom(Material, TEXT("float2 d = UV - 0.5;\n")
		TEXT("float r = length(d) * 2 + 0.2 * sin(atan2(d.y, d.x) * 4 + Seed.x * 40);\n")
		TEXT("return pow(saturate(1 - r), 1.5) * smoothstep(0, 0.08, Age) * pow(1 - Age, 1.4);"), CMOT_Float1, {
		{ TEXT("UV"), Coordinates(Material, 0) }, { TEXT("Seed"), Seed }, { TEXT("Age"), Age } });
	if (!Age || !Move || !Puff)
	{
		return false;
	}
	UMaterialEditingLibrary::ConnectMaterialProperty(Move, TEXT(""), MP_WorldPositionOffset);
	UMaterialEditingLibrary::ConnectMaterialProperty(Vector(Material, UghMaterials::ColorParameter, PuffColor),
		TEXT(""), MP_BaseColor);
	UMaterialEditingLibrary::ConnectMaterialProperty(Constant(Material, 1.f), TEXT(""), MP_Roughness);
	UMaterialEditingLibrary::ConnectMaterialProperty(
		Times(Material, Puff, Scalar(Material, UghMaterials::OpacityParameter, PuffOpacity)), TEXT(""), MP_Opacity);
	return true;
}

bool UUghMakeAssetsCommandlet::MakeMembrane(UMaterial* Material)
{
	Material->SetShadingModel(MSM_TwoSidedFoliage);
	Material->TwoSided = true;
	Material->SetMaterialUsage(MATUSAGE_SkeletalMesh);
	UMaterialExpression* Color = TextureSample(Material, UghMaterials::BaseColorParameter, SAMPLERTYPE_Color, DefaultColor);
	UMaterialEditingLibrary::ConnectMaterialProperty(Color, TEXT("RGB"), MP_BaseColor);
	UMaterialEditingLibrary::ConnectMaterialProperty(
		TextureSample(Material, UghMaterials::NormalParameter, SAMPLERTYPE_Normal, DefaultNormal), TEXT("RGB"), MP_Normal);
	UMaterialEditingLibrary::ConnectMaterialProperty(Constant(Material, MembraneRoughness), TEXT(""), MP_Roughness);
	UMaterialEditingLibrary::ConnectMaterialProperty(
		Times(Material, Color, Vector(Material, UghMaterials::ColorParameter, Shine)), TEXT(""), MP_SubsurfaceColor);
	return true;
}
