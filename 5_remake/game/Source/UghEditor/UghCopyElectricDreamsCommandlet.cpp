#include "UghCopyElectricDreamsCommandlet.h"

#include "AssetCompilingManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UObject/CoreRedirects.h"
#include "UghElectricDreams.h"

DEFINE_LOG_CATEGORY_STATIC(LogUghCopyElectricDreams, Log, All);

namespace
{
	/** Where the sample's content is mounted (read-only) for the registry. */
	const FString SampleMount = TEXT("/UghSample/");
	/** The sample's packages are /Game/<path> there. */
	const FString SampleGame = TEXT("/Game/");
	/** The files a package may have. */
	const TCHAR* const Extensions[] = { TEXT(".uasset"), TEXT(".umap"), TEXT(".uexp"), TEXT(".ubulk"), TEXT(".uptnl"),
		TEXT(".m.ubulk") };

	/** Content/External/ElectricDreams of this project. */
	FString CopyFolder()
	{
		return FPaths::ConvertRelativePathToFull(FPackageName::LongPackageNameToFilename(
			FString(UghElectricDreams::Root) + TEXT("/")));
	}

	/** The path below the sample's /Game of a dependency (as the sample names it, or redirected here); none else. */
	TOptional<FString> SamplePath(const FString& Dependency)
	{
		const FString Here = FString(UghElectricDreams::Root) + TEXT("/");
		if (Dependency.StartsWith(Here))
		{
			return Dependency.RightChop(Here.Len());
		}
		if (Dependency.StartsWith(SampleGame))
		{
			return Dependency.RightChop(SampleGame.Len());
		}
		return {};
	}
}

UUghCopyElectricDreamsCommandlet::UUghCopyElectricDreamsCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UUghCopyElectricDreamsCommandlet::Main(const FString& Params)
{
	FString Source;
	if (!FParse::Value(*Params, TEXT("Source="), Source))
	{
		UE_LOG(LogUghCopyElectricDreams, Error, TEXT("no -Source=<the Electric Dreams sample's project folder>"));
		return 1;
	}
	const FString Content = FPaths::ConvertRelativePathToFull(Source / TEXT("Content")) / TEXT("");
	if (!IFileManager::Get().DirectoryExists(*Content))
	{
		UE_LOG(LogUghCopyElectricDreams, Error, TEXT("no sample in %s (its Content folder is missing)"), *Source);
		return 1;
	}
	FPackageName::RegisterMountPoint(SampleMount, Content);
	TSet<FString> Packages;
	const bool bGathered = Gather(Content, Packages);
	FPackageName::UnRegisterMountPoint(SampleMount, Content);
	if (!bGathered || !Redirected(Packages))
	{
		return 1;
	}
	int64 Copied = 0, Total = 0;
	if (!Copy(Content, Packages, Copied, Total))
	{
		return 1;
	}
	FAssetRegistryModule::GetRegistry().ScanPathsSynchronous({ UghElectricDreams::Root }, true);
	if (!LoadAll())
	{
		return 1;
	}
	UE_LOG(LogUghCopyElectricDreams, Display, TEXT("%d packages for %d assets, %.2f GB in %s (%.2f GB copied now)"),
		Packages.Num(), UghElectricDreams::All().Num(), Total / 1e9, *CopyFolder(), Copied / 1e9);
	return 0;
}

bool UUghCopyElectricDreamsCommandlet::Gather(const FString& Content, TSet<FString>& Packages)
{
	IAssetRegistry& Registry = FAssetRegistryModule::GetRegistry();
	TArray<FString> Queue;
	for (const TCHAR* Path : UghElectricDreams::All())
	{
		Queue.Add(Path);
	}
	TSet<FString> Outside;
	bool bAll = true;
	while (!Queue.IsEmpty())
	{
		const FString Path = Queue.Pop();
		if (Packages.Contains(Path))
		{
			continue;
		}
		Packages.Add(Path);
		const FString Asset = Content / Path + TEXT(".uasset"), Map = Content / Path + TEXT(".umap");
		const FString* File = FPaths::FileExists(Asset) ? &Asset : FPaths::FileExists(Map) ? &Map : nullptr;
		if (!File)
		{
			UE_LOG(LogUghCopyElectricDreams, Error, TEXT("the sample has no %s%s"), *SampleGame, *Path);
			bAll = false;
			continue;
		}
		Registry.ScanFilesSynchronous({ *File }, true);
		TArray<FName> Dependencies;
		Registry.GetDependencies(FName(SampleMount + Path), Dependencies, UE::AssetRegistry::EDependencyCategory::Package,
			UE::AssetRegistry::EDependencyQuery::Hard);
		for (const FName& Dependency : Dependencies)
		{
			const FString Name = Dependency.ToString();
			if (const TOptional<FString> Needed = SamplePath(Name))
			{
				Queue.Add(*Needed);
			}
			else if (!Name.StartsWith(TEXT("/Engine/")) && !Name.StartsWith(TEXT("/Script/")))
			{
				Outside.Add(Name);
			}
		}
	}
	for (const FString& Name : Outside)
	{
		UE_LOG(LogUghCopyElectricDreams, Warning, TEXT("needed but not in the sample's content (a plugin's?): %s"),
			*Name);
	}
	return bAll;
}

