// The materials of the events' bursts, made by the commandlet UghMakeAssets: dust, smoke and spray (Burst), debris,
// leaves, feathers and shells (Bits), fire, sparks and glints (Glint).
#include "UghMakeAssetsCommandlet.h"

#include "Materials/Material.h"
#include "Materials/MaterialExpressionCameraPositionWS.h"
#include "Materials/MaterialExpressionCameraVectorWS.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "UghMaterialNodes.h"
#include "UghMaterials.h"

using namespace UghMaterialNodes;

namespace
{
	/** The nodes every burst's material has: how far each particle is in its life, its motion, its look. */
	struct FBurstNodes
	{
		UMaterialExpression* Seed = nullptr;
		UMaterialExpression* AgeLife = nullptr;
		UMaterialExpression* Spin = nullptr;
		UMaterialExpression* Move = nullptr;   // the world position offset
		UMaterialExpression* Look = nullptr;   // the colour and the cover (alpha)
		UMaterialExpression* Color = nullptr;
		bool IsMade() const { return AgeLife && Move && Look; }
	};

	FBurstNodes BurstNodes(UMaterial* Material)
	{
		FBurstNodes Nodes;
		Nodes.Seed = Coordinates(Material, 1);
		UMaterialExpression* UV = Coordinates(Material, 0);
		Nodes.AgeLife = Custom(Material,
			TEXT("float life = Life * (0.7 + 0.3 * frac(Seed.x * 7.13 + Seed.y * 3.71));\n")
			TEXT("float t = Elapsed - Seed.y * Stagger;\n")
			TEXT("return float2(Repeat > 0.5 ? frac(t / life) : t / life, life);"), CMOT_Float2, {
			{ TEXT("Seed"), Nodes.Seed },
			{ UghMaterials::ElapsedParameter, Scalar(Material, UghMaterials::ElapsedParameter, 0.f) },
			{ UghMaterials::LifeParameter, Scalar(Material, UghMaterials::LifeParameter, 1.f) },
			{ UghMaterials::StaggerParameter, Scalar(Material, UghMaterials::StaggerParameter, 0.f) },
			{ UghMaterials::RepeatParameter, Scalar(Material, UghMaterials::RepeatParameter, 0.f) } });
		if (!Nodes.AgeLife)
		{
			return Nodes;
		}
		Nodes.Spin = Scalar(Material, UghMaterials::SpinParameter, 0.f);
		const FString Spin = ShaderCode(TEXT("UghBurstSpin.hlsl")), Motion = ShaderCode(TEXT("UghBurst.hlsl"));
		Nodes.Move = Spin.IsEmpty() || Motion.IsEmpty() ? nullptr : Custom(Material, Spin + Motion, CMOT_Float3, {
			{ TEXT("Position"), Add<UMaterialExpressionWorldPosition>(Material) },
			{ TEXT("UV"), UV },
			{ TEXT("Seed"), Nodes.Seed },
			{ TEXT("CameraPosition"), Add<UMaterialExpressionCameraPositionWS>(Material) },
			{ TEXT("AgeLife"), Nodes.AgeLife },
			{ UghMaterials::DirectionParameter, Vector(Material, UghMaterials::DirectionParameter, FLinearColor(0, 0, 1)) },
			{ UghMaterials::SpeedParameter, Scalar(Material, UghMaterials::SpeedParameter, 0.f) },
			{ UghMaterials::SpreadParameter, Scalar(Material, UghMaterials::SpreadParameter, 1.f) },
			{ UghMaterials::LiftParameter, Scalar(Material, UghMaterials::LiftParameter, 1.f) },
			{ UghMaterials::GravityParameter, Scalar(Material, UghMaterials::GravityParameter, 0.f) },
			{ UghMaterials::DragParameter, Scalar(Material, UghMaterials::DragParameter, 0.f) },
			{ UghMaterials::SizeParameter, Scalar(Material, UghMaterials::SizeParameter, 10.f) },
			{ UghMaterials::GrowParameter, Scalar(Material, UghMaterials::GrowParameter, 1.f) },
			{ UghMaterials::StretchParameter, Scalar(Material, UghMaterials::StretchParameter, 0.f) },
			{ UghMaterials::ModeParameter, Scalar(Material, UghMaterials::ModeParameter, 0.f) },
			{ UghMaterials::FlutterParameter, Scalar(Material, UghMaterials::FlutterParameter, 0.f) },
			{ UghMaterials::SpinParameter, Nodes.Spin },
			{ UghMaterials::BoxParameter, Vector(Material, UghMaterials::BoxParameter, FLinearColor::Black) },
			{ UghMaterials::ScaleParameter, Scalar(Material, UghMaterials::ScaleParameter, 1.f) },
			{ UghMaterials::FloorParameter, Scalar(Material, UghMaterials::FloorParameter, -1e6f) } });
		Nodes.Look = Custom(Material, ShaderCode(TEXT("UghBurstLook.hlsl")), CMOT_Float4, {
			{ TEXT("UV"), UV },
			{ TEXT("Seed"), Nodes.Seed },
			{ TEXT("AgeLife"), Nodes.AgeLife },
			{ UghMaterials::ShapeParameter, Scalar(Material, UghMaterials::ShapeParameter, 0.f) },
			{ TEXT("Where"), Add<UMaterialExpressionWorldPosition>(Material) },
			{ UghMaterials::WaterLevelParameter, Scalar(Material, UghMaterials::WaterLevelParameter, -1e6f) } });
		Nodes.Color = Vector(Material, UghMaterials::ColorParameter, FLinearColor::White);
		return Nodes;
	}

