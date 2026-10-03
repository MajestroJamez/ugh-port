#include "UghSpriteSizes.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

bool FUghSpriteSizes::Load(const FString& Path, FString& OutError)
{
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *Path))
	{
		OutError = FString::Printf(TEXT("cannot read %s (.\\gradlew.bat :extractor:run)"), *Path);
		return false;
	}
	TArray<TSharedPtr<FJsonValue>> Sprites;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Sprites))
	{
		OutError = FString::Printf(TEXT("%s is not a JSON array"), *Path);
		return false;
	}
	Sizes.Reset();
	for (const TSharedPtr<FJsonValue>& Value : Sprites)
	{
		const TSharedPtr<FJsonObject>* Sprite;
		int32 Index, Width, Height;
		if (!Value->TryGetObject(Sprite) || !(*Sprite)->TryGetNumberField(TEXT("index"), Index) ||
			!(*Sprite)->TryGetNumberField(TEXT("width"), Width) || !(*Sprite)->TryGetNumberField(TEXT("height"), Height) ||
			Index < 0)
		{
			OutError = FString::Printf(TEXT("%s: a sprite without index, width and height"), *Path);
			return false;
		}
		if (Index >= Sizes.Num())
		{
			Sizes.SetNum(Index + 1);
		}
		Sizes[Index] = FIntPoint(Width, Height);
	}
	return true;
}

FIntPoint FUghSpriteSizes::Size(int32 Sprite) const
{
	if (Sizes.IsValidIndex(Sprite) && Sizes[Sprite].X > 0 && Sizes[Sprite].Y > 0)
	{
		return Sizes[Sprite];
	}
	return FIntPoint(DefaultSize, DefaultSize);
}
