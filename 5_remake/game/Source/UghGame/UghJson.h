// A JSON file of the extracted data.
#pragma once

#include "CoreMinimal.h"

class FJsonObject;

namespace UghJson
{
	/** The levels' file in the assets: their tiles (FUghLevelArt) and passwords (FUghPasswords). */
	inline const TCHAR* LevelsFile = TEXT("levels.json");

	/** Reads the JSON object in the file at `Path`; false and the reason when it cannot. */
	bool ReadObject(const FString& Path, TSharedPtr<FJsonObject>& OutRoot, FString& OutError);
}
