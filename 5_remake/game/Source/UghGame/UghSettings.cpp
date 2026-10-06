#include "UghSettings.h"

#include "Dom/JsonObject.h"
#include "GenericPlatform/GenericWindow.h"

namespace
{
	const TCHAR* const QualityNames[FUghSettings::QualityLevels] = {
		TEXT("Low"), TEXT("Medium"), TEXT("High"), TEXT("Epic") };
	const TCHAR* const UpscalerNames[] = { TEXT("DLSS"), TEXT("FSR"), TEXT("TSR") };
	const TCHAR* const FrameGenerationNames[FUghSettings::FrameGenerations] = {
		TEXT("Off"), TEXT("2x"), TEXT("3x"), TEXT("4x") };
	const TCHAR* const WindowModeNames[] = { TEXT("Fullscreen"), TEXT("Borderless"), TEXT("Window") };
	static_assert(EWindowMode::Fullscreen == 0 && EWindowMode::WindowedFullscreen == 1 && EWindowMode::Windowed == 2);

	/** The integer field `Name` within `Min` .. `Max`, else `Value` as it is. */
	void ReadInt(const FJsonObject& Json, const TCHAR* Name, int32 Min, int32 Max, int32& Value)
	{
		int32 Read = 0;
		if (Json.TryGetNumberField(Name, Read) && Read >= Min && Read <= Max)
		{
			Value = Read;
		}
	}

	/** A volume in steps of a tenth. */
	void ReadVolume(const FJsonObject& Json, const TCHAR* Name, int32& Value)
	{
		ReadInt(Json, Name, 0, 100, Value);
		Value = Value / FUghSettings::VolumeStep * FUghSettings::VolumeStep;
	}
}

const TCHAR* FUghSettings::QualityName(int32 Quality)
{
	return QualityNames[FMath::Clamp(Quality, 0, QualityLevels - 1)];
}

const TCHAR* FUghSettings::UpscalerName(EUghUpscaler Upscaler)
{
	return UpscalerNames[static_cast<int32>(Upscaler)];
}

const TCHAR* FUghSettings::FrameGenerationName(int32 FrameGeneration)
{
	return FrameGenerationNames[FMath::Clamp(FrameGeneration, 0, FrameGenerations - 1)];
}

const TCHAR* FUghSettings::WindowModeName(int32 WindowMode)
{
	return WindowModeNames[FMath::Clamp(WindowMode, 0, int32(UE_ARRAY_COUNT(WindowModeNames)) - 1)];
}

TSharedRef<FJsonObject> FUghSettings::ToJson() const
{
	TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
	Json->SetStringField(TEXT("quality"), QualityName(Quality));
	Json->SetStringField(TEXT("upscaler"), UpscalerName(Upscaler));
	Json->SetNumberField(TEXT("frameGeneration"), FrameGeneration);
	Json->SetNumberField(TEXT("width"), Resolution.X);
	Json->SetNumberField(TEXT("height"), Resolution.Y);
	Json->SetNumberField(TEXT("windowMode"), WindowMode);
	Json->SetNumberField(TEXT("volume"), Volume);
	Json->SetNumberField(TEXT("music"), Music);
	Json->SetNumberField(TEXT("effects"), Effects);
	Json->SetBoolField(TEXT("intro"), bIntro);
	Json->SetObjectField(TEXT("keys"), Keys.ToJson());
	return Json;
}

FUghSettings FUghSettings::FromJson(const FJsonObject& Json)
{
	FUghSettings Settings;
	FString Name;
	if (Json.TryGetStringField(TEXT("quality"), Name))
	{
		for (int32 Level = 0; Level < QualityLevels; ++Level)
		{
			Settings.Quality = Name == QualityNames[Level] ? Level : Settings.Quality;
		}
	}
	if (Json.TryGetStringField(TEXT("upscaler"), Name))
	{
		for (int32 Kind = 0; Kind < UE_ARRAY_COUNT(UpscalerNames); ++Kind)
		{
			Settings.Upscaler = Name == UpscalerNames[Kind] ? static_cast<EUghUpscaler>(Kind) : Settings.Upscaler;
		}
	}
	ReadInt(Json, TEXT("frameGeneration"), 0, FrameGenerations - 1, Settings.FrameGeneration);
	FIntPoint Resolution = FIntPoint::ZeroValue;
	ReadInt(Json, TEXT("width"), 640, 16384, Resolution.X);
	ReadInt(Json, TEXT("height"), 360, 16384, Resolution.Y);
	Settings.Resolution = Resolution.X > 0 && Resolution.Y > 0 ? Resolution : FIntPoint::ZeroValue;
	ReadInt(Json, TEXT("windowMode"), -1, EWindowMode::Windowed, Settings.WindowMode);
	ReadVolume(Json, TEXT("volume"), Settings.Volume);
	ReadVolume(Json, TEXT("music"), Settings.Music);
	ReadVolume(Json, TEXT("effects"), Settings.Effects);
	Json.TryGetBoolField(TEXT("intro"), Settings.bIntro);
	const TSharedPtr<FJsonObject>* Keys = nullptr;
	if (Json.TryGetObjectField(TEXT("keys"), Keys))
	{
		Settings.Keys = FUghKeyBindings::FromJson(**Keys);
	}
	return Settings;
}
