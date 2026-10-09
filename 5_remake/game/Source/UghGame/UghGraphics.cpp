#include "UghGraphics.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"
#include "GroomComponent.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/KismetSystemLibrary.h"
#include "RHIGlobals.h"
#include "Scalability.h"
#include "UghUpscaler.h"
#include "UnrealClient.h"
#include "UObject/UObjectIterator.h"
#include "Widgets/SWindow.h"

namespace
{
	/** The grooms' level of detail at each preset: hair cards (3), helmets (4). */
	constexpr int32 GroomLODs[FUghSettings::QualityLevels] = { 4, 4, 3, 3 };
	int32 AppliedGroomLOD = GroomLODs[FUghSettings::QualityLevels - 1];

	/** The resolutions offered where the screen tells none (no window). */
	const FIntPoint CommonResolutions[] = { { 1280, 720 }, { 1600, 900 }, { 1920, 1080 }, { 2560, 1440 }, { 3840, 2160 } };
}

TConstArrayView<UghGraphics::FVariable> UghGraphics::Variables()
{
	static const FVariable Table[] = {
		// the shafts of sunlight into the cave: none, a coarse grid, the grid of an eighth of the cells (as tuned)
		{ TEXT("r.VolumetricFog"), { TEXT("0"), TEXT("1"), TEXT("1"), TEXT("1") } },
		{ TEXT("r.VolumetricFog.GridPixelSize"), { TEXT("16"), TEXT("24"), TEXT("16"), TEXT("16") } },
		{ TEXT("r.VolumetricFog.GridSizeZ"), { TEXT("64"), TEXT("48"), TEXT("64"), TEXT("64") } },
		// the sea: at low the engine's screen-space reflections on it alone (the level mirrored where the screen shows
		// it, as medium has them; the scene's own off, ugh.SceneReflections - without Lumen the engine's other
		// reflections had only the sky light to mirror: a white sea), else as the scene's (Lumen's at half resolution,
		// they look the same; the open sea mirrors its sky by itself, UghWater.hlsl); its refraction at half resolution
		// below high
		{ TEXT("r.Water.SingleLayer.Reflection"), { TEXT("3"), TEXT("1"), TEXT("1"), TEXT("1") } },
		{ TEXT("r.SSR.Quality"), { TEXT("1"), TEXT("2"), TEXT("2"), TEXT("3") } },
		{ TEXT("ugh.SceneReflections"), { TEXT("0"), TEXT("1"), TEXT("1"), TEXT("1") } },
		{ TEXT("r.Water.SingleLayer.Reflection.DownsampleFactor"), { TEXT("2"), TEXT("2"), TEXT("2"), TEXT("2") } },
		{ TEXT("r.Water.SingleLayer.RefractionDownsampleFactor"), { TEXT("2"), TEXT("2"), TEXT("1"), TEXT("1") } },
		// Lumen's reflections at half resolution; the sun's shadow maps coarser the lower (the scalability Epic has
		// -1.5); the lighting volume of the translucency (rain, flames) in fewer cells
		{ TEXT("r.Lumen.Reflections.DownsampleFactor"), { TEXT("2"), TEXT("2"), TEXT("2"), TEXT("2") } },
		{ TEXT("r.Shadow.Virtual.ResolutionLodBiasDirectional"), { TEXT("2"), TEXT("1.5"), TEXT("1"), TEXT("1") } },
		{ TEXT("r.TranslucencyLightingVolume.Dim"), { TEXT("16"), TEXT("24"), TEXT("32"), TEXT("32") } },
		// the resolution the upscaler (DLSS, TSR) works from, percent of the screen's; FSR sets it by code from its
		// own mode (performance 50 %, balanced 59 %, quality 67 %), so without it low drew at 67 % too
		{ TEXT("r.ScreenPercentage"), { TEXT("50"), TEXT("58"), TEXT("67"), TEXT("67") } },
		{ TEXT("r.FidelityFX.FSR.QualityMode"), { TEXT("3"), TEXT("2"), TEXT("1"), TEXT("1") } },
		// low and medium as sharp as their GPU keeps the frame within its time (60 fps, 45 fps), down to a third and to
		// 42 % of the screen: a warm notebook draws coarser, not slower; high and epic at their fixed resolution
		{ TEXT("r.DynamicRes.OperationMode"), { TEXT("2"), TEXT("2"), TEXT("0"), TEXT("0") } },
		{ TEXT("r.DynamicRes.MinScreenPercentage"), { TEXT("33"), TEXT("42"), TEXT("50"), TEXT("50") } },
		{ TEXT("r.DynamicRes.MaxScreenPercentage"), { TEXT("50"), TEXT("59"), TEXT("67"), TEXT("67") } },
		{ TEXT("r.DynamicRes.FrameTimeBudget"), { TEXT("16.6"), TEXT("22.2"), TEXT("16.6"), TEXT("16.6") } },
		// the campfires' and torches' shadows (point lights: a cube of six shadow maps each - on an integrated GPU low
		// is twice as fast without them, medium 15 % faster), and their lights licking about (moved, they draw them anew
		// every frame)
		{ TEXT("r.AllowPointLightCubemapShadows"), { TEXT("0"), TEXT("0"), TEXT("1"), TEXT("1") } },
		{ TEXT("ugh.FireLights.Lick"), { TEXT("0"), TEXT("0"), TEXT("0"), TEXT("1") } },
		// Nanite's triangles twice as big on the screen at low (fewer to draw at its half resolution)
		{ TEXT("r.Nanite.MaxPixelsPerEdge"), { TEXT("2"), TEXT("1"), TEXT("1"), TEXT("1") } },
		// the figures' halo (a post-process of 24 taps a pixel of the screen): none at low
		{ TEXT("ugh.Halo"), { TEXT("0"), TEXT("1"), TEXT("1"), TEXT("1") } },
	};
	return Table;
}

