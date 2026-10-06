#include "UghUpscaler.h"

#include "DLSSLibrary.h"
#include "HAL/IConsoleManager.h"
#include "StreamlineLibraryDLSSG.h"

namespace
{
	const EStreamlineDLSSGMode FrameGenerationModes[] = {
		EStreamlineDLSSGMode::Off, EStreamlineDLSSGMode::On2X, EStreamlineDLSSGMode::On3X, EStreamlineDLSSGMode::On4X };
	static_assert(UE_ARRAY_COUNT(FrameGenerationModes) == FUghSettings::FrameGenerations);

	void SetCVar(const TCHAR* Name, const TCHAR* Value)
	{
		if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			Variable->Set(Value, ECVF_SetByCode);
		}
	}
}

bool FUghUpscaler::HasDlss()
{
	return UDLSSLibrary::IsDLSSSupported();
}

TArray<int32> FUghUpscaler::FrameGenerations()
{
	TArray<int32> Supported = { 0 };
	for (int32 Mode = 1; Mode < UE_ARRAY_COUNT(FrameGenerationModes) && UStreamlineLibraryDLSSG::IsDLSSGSupported(); ++Mode)
	{
		if (UStreamlineLibraryDLSSG::IsDLSSGModeSupported(FrameGenerationModes[Mode]))
		{
			Supported.Add(Mode);
		}
	}
	return Supported;
}

void FUghUpscaler::Use(EUghUpscaler NewKind)
{
	Kind = NewKind == EUghUpscaler::Dlss && !HasDlss() ? EUghUpscaler::Fsr : NewKind;
	SetCVar(TEXT("r.NGX.DLSS.Enable"), Kind == EUghUpscaler::Dlss ? TEXT("1") : TEXT("0"));
	SetCVar(TEXT("r.FidelityFX.FSR.Enabled"), Kind == EUghUpscaler::Fsr ? TEXT("1") : TEXT("0"));
	SetCVar(TEXT("r.ScreenPercentage"), TEXT("67"));
}

void FUghUpscaler::Next()
{
	switch (Kind)
	{
	case EUghUpscaler::Dlss: Use(EUghUpscaler::Fsr); break;
	case EUghUpscaler::Fsr: Use(EUghUpscaler::Tsr); break;
	case EUghUpscaler::Tsr: Use(EUghUpscaler::Dlss); break;
	}
}

void FUghUpscaler::SetFrameGeneration(int32 NewFrameGeneration)
{
	const int32 Wanted = FMath::Clamp(NewFrameGeneration, 0, FUghSettings::FrameGenerations - 1);
	FrameGeneration = FrameGenerations().Contains(Wanted) ? Wanted : 0;
	if (UStreamlineLibraryDLSSG::IsDLSSGSupported())
	{
		UStreamlineLibraryDLSSG::SetDLSSGMode(FrameGenerationModes[FrameGeneration]);
	}
}

void FUghUpscaler::NextFrameGeneration()
{
	const TArray<int32> Supported = FrameGenerations();
	const int32 Index = Supported.Find(FrameGeneration);
	SetFrameGeneration(Supported[(Index + 1) % Supported.Num()]);
}

FString FUghUpscaler::Describe() const
{
	return FString::Printf(TEXT("%s, frame generation %s"), FUghSettings::UpscalerName(Kind),
		UStreamlineLibraryDLSSG::IsDLSSGSupported() ? *FString(FUghSettings::FrameGenerationName(FrameGeneration)).ToLower()
			: TEXT("not supported"));
}
