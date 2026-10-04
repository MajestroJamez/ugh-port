#include "UghAssets.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Engine/StaticMesh.h"

TArray<UStaticMesh*> UghAssets::Meshes(TConstArrayView<const TCHAR*> Ids)
{
	IAssetRegistry& Registry = FAssetRegistryModule::GetRegistry();
	TArray<UStaticMesh*> Meshes;
	for (const TCHAR* Id : Ids)
	{
		const FString Path = Folder(Id);
#if WITH_EDITOR
		Registry.ScanPathsSynchronous({ Path });   // the editor's registry is still scanning at the start
#endif
		TArray<FAssetData> Found;
		Registry.GetAssetsByPath(FName(*Path), Found, true);
		Found.RemoveAll([](const FAssetData& Asset) { return !Asset.IsInstanceOf(UStaticMesh::StaticClass()); });
		Found.Sort([](const FAssetData& A, const FAssetData& B) { return A.AssetName.LexicalLess(B.AssetName); });
		for (const FAssetData& Asset : Found)
		{
			if (UStaticMesh* Mesh = Cast<UStaticMesh>(Asset.GetAsset()))
			{
				Meshes.Add(Mesh);
			}
		}
		if (Found.IsEmpty())
		{
			UE_LOG(LogTemp, Display, TEXT("UGH no asset %s (fetch-assets.ps1, build.ps1): clay shapes instead"), Id);
		}
	}
	return Meshes;
}
