#include "UghAssets.h"

#include "Animation/AnimSequence.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Materials/MaterialInterface.h"

namespace
{
	/** The assets of `Class` imported for `Id`, in the order of their names; none is logged. */
	TArray<UObject*> Find(const TCHAR* Id, const UClass* Class)
	{
		IAssetRegistry& Registry = FAssetRegistryModule::GetRegistry();
		const FString Path = UghAssets::Folder(Id);
#if WITH_EDITOR
		Registry.ScanPathsSynchronous({ Path });   // the editor's registry is still scanning at the start
#endif
		TArray<FAssetData> Found;
		Registry.GetAssetsByPath(FName(*Path), Found, true);
		Found.RemoveAll([&](const FAssetData& Asset) { return !Asset.IsInstanceOf(Class); });
		Found.Sort([](const FAssetData& A, const FAssetData& B) { return A.AssetName.LexicalLess(B.AssetName); });
		TArray<UObject*> Objects;
		for (const FAssetData& Asset : Found)
		{
			if (UObject* Object = Asset.GetAsset())
			{
				Objects.Add(Object);
			}
		}
		if (Objects.IsEmpty())
		{
			UE_LOG(LogTemp, Display, TEXT("UGH no asset %s (fetch-assets.ps1, build.ps1): a simpler look instead"), Id);
		}
		return Objects;
	}

	/** The asset of type TAsset of `Id` whose name passes `Wanted`; none and a log line (no `What`) without one. */
	template <typename TAsset>
	TAsset* FindOne(const TCHAR* Id, TFunctionRef<bool(const FString&)> Wanted, const TCHAR* What)
	{
		for (UObject* Object : Find(Id, TAsset::StaticClass()))
		{
			if (Wanted(Object->GetName()))
			{
				return CastChecked<TAsset>(Object);
			}
		}
		UE_LOG(LogTemp, Display, TEXT("UGH no %s in asset %s: a simpler look instead"), What, Id);
		return nullptr;
	}
}

TArray<const TCHAR*> UghAssets::All()
{
	TArray<const TCHAR*> Ids;
	const TConstArrayView<const TCHAR*> Lists[] = { Grasses, Flowers, Rocks, Bushes, Plants, CliffSets };
	for (const TConstArrayView<const TCHAR*> List : Lists)
	{
		Ids.Append(List.GetData(), List.Num());
	}
	Ids.Append({ Fern, Stump, Palm, Campfire, Bones, Totem, Hut, Vines, Signs, Torch, BridgeWood, SkyDay, SkyEvening,
		SkyDusk, SkyNight, SkyStorm, Copter, Caveman, StoneSlate, StonePassenger, Pterodactyl, Triceratops, Blower,
		FruitTree, BonusItems, BlowerTrex, WalkerTriceratops, TreeHornbeam });
	TArray<const TCHAR*> Once;
	for (const TCHAR* Id : Ids)
	{
		if (!Once.ContainsByPredicate([&](const TCHAR* Each) { return FCString::Strcmp(Each, Id) == 0; }))
		{
			Once.Add(Id);
		}
	}
	return Once;
}

TArray<UStaticMesh*> UghAssets::Meshes(TConstArrayView<const TCHAR*> Ids)
{
	TArray<UStaticMesh*> Meshes;
	for (const TCHAR* Id : Ids)
	{
		for (UObject* Object : Find(Id, UStaticMesh::StaticClass()))
		{
			Meshes.Add(CastChecked<UStaticMesh>(Object));
		}
	}
	return Meshes;
}

UStaticMesh* UghAssets::Mesh(const TCHAR* Id, const TCHAR* Name)
{
	return FindOne<UStaticMesh>(Id, [&](const FString& Found) { return Found == Name; }, Name);
}

USkeletalMesh* UghAssets::SkeletalMesh(const TCHAR* Id)
{
	return FindOne<USkeletalMesh>(Id, [](const FString&) { return true; }, TEXT("skeletal mesh"));
}

UAnimSequence* UghAssets::Animation(const TCHAR* Id, const TCHAR* Action)
{
	return FindOne<UAnimSequence>(Id, [&](const FString& Found) { return Found.EndsWith(Action); }, Action);
}

UMaterialInterface* UghAssets::Material(const TCHAR* Id)
{
	const FString Name = FString(MaterialPrefix) + Id;
	for (UObject* Object : Find(Id, UMaterialInterface::StaticClass()))
	{
		if (Object->GetName() == Name)
		{
			return CastChecked<UMaterialInterface>(Object);
		}
	}
	return nullptr;
}

UTexture* UghAssets::Texture(const TCHAR* Id)
{
	const TArray<UObject*> Textures = Find(Id, UTexture::StaticClass());
	return Textures.IsEmpty() ? nullptr : CastChecked<UTexture>(Textures[0]);
}

UTexture* UghAssets::Texture(const TCHAR* Id, const TCHAR* Name)
{
	return FindOne<UTexture>(Id, [&](const FString& Found) { return Found == Name; }, Name);
}

UStaticMesh* UghAssets::Stone()
{
	UStaticMesh* Slate = Mesh(StoneSlate, StoneSlate);
	return Slate ? Slate : Mesh(StonePassenger, StonePassenger);
}
