#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "UObject/ICookInfo.h"
#include "UghAssets.h"
#include "UghElectricDreams.h"
#include "UghMetaHumans.h"

namespace
{
	const FName Instigator(TEXT("UghCookList"));

	/**
	 * UghCookList: the packages the game loads by path, for the cook (it adds what they reference). The imported
	 * assets the frontend asks for (UghAssets::All, each one's whole folder: the frontend finds them in it by name),
	 * the assets of the Electric Dreams sample it uses (UghElectricDreams::All without what only the Blender scripts
	 * read, and the wind of its plants), the MetaHumans (their blueprints and actions). Not whole folders: an import or
	 * a scan no longer used (an older stone, a texture only a Blender script reads, a dependency of an asset the copy
	 * once had) stays out of the package. /Game/Generated is cooked whole (Config/DefaultGame.ini).
	 */
	void AddPlayed(UE::Cook::ICookInfo&, TArray<UE::Cook::FPackageCookRule>& Rules)
	{
		const IAssetRegistry& Registry = FAssetRegistryModule::GetRegistry();
		TArray<FName> Packages;
		const auto AddFolder = [&](const FString& Folder) {
			TArray<FAssetData> Found;
			Registry.GetAssetsByPath(FName(*Folder), Found, true);
			for (const FAssetData& Asset : Found)
			{
				Packages.AddUnique(Asset.PackageName);
			}
		};
		const auto AddPackage = [&](const FString& Package) {
			if (FPackageName::DoesPackageExist(Package))
			{
				Packages.AddUnique(FName(*Package));
			}
		};
		for (const TCHAR* Id : UghAssets::All())
		{
			AddFolder(UghAssets::Folder(Id));
		}
		// (what only Blender wants is in the list once: the taro is a plant of the game too)
		const TArray<const TCHAR*> All = UghElectricDreams::All();
		const auto OnlyForBlender = [&All](const TCHAR* Path)
		{
			int32 Times = 0;
			for (const TCHAR* Each : All)
			{
				Times += FCString::Strcmp(Each, Path) == 0 ? 1 : 0;
			}
			return Times == 1 && TConstArrayView<const TCHAR*>(UghElectricDreams::ForBlender).ContainsByPredicate(
				[Path](const TCHAR* Each) { return FCString::Strcmp(Each, Path) == 0; });
		};
		for (const TCHAR* Path : All)
		{
			if (!OnlyForBlender(Path))
			{
				AddPackage(FString(UghElectricDreams::Root) / Path);
			}
		}
		AddPackage(FString(UghElectricDreams::Root) / UghElectricDreams::FoliageWind);
		for (const TCHAR* Name : UghMetaHumans::Names)
		{
			const FString Folder = FString(UghMetaHumans::Root) / Name;
			AddPackage(Folder / FString::Printf(TEXT("BP_%s"), Name));
			AddFolder(Folder / TEXT("Actions"));
		}
		for (const FName Package : Packages)
		{
			Rules.Add({ Package, Instigator, UE::Cook::EPackageCookRule::AddToCook });
		}
		UE_LOG(LogTemp, Display, TEXT("UGH cook list: %d packages the game loads by path"), Packages.Num());
	}

	class FUghEditorModule : public IModuleInterface
	{
	public:
		virtual void StartupModule() override
		{
			Handle = UE::Cook::FDelegates::ModifyCook.AddStatic(&AddPlayed);
		}

		virtual void ShutdownModule() override
		{
			UE::Cook::FDelegates::ModifyCook.Remove(Handle);
		}

	private:
		FDelegateHandle Handle;
	};
}

IMPLEMENT_MODULE(FUghEditorModule, UghEditor);
