#include "UghLevelArt.h"

#include "Dom/JsonObject.h"
#include "UghShapes.h"
#include "UghSprites.h"
#include "UghTexture.h"

bool FUghLevelArt::Load(const FJsonObject& LevelsFile, const FString& Path, FString& OutError)
{
	const TArray<TSharedPtr<FJsonValue>>* Levels = nullptr;
	if (!LevelsFile.TryGetArrayField(TEXT("levels"), Levels))
	{
		OutError = FString::Printf(TEXT("%s: no levels"), *Path);
		return false;
	}
	Tiles.Reset();
	for (const TSharedPtr<FJsonValue>& Level : *Levels)
	{
		const TSharedPtr<FJsonObject>* Record;
		const TArray<TSharedPtr<FJsonValue>>* Rows;
		if (!Level->TryGetObject(Record) || !(*Record)->TryGetArrayField(TEXT("tiles"), Rows))
		{
			OutError = FString::Printf(TEXT("%s: a level without tiles"), *Path);
			return false;
		}
		TArray<int32>& LevelTiles = Tiles.AddDefaulted_GetRef();
		for (const TSharedPtr<FJsonValue>& Row : *Rows)
		{
			const TArray<TSharedPtr<FJsonValue>>& Cells = Row->AsArray();
			Columns = Cells.Num();
			for (const TSharedPtr<FJsonValue>& Cell : Cells)
			{
				LevelTiles.Add(static_cast<int32>(Cell->AsNumber()));
			}
		}
	}
	return true;
}

UTexture2D* FUghLevelArt::Draw(UObject* Outer, int32 LevelId, const FUghSprites& Sprites) const
{
	constexpr int32 Width = UghShapes::ScreenWidth, Height = UghShapes::ScreenHeight;
	TArray<FColor> Screen;
	Screen.Init(FColor::Black, Width * Height);
	if (Tiles.IsValidIndex(LevelId) && Columns > 0)
	{
		const TArray<int32>& LevelTiles = Tiles[LevelId];
		for (int32 Index = 0; Index < LevelTiles.Num(); ++Index)
		{
			const int32 Left = Index % Columns * TileWidth, Top = Index / Columns * TileHeight;
			const FIntPoint Size = Sprites.Size(LevelTiles[Index]);
			const TArray<FColor>& Pixels = Sprites.Pixels(LevelTiles[Index]);
			for (int32 Y = 0; Y < Size.Y && !Pixels.IsEmpty(); ++Y)
			{
				for (int32 X = 0; X < Size.X; ++X)
				{
					const FColor Pixel = Pixels[Y * Size.X + X];
					if (Pixel.A > 0 && Left + X < Width && Top + Y < Height)
					{
						Screen[(Top + Y) * Width + Left + X] = Pixel;
					}
				}
			}
		}
	}
	return UghTexture::Create(Outer, Width, Height, Screen, false);
}
