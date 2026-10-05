// The nodes the commandlet UghMakeAssets builds its materials of.
#pragma once

#include "CoreMinimal.h"
#include "MaterialEditingLibrary.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionTextureBase.h"

class UMaterial;
class UMaterialExpression;
class UMaterialExpressionTextureObjectParameter;

/** New expressions in a material, as UghMakeAssets wires them. */
namespace UghMaterialNodes
{
	template <typename TExpression>
	TExpression* Add(UMaterial* Material)
	{
		return Cast<TExpression>(UMaterialEditingLibrary::CreateMaterialExpression(Material, TExpression::StaticClass()));
	}

	UMaterialExpression* Constant(UMaterial* Material, float Value);
	UMaterialExpression* Scalar(UMaterial* Material, const TCHAR* Name, float Default);
	UMaterialExpression* Vector(UMaterial* Material, const TCHAR* Name, const FLinearColor& Default);
	/** A texture parameter as an object (for custom code), the engine's texture `Default` until it is set. */
	UMaterialExpressionTextureObjectParameter* TextureObject(UMaterial* Material, const FString& Name,
		EMaterialSamplerType Sampler, const TCHAR* Default);
	UMaterialExpression* Times(UMaterial* Material, UMaterialExpression* A, UMaterialExpression* B);
	/** Texture coordinates of channel `Index`. */
	UMaterialExpression* Coordinates(UMaterial* Material, int32 Index);
	/** The red, green, blue and alpha of `Expression` as one (its first output has only the colour: vertex colours, vector
	 * parameters). */
	UMaterialExpression* WithAlpha(UMaterial* Material, UMaterialExpression* Expression);

	/** The HLSL of Source/UghEditor/Shaders/<File>; empty (and an error in the log) when it cannot be read. */
	FString ShaderCode(const TCHAR* File);
	/**
	 * A custom node running `Code` on its named inputs, returning `Type` (its output "return" when there are `Outputs`
	 * too, else its only output ""); none when the code is empty.
	 */
	UMaterialExpressionCustom* Custom(UMaterial* Material, const FString& Code, ECustomMaterialOutputType Type,
		const TArray<TPair<FName, UMaterialExpression*>>& Inputs, const TArray<FCustomOutput>& Outputs = {});

	/** The engine's textures the parameters show until the game sets theirs. */
	inline const TCHAR* DefaultColor = TEXT("/Engine/EngineResources/DefaultTexture.DefaultTexture");
	inline const TCHAR* DefaultNormal = TEXT("/Engine/EngineMaterials/DefaultNormal.DefaultNormal");
	inline const TCHAR* DefaultMasks = TEXT("/Engine/EngineMaterials/DefaultDiffuse_TC_Masks.DefaultDiffuse_TC_Masks");
	/** A cube (the imported skies are cubes made from their long-lat pictures). */
	inline const TCHAR* DefaultCube = TEXT("/Engine/EngineResources/DefaultTextureCube.DefaultTextureCube");
}