	/** A node of `Code` on the look (Look) and the colour (Color) of `Nodes`, and `Parameter` when there is one. */
	UMaterialExpression* OfLook(UMaterial* Material, const FBurstNodes& Nodes, const TCHAR* Code,
		ECustomMaterialOutputType Type, const TCHAR* Parameter = nullptr, float Default = 0.f)
	{
		TArray<TPair<FName, UMaterialExpression*>> Inputs = { { TEXT("Look"), Nodes.Look }, { TEXT("Color"), Nodes.Color } };
		if (Parameter)
		{
			Inputs.Add({ Parameter, Scalar(Material, Parameter, Default) });
		}
		return Custom(Material, Code, Type, Inputs);
	}
}

bool UUghMakeAssetsCommandlet::MakeBurst(UMaterial* Material)
{
	Material->BlendMode = BLEND_Translucent;
	Material->SetShadingModel(MSM_DefaultLit);
	Material->TranslucencyLightingMode = TLM_SurfacePerPixelLighting;
	Material->TwoSided = true;
	const FBurstNodes Nodes = BurstNodes(Material);
	if (!Nodes.IsMade())
	{
		return false;
	}
	UMaterialEditingLibrary::ConnectMaterialProperty(Nodes.Move, TEXT(""), MP_WorldPositionOffset);
	UMaterialEditingLibrary::ConnectMaterialProperty(
		OfLook(Material, Nodes, TEXT("return Look.rgb * Color;"), CMOT_Float3), TEXT(""), MP_BaseColor);
	UMaterialEditingLibrary::ConnectMaterialProperty(Constant(Material, 1.f), TEXT(""), MP_Roughness);
	UMaterialEditingLibrary::ConnectMaterialProperty(OfLook(Material, Nodes, TEXT("return Look.a * Opacity;"),
		CMOT_Float1, UghMaterials::OpacityParameter, 0.5f), TEXT(""), MP_Opacity);
	return true;
}

bool UUghMakeAssetsCommandlet::MakeBits(UMaterial* Material)
{
	Material->BlendMode = BLEND_Masked;
	Material->SetShadingModel(MSM_DefaultLit);
	Material->TwoSided = true;
	Material->bTangentSpaceNormal = false;   // the normal of the tumbling quad, in the world
	const FBurstNodes Nodes = BurstNodes(Material);
	if (!Nodes.IsMade())
	{
		return false;
	}
	// the quad's normal (towards the camera, +Y) turned as its motion turns it, on the side the camera sees
	UMaterialExpression* Normal = Custom(Material, ShaderCode(TEXT("UghBurstSpin.hlsl")) +
		TEXT("float c = cos(angle), s = sin(angle);\n")
		TEXT("float3 n = float3(0, 1, 0);\n")
		TEXT("n = n * c + cross(axis, n) * s + axis * dot(axis, n) * (1 - c);\n")
		TEXT("return dot(n, ToCamera) < 0 ? -n : n;"), CMOT_Float3, {
		{ TEXT("Seed"), Nodes.Seed }, { TEXT("AgeLife"), Nodes.AgeLife }, { UghMaterials::SpinParameter, Nodes.Spin },
		{ TEXT("ToCamera"), Add<UMaterialExpressionCameraVectorWS>(Material) } });
	UMaterialEditingLibrary::ConnectMaterialProperty(Nodes.Move, TEXT(""), MP_WorldPositionOffset);
	UMaterialEditingLibrary::ConnectMaterialProperty(
		OfLook(Material, Nodes, TEXT("return Look.rgb * Color;"), CMOT_Float3), TEXT(""), MP_BaseColor);
	UMaterialEditingLibrary::ConnectMaterialProperty(Normal, TEXT(""), MP_Normal);
	UMaterialEditingLibrary::ConnectMaterialProperty(Scalar(Material, UghMaterials::RoughnessParameter, 0.7f),
		TEXT(""), MP_Roughness);
	UMaterialEditingLibrary::ConnectMaterialProperty(
		OfLook(Material, Nodes, TEXT("return Look.a;"), CMOT_Float1), TEXT(""), MP_OpacityMask);
	return true;
}

bool UUghMakeAssetsCommandlet::MakeGlint(UMaterial* Material)
{
	Material->BlendMode = BLEND_Additive;
	Material->SetShadingModel(MSM_Unlit);
	Material->TwoSided = true;
	const FBurstNodes Nodes = BurstNodes(Material);
	if (!Nodes.IsMade())
	{
		return false;
	}
	UMaterialEditingLibrary::ConnectMaterialProperty(Nodes.Move, TEXT(""), MP_WorldPositionOffset);
	UMaterialEditingLibrary::ConnectMaterialProperty(OfLook(Material, Nodes,
		TEXT("return Look.rgb * Look.a * Color * Intensity;"), CMOT_Float3, UghMaterials::IntensityParameter, 4.f),
		TEXT(""), MP_EmissiveColor);
	return true;
}
