#include "UghUpscaler.h"

#include "DLSSLibrary.h"
#include "HAL/IConsoleManager.h"
#include "StreamlineLibraryDLSSG.h"

namespace
{
	const EStreamlineDLSSGMode FrameGenerationModes[] = {
		EStreamlineDLSSGMode::Off, EStreamlineDLSSGMode::On2X, EStreamlineDLSSGMode::On3X, EStreamlineDLSSGMode::On4X };
	const TCHAR* FrameGenerationNames[] = { TEXT("off"), TEXT("2x"), TEXT("3x"), TEXT("4x") };
	static_assert(UE_ARRAY_COUNT(FrameGenerationModes) == UE_ARRAY_COUNT(FrameGenerationNames));

	void SetCVar(const TCHAR* Name, const TCHAR* Value)
	{
		if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			Variable->Set(Value, ECVF_SetByCode);
		}
	}
}

void FUghUpscaler::ChooseBest()
{
	Use(UDLSSLibrary::IsDLSSSupported() ? EKind::Dlss : EKind::Fsr);
}

void FUghUpscaler::Next()
{
	switch (Kind)
	{
	case EKind::Dlss: Use(EKind::Fsr); break;
	case EKind::Fsr: Use(EKind::Tsr); break;
	case EKind::Tsr: Use(UDLSSLibrary::IsDLSSSupported() ? EKind::Dlss : EKind::Fsr); break;
	}
}

void FUghUpscaler::Use(EKind NewKind)
{
	Kind = NewKind;
	SetCVar(TEXT("r.NGX.DLSS.Enable"), Kind == EKind::Dlss ? TEXT("1") : TEXT("0"));
	SetCVar(TEXT("r.FidelityFX.FSR.Enabled"), Kind == EKind::Fsr ? TEXT("1") : TEXT("0"));
	SetCVar(TEXT("r.ScreenPercentage"), TEXT("67"));
}

void FUghUpscaler::NextFrameGeneration()
{
	if (!UStreamlineLibraryDLSSG::IsDLSSGSupported())
	{
		return;
	}
	for (int32 Step = 1; Step <= UE_ARRAY_COUNT(FrameGenerationModes); ++Step)
	{
		const int32 Mode = (FrameGeneration + Step) % UE_ARRAY_COUNT(FrameGenerationModes);
		if (FrameGenerationModes[Mode] == EStreamlineDLSSGMode::Off ||
			UStreamlineLibraryDLSSG::IsDLSSGModeSupported(FrameGenerationModes[Mode]))
		{
			FrameGeneration = Mode;
			UStreamlineLibraryDLSSG::SetDLSSGMode(FrameGenerationModes[Mode]);
			return;
		}
	}
}

FString FUghUpscaler::Describe() const
{
	const TCHAR* Name = Kind == EKind::Dlss ? TEXT("DLSS") : Kind == EKind::Fsr ? TEXT("FSR") : TEXT("TSR");
	return FString::Printf(TEXT("%s, frame generation %s"), Name,
		UStreamlineLibraryDLSSG::IsDLSSGSupported() ? FrameGenerationNames[FrameGeneration] : TEXT("not supported"));
}
