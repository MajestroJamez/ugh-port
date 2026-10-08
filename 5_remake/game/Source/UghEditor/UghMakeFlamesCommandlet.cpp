#include "UghMakeFlamesCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Texture2D.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UghFireSim.h"
#include "UghFlames.h"

DEFINE_LOG_CATEGORY_STATIC(LogUghMakeFlames, Log, All);

namespace
{
	/** A flame: its texture, its simulation's grid (cells, as tall as twice its width), its fire, its seed. */
	struct FFlame
	{
		const TCHAR* Texture;
		const TCHAR* Name;
		int32 Cells;
		FUghFireSource Source;
		int32 Seed;
	};
	/**
	 * The campfire: a broad bed of embers feeding several tongues; the torch: a small head feeding one or two. The
	 * fire burns WarmUp seconds before its frames are taken (it grows to its height first).
	 */
	const FFlame Flames[] = {
		{ UghFlames::Campfire, TEXT("campfire"), 160, { 0.3, 0.01, 0.06, 4, 3500, 30, 400, 3.5 }, 1 },
		{ UghFlames::Torch, TEXT("torch"), 160, { 0.18, 0.03, 0.09, 1.5, 3000, 25, 400, 4 }, 2 } };
	constexpr double WarmUp = 3;

	/**
	 * A flame is a slice of air: its light is that of a few slices burning their own ways one behind the other (as deep
	 * as a real flame), each this far aside (a part of the width of its bed of fuel) and this bright.
	 */
	constexpr double LayerAside[] = { 0, -0.15, 0.15 }, LayerLight[] = { 1, 0.7, 0.7 };

	/** The frames of `Flame`'s simulation the flipbook needs: its layers' light added up. */
	TArray<UghFlipbook::FFrame> Simulate(const FFlame& Flame)
	{
		TArray<UghFlipbook::FFrame> Frames;
		for (int32 Layer = 0; Layer < UE_ARRAY_COUNT(LayerAside); ++Layer)
		{
			FUghFireSim Fire(Flame.Cells, Flame.Cells * 2, Flame.Source, Flame.Seed * 10 + Layer);
			Fire.Advance(WarmUp);
			const int32 Aside = FMath::RoundToInt32(LayerAside[Layer] * Flame.Source.Width * Flame.Cells);
			for (int32 Index = 0; Index < UghFlames::Frames + UghFlames::Blend; ++Index)
			{
				Fire.Advance(1 / UghFlames::FramesPerSecond);
				const UghFlipbook::FFrame Slice = Fire.Light();
				if (Layer == 0)
				{
					Frames.Add(UghFlipbook::FFrame{ Slice.Width, Slice.Height, {} });
					Frames.Last().Pixels.Init(FLinearColor::Black, Slice.Pixels.Num());
				}
				for (int32 Pixel = 0; Pixel < Slice.Pixels.Num(); ++Pixel)
				{
					const int32 Column = Pixel % Slice.Width + Aside;
					if (Column >= 0 && Column < Slice.Width)
					{
						Frames[Index].Pixels[Pixel + Aside] += Slice.Pixels[Pixel] * float(LayerLight[Layer]);
					}
				}
			}
		}
		return Frames;
	}

	/** The texture at `Path` (the one saved before, or a new one) showing `Pixels` (sRGB); saved. */
	bool SaveTexture(const TCHAR* Path, int32 Width, int32 Height, const TArray<FColor>& Pixels)
	{
		const FString Name = FPackageName::GetShortName(Path);
		UPackage* Package = FPackageName::DoesPackageExist(Path) ? LoadPackage(nullptr, Path, LOAD_None) : nullptr;
		UTexture2D* Texture = Package ? FindObject<UTexture2D>(Package, *Name) : nullptr;
		if (!Texture)
		{
			Package = CreatePackage(Path);
			Texture = NewObject<UTexture2D>(Package, *Name, RF_Public | RF_Standalone);
			FAssetRegistryModule::AssetCreated(Texture);
		}
		Texture->Source.Init(Width, Height, 1, 1, TSF_BGRA8, reinterpret_cast<const uint8*>(Pixels.GetData()));
		Texture->SRGB = true;
		Texture->CompressionSettings = TC_Default;
		Texture->NeverStream = true;   // all its mips at once: a card shows a small part of it close up
		// an effect's flipbook: not of the world's textures, which the package caps (Config/DefaultDeviceProfiles.ini)
		Texture->LODGroup = TEXTUREGROUP_Effects;
		Texture->PostEditChange();
		Texture->MarkPackageDirty();
		const FString File = FPackageName::LongPackageNameToFilename(Path, FPackageName::GetAssetPackageExtension());
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		const bool bSaved = UPackage::SavePackage(Package, Texture, *File, Args);
		UE_LOG(LogUghMakeFlames, Display, TEXT("%s %s"), bSaved ? TEXT("saved") : TEXT("FAILED to save"), *File);
		return bSaved;
	}
}

UUghMakeFlamesCommandlet::UUghMakeFlamesCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UUghMakeFlamesCommandlet::Main(const FString& Params)
{
	constexpr int32 Width = UghFlames::Columns * UghFlames::CellWidth, Height = UghFlames::Rows * UghFlames::CellHeight;
	bool bAllMade = true;
	for (const FFlame& Flame : Flames)
	{
		const double Started = FPlatformTime::Seconds();
		const TArray<FColor> Pixels = UghFlipbook::Make(Simulate(Flame));
		UE_LOG(LogUghMakeFlames, Display, TEXT("%s: simulated in %.1f s"), Flame.Name, FPlatformTime::Seconds() - Started);
		if (Pixels.IsEmpty())
		{
			UE_LOG(LogUghMakeFlames, Error, TEXT("%s: no flame"), Flame.Name);
			bAllMade = false;
			continue;
		}
		TArray64<uint8> Png;
		FImageUtils::PNGCompressImageArray(Width, Height, TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), Png);
		FFileHelper::SaveArrayToFile(Png, *(FPaths::ProjectSavedDir() / TEXT("Flames") / Flame.Name + TEXT(".png")));
		bAllMade = SaveTexture(Flame.Texture, Width, Height, Pixels) && bAllMade;
	}
	return bAllMade ? 0 : 1;
}
