#include "UghJson.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

bool UghJson::ReadObject(const FString& Path, TSharedPtr<FJsonObject>& OutRoot, FString& OutError)
{
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *Path) ||
		!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), OutRoot) || !OutRoot.IsValid())
	{
		OutError = FString::Printf(TEXT("cannot read %s (.\\gradlew.bat :extractor:run)"), *Path);
		return false;
	}
	return true;
}