bool UUghCopyElectricDreamsCommandlet::Redirected(const TSet<FString>& Packages)
{
	TSet<FString> Tops;
	for (const FString& Path : Packages)
	{
		FString Top, Rest;
		Tops.Add(Path.Split(TEXT("/"), &Top, &Rest) ? Top : Path);
	}
	bool bAll = true;
	for (const FString& Top : Tops)
	{
		const FString Probe = SampleGame + Top + TEXT("/Probe");
		const FString Wanted = FString(UghElectricDreams::Root) / Top / TEXT("Probe");
		const FCoreRedirectObjectName Redirect = FCoreRedirects::GetRedirectedName(ECoreRedirectFlags::Type_Package,
			FCoreRedirectObjectName(NAME_None, NAME_None, FName(*Probe)));
		if (Redirect.PackageName.ToString() != Wanted)
		{
			UE_LOG(LogUghCopyElectricDreams, Error, TEXT("Config/DefaultEngine.ini lacks the redirect "
				"+PackageRedirects=(OldName=\"%s%s/...\",NewName=\"%s/%s/\",MatchWildcard=true)"),
				*SampleGame, *Top, UghElectricDreams::Root, *Top);
			bAll = false;
		}
	}
	return bAll;
}

bool UUghCopyElectricDreamsCommandlet::Copy(const FString& Content, const TSet<FString>& Packages, int64& Copied,
	int64& Total)
{
	IFileManager& Files = IFileManager::Get();
	const FString Folder = CopyFolder();
	TSet<FString> Wanted;
	bool bAll = true;
	for (const FString& Path : Packages)
	{
		for (const TCHAR* Extension : Extensions)
		{
			const FString From = Content / Path + Extension, To = Folder / Path + Extension;
			const FFileStatData Stat = Files.GetStatData(*From);
			if (!Stat.bIsValid)
			{
				continue;
			}
			Wanted.Add(FPaths::ConvertRelativePathToFull(To));
			Total += Stat.FileSize;
			const FFileStatData There = Files.GetStatData(*To);
			if (There.bIsValid && There.FileSize == Stat.FileSize && There.ModificationTime == Stat.ModificationTime)
			{
				continue;
			}
			if (Files.Copy(*To, *From, true, true) != COPY_OK)
			{
				UE_LOG(LogUghCopyElectricDreams, Error, TEXT("could not copy %s"), *From);
				bAll = false;
				continue;
			}
			Files.SetTimeStamp(*To, Stat.ModificationTime);
			Copied += Stat.FileSize;
		}
	}
	TArray<FString> There;
	Files.FindFilesRecursive(There, *Folder, TEXT("*"), true, false);
	for (const FString& File : There)
	{
		if (!Wanted.Contains(FPaths::ConvertRelativePathToFull(File)))
		{
			UE_LOG(LogUghCopyElectricDreams, Display, TEXT("no longer needed: %s"), *File);
			Files.Delete(*File);
		}
	}
	return bAll;
}

bool UUghCopyElectricDreamsCommandlet::LoadAll()
{
	bool bAll = true;
	for (const TCHAR* Path : UghElectricDreams::All())
	{
		if (!LoadObject<UObject>(nullptr, *UghElectricDreams::ObjectPath(Path)))
		{
			UE_LOG(LogUghCopyElectricDreams, Error, TEXT("the copy of %s does not load"), Path);
			bAll = false;
		}
	}
	FAssetCompilingManager::Get().FinishAllCompilation();
	return bAll;
}