FUghDisplayOptions UghGraphics::Options()
{
	FUghDisplayOptions Options;
	Options.bDlss = FUghUpscaler::HasDlss();
	Options.FrameGenerations = FUghUpscaler::FrameGenerations();
	TArray<FIntPoint> Resolutions;
	if (!UKismetSystemLibrary::GetSupportedFullscreenResolutions(Resolutions) || Resolutions.IsEmpty())
	{
		Resolutions.Reset();
		Resolutions.Append(CommonResolutions, UE_ARRAY_COUNT(CommonResolutions));
	}
	for (const FIntPoint& Resolution : Resolutions)
	{
		Options.Resolutions.AddUnique(Resolution);
	}
	Options.Resolutions.Sort([](const FIntPoint& A, const FIntPoint& B)
	{
		return A.X * A.Y < B.X * B.Y || (A.X * A.Y == B.X * B.Y && A.X < B.X);
	});
	if (const UGameUserSettings* User = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		Options.Current = User->GetScreenResolution();
		Options.CurrentWindowMode = User->GetFullscreenMode();
	}
	// the window as it is (the command line's, r.SetRes)
	const UGameViewportClient* Viewport = GEngine ? GEngine->GameViewport.Get() : nullptr;
	if (Viewport && Viewport->Viewport && Viewport->Viewport->GetSizeXY().X > 0)
	{
		Options.Current = Viewport->Viewport->GetSizeXY();
	}
	if (Viewport && Viewport->GetWindow())
	{
		Options.CurrentWindowMode = Viewport->GetWindow()->GetWindowMode();
	}
	return Options;
}

int32 UghGraphics::RecommendedQuality()
{
	const bool bDlss = FUghUpscaler::HasDlss();
	const int32 Quality = FUghSettings::RecommendedQuality(bDlss, GRHIDeviceIsIntegrated);
	UE_LOG(LogTemp, Display, TEXT("UGH recommended quality %s for %s (%s, %s)"), FUghSettings::QualityName(Quality),
		*GRHIAdapterName, GRHIDeviceIsIntegrated ? TEXT("integrated") : TEXT("discrete"),
		bDlss ? TEXT("DLSS") : TEXT("no DLSS"));
	return Quality;
}

void UghGraphics::ApplyQuality(int32 Quality, UWorld* World)
{
	const int32 Level = FMath::Clamp(Quality, 0, FUghSettings::QualityLevels - 1);
	Scalability::FQualityLevels Levels = Scalability::GetQualityLevels();
	Levels.SetFromSingleQualityLevel(Level);
	Levels.ShadowQuality = FMath::Max(Level, 1);
	Levels.GlobalIlluminationQuality = Level;   // low without Lumen's global illumination: the sky light only
	Scalability::SetQualityLevels(Levels);
	// the engine's user settings too: the engine puts their scalability back whenever it applies them (Alt+Enter
	// toggling the full screen did, so a game at low went on at epic)
	if (UGameUserSettings* User = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		User->SetOverallScalabilityLevel(Level);
		User->SetShadowQuality(Levels.ShadowQuality);
		User->SetGlobalIlluminationQuality(Levels.GlobalIlluminationQuality);
	}
	for (const FVariable& Variable : Variables())
	{
		if (IConsoleVariable* Found = IConsoleManager::Get().FindConsoleVariable(Variable.Name))
		{
			Found->Set(Variable.Values[Level], ECVF_SetByGameSetting);
		}
	}
	AppliedGroomLOD = GroomLODs[Level];
	for (TObjectIterator<UGroomComponent> Groom; Groom; ++Groom)
	{
		if (Groom->GetWorld() == World)
		{
			Groom->SetForcedLOD(AppliedGroomLOD);
		}
	}
	UE_LOG(LogTemp, Display, TEXT("UGH quality %s"), FUghSettings::QualityName(Level));
}

int32 UghGraphics::GroomLOD()
{
	return AppliedGroomLOD;
}

void UghGraphics::ApplyDisplay(const FUghSettings& Settings)
{
	UGameUserSettings* User = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!User || (Settings.Resolution == FIntPoint::ZeroValue && Settings.WindowMode < 0))
	{
		return;
	}
	if (Settings.Resolution != FIntPoint::ZeroValue)
	{
		User->SetScreenResolution(Settings.Resolution);
	}
	if (Settings.WindowMode >= 0)
	{
		User->SetFullscreenMode(EWindowMode::ConvertIntToWindowMode(Settings.WindowMode));
	}
	User->ApplyResolutionSettings(false);
}
