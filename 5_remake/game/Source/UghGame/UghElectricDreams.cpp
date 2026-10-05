#include "UghElectricDreams.h"

#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialParameterCollection.h"
#include "Misc/PackageName.h"

namespace
{
	/** The asset of type TAsset at `Path`; none when it was not copied (a log line, one for no copy at all). */
	template <typename TAsset>
	TAsset* Find(const TCHAR* Path)
	{
		if (!UghElectricDreams::IsCopied())
		{
			return nullptr;
		}
		const FString Package = FString(UghElectricDreams::Root) / Path;
		if (!FPackageName::DoesPackageExist(Package))
		{
			UE_LOG(LogTemp, Display, TEXT("UGH no %s (electric-dreams.ps1): a simpler look instead"), *Package);
			return nullptr;
		}
		return LoadObject<TAsset>(nullptr, *UghElectricDreams::ObjectPath(Path));
	}
}

bool UghElectricDreams::IsCopied()
{
	static const bool bCopied = [] {
		const bool bThere = FPackageName::DoesPackageExist(FString(Root) / All()[0]);
		UE_CLOG(!bThere, LogTemp, Display,
			TEXT("UGH no copy of the Electric Dreams sample (electric-dreams.ps1): the free assets instead"));
		return bThere;
	}();
	return bCopied;
}

TArray<const TCHAR*> UghElectricDreams::All()
{
	TArray<const TCHAR*> Paths;
	const TConstArrayView<const TCHAR*> Lists[] = { Grasses, Flowers, Rocks, Ferns, Bushes, Plants, Stumps, Palms, Vines,
		Creepers, FireStones, FireLogs, Cliffs, Roots };
	for (const TConstArrayView<const TCHAR*> List : Lists)
	{
		Paths.Append(List.GetData(), List.Num());
	}
	for (const FCliffLayer& Layer : CliffLayers)
	{
		Paths.Append({ Layer.BaseColor, Layer.Normal, Layer.Packed });
	}
	Paths.Add(LeafMaterial);
	return Paths;
}

FString UghElectricDreams::ObjectPath(const TCHAR* Path)
{
	return FString::Printf(TEXT("%s/%s.%s"), Root, Path, *FPackageName::GetShortName(Path));
}

TArray<UStaticMesh*> UghElectricDreams::Meshes(TConstArrayView<const TCHAR*> Paths)
{
	TArray<UStaticMesh*> Meshes;
	for (const TCHAR* Path : Paths)
	{
		if (UStaticMesh* Mesh = Find<UStaticMesh>(Path))
		{
			Meshes.Add(Mesh);
		}
	}
	return Meshes;
}

UTexture* UghElectricDreams::Texture(const TCHAR* Path)
{
	return Find<UTexture>(Path);
}

UMaterialInterface* UghElectricDreams::Material(const TCHAR* Path)
{
	return Find<UMaterialInterface>(Path);
}

UMaterialParameterCollection* UghElectricDreams::Collection(const TCHAR* Path)
{
	return Find<UMaterialParameterCollection>(Path);
}
