#include "UghProfile.h"

#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "UghJson.h"

FString FUghProfile::DefaultPath()
{
	FString Path;
	if (FParse::Value(FCommandLine::Get(), TEXT("-UghProfile="), Path))
	{
		return FPaths::ConvertRelativePathToFull(Path);
	}
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("UghProfile.json"));
}

bool FUghProfile::Load(const FString& Path, FString& OutError)
{
	*this = FUghProfile();
	if (!FPaths::FileExists(Path))
	{
		return true;
	}
	TSharedPtr<FJsonObject> Root;
	if (!UghJson::ReadObject(Path, Root, OutError))
	{
		return false;
	}
	const TSharedPtr<FJsonObject>* Part = nullptr;
	if (Root->TryGetObjectField(TEXT("settings"), Part))
	{
		Settings = FUghSettings::FromJson(**Part);
	}
	if (Root->TryGetObjectField(TEXT("highScores"), Part))
	{
		Scores = FUghHighScores::FromJson(**Part);
	}
	return true;
}

bool FUghProfile::Save(const FString& Path, FString& OutError) const
{
	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetNumberField(TEXT("version"), Version);
	Root->SetObjectField(TEXT("settings"), Settings.ToJson());
	Root->SetObjectField(TEXT("highScores"), Scores.ToJson());
	FString Text;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Text);
	if (!FJsonSerializer::Serialize(Root, Writer))
	{
		OutError = TEXT("cannot write the JSON");
		return false;
	}
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true);
	if (!FFileHelper::SaveStringToFile(Text, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		OutError = FString::Printf(TEXT("cannot write %s"), *Path);
		return false;
	}
	return true;
}
