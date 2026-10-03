#include "UghSprites.h"

#include "Dom/JsonObject.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Misc/FileHelper.h"
#include "Modules/ModuleManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	/** The pixels of a PNG as BGRA; empty when it cannot be read. */
	TArray<FColor> ReadPng(const FString& Path)
	{
		TArray<uint8> Bytes;
		TArray<uint8> Raw;
		IImageWrapperModule& Images = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
		TSharedPtr<IImageWrapper> Png = Images.CreateImageWrapper(EImageFormat::PNG);
		if (!FFileHelper::LoadFileToArray(Bytes, *Path) || !Png->SetCompressed(Bytes.GetData(), Bytes.Num()) ||
			!Png->GetRaw(ERGBFormat::BGRA, 8, Raw))
		{
			UE_LOG(LogTemp, Warning, TEXT("UGH cannot read %s"), *Path);
			return {};
		}
		TArray<FColor> Pixels;
		Pixels.SetNumUninitialized(Raw.Num() / sizeof(FColor));
		FMemory::Memcpy(Pixels.GetData(), Raw.GetData(), Raw.Num());
		return Pixels;
	}
}

bool FUghSprites::Load(const FString& AssetsDir, FString& OutError)
{
	Dir = AssetsDir;
	const FString Path = AssetsDir / TEXT("sprites.json");
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *Path))
	{
		OutError = FString::Printf(TEXT("cannot read %s (.\\gradlew.bat :extractor:run)"), *Path);
		return false;
	}
	TArray<TSharedPtr<FJsonValue>> Records;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Records))
	{
		OutError = FString::Printf(TEXT("%s is not a JSON array"), *Path);
		return false;
	}
	Sprites.Reset();
	Loaded.Reset();
	for (const TSharedPtr<FJsonValue>& Value : Records)
	{
		const TSharedPtr<FJsonObject>* Record;
		int32 Index, Width, Height;
		FString File;
		if (!Value->TryGetObject(Record) || !(*Record)->TryGetNumberField(TEXT("index"), Index) ||
			!(*Record)->TryGetNumberField(TEXT("width"), Width) || !(*Record)->TryGetNumberField(TEXT("height"), Height) ||
			Index < 0)
		{
			OutError = FString::Printf(TEXT("%s: a sprite without index, width and height"), *Path);
			return false;
		}
		(*Record)->TryGetStringField(TEXT("file"), File);   // none for a sprite without pixels
		if (Index >= Sprites.Num())
		{
			Sprites.SetNum(Index + 1);
		}
		Sprites[Index] = { FIntPoint(Width, Height), File };
	}
	return true;
}

FIntPoint FUghSprites::Size(int32 Sprite) const
{
	if (Sprites.IsValidIndex(Sprite) && Sprites[Sprite].Size.X > 0 && Sprites[Sprite].Size.Y > 0)
	{
		return Sprites[Sprite].Size;
	}
	return FIntPoint(DefaultSize, DefaultSize);
}

const TArray<FColor>& FUghSprites::Pixels(int32 Sprite) const
{
	if (const TArray<FColor>* Known = Loaded.Find(Sprite))
	{
		return *Known;
	}
	TArray<FColor> Pixels;
	if (Sprites.IsValidIndex(Sprite) && !Sprites[Sprite].File.IsEmpty())
	{
		Pixels = ReadPng(Dir / Sprites[Sprite].File);
		if (Pixels.Num() != Sprites[Sprite].Size.X * Sprites[Sprite].Size.Y)
		{
			Pixels.Reset();   // not the size sprites.json gives
		}
	}
	return Loaded.Add(Sprite, MoveTemp(Pixels));
}
