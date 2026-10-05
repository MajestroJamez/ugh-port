#include "UghMaterialNodes.h"

#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogUghMaterialNodes, Log, All);

UMaterialExpression* UghMaterialNodes::Constant(UMaterial* Material, float Value)
{
	UMaterialExpressionConstant* Expression = Add<UMaterialExpressionConstant>(Material);
	Expression->R = Value;
	return Expression;
}

UMaterialExpression* UghMaterialNodes::Scalar(UMaterial* Material, const TCHAR* Name, float Default)
{
	UMaterialExpressionScalarParameter* Expression = Add<UMaterialExpressionScalarParameter>(Material);
	Expression->ParameterName = Name;
	Expression->DefaultValue = Default;
	return Expression;
}

UMaterialExpression* UghMaterialNodes::Vector(UMaterial* Material, const TCHAR* Name, const FLinearColor& Default)
{
	UMaterialExpressionVectorParameter* Expression = Add<UMaterialExpressionVectorParameter>(Material);
	Expression->ParameterName = Name;
	Expression->DefaultValue = Default;
	return Expression;
}

UMaterialExpressionTextureSampleParameter2D* UghMaterialNodes::TextureSample(UMaterial* Material, const TCHAR* Name,
	EMaterialSamplerType Sampler, const TCHAR* Default)
{
	UMaterialExpressionTextureSampleParameter2D* Sample = Add<UMaterialExpressionTextureSampleParameter2D>(Material);
	Sample->ParameterName = Name;
	Sample->SamplerType = Sampler;
	Sample->Texture = LoadObject<UTexture2D>(nullptr, Default);
	return Sample;
}

UMaterialExpressionTextureObjectParameter* UghMaterialNodes::TextureObject(UMaterial* Material, const FString& Name,
	EMaterialSamplerType Sampler, const TCHAR* Default)
{
	UMaterialExpressionTextureObjectParameter* Expression = Add<UMaterialExpressionTextureObjectParameter>(Material);
	Expression->ParameterName = *Name;
	Expression->SamplerType = Sampler;
	Expression->Texture = LoadObject<UTexture>(nullptr, Default);
	return Expression;
}

UMaterialExpression* UghMaterialNodes::Times(UMaterial* Material, UMaterialExpression* A, UMaterialExpression* B)
{
	UMaterialExpressionMultiply* Multiply = Add<UMaterialExpressionMultiply>(Material);
	UMaterialEditingLibrary::ConnectMaterialExpressions(A, TEXT(""), Multiply, TEXT("A"));
	UMaterialEditingLibrary::ConnectMaterialExpressions(B, TEXT(""), Multiply, TEXT("B"));
	return Multiply;
}

UMaterialExpression* UghMaterialNodes::Coordinates(UMaterial* Material, int32 Index)
{
	UMaterialExpressionTextureCoordinate* Expression = Add<UMaterialExpressionTextureCoordinate>(Material);
	Expression->CoordinateIndex = Index;
	return Expression;
}

UMaterialExpression* UghMaterialNodes::WithAlpha(UMaterial* Material, UMaterialExpression* Expression)
{
	constexpr int32 AlphaOutput = 4;
	UMaterialExpressionAppendVector* Append = Add<UMaterialExpressionAppendVector>(Material);
	Append->A.Connect(0, Expression);
	Append->B.Connect(AlphaOutput, Expression);
	return Append;
}

FString UghMaterialNodes::ShaderCode(const TCHAR* File)
{
	const FString Path = FPaths::ProjectDir() / TEXT("Source/UghEditor/Shaders") / File;
	FString Code;
	if (!FFileHelper::LoadFileToString(Code, *Path))
	{
		UE_LOG(LogUghMaterialNodes, Error, TEXT("cannot read %s"), *Path);
		return FString();
	}
	return Code;
}

UMaterialExpressionCustom* UghMaterialNodes::Custom(UMaterial* Material, const FString& Code,
	ECustomMaterialOutputType Type, const TArray<TPair<FName, UMaterialExpression*>>& Inputs,
	const TArray<FCustomOutput>& Outputs)
{
	if (Code.IsEmpty())
	{
		return nullptr;
	}
	UMaterialExpressionCustom* Node = Add<UMaterialExpressionCustom>(Material);
	Node->Code = Code;
	Node->OutputType = Type;
	Node->Inputs.Reset();
	for (const TPair<FName, UMaterialExpression*>& Input : Inputs)
	{
		FCustomInput& Added = Node->Inputs.AddDefaulted_GetRef();
		Added.InputName = Input.Key;
		Added.Input.Connect(0, Input.Value);
	}
	Node->AdditionalOutputs = Outputs;
	Node->RebuildOutputs();
	return Node;
}
