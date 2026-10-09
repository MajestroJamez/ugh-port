#include "UghSettingsMenu.h"

#include "UghControls.h"

using ERow = FUghSettingsMenu::ERow;

namespace
{
	const TCHAR* const Labels[FUghSettingsMenu::RowCount] = { TEXT("Quality"), TEXT("Upscaler"),
		TEXT("Frame generation"), TEXT("Resolution"), TEXT("Window"), TEXT("Volume"), TEXT("Music"), TEXT("Effects"),
		TEXT("Ambience"), TEXT("Flight to the stone"), TEXT("Camera shake"), TEXT("Ghost of the best"), TEXT("Controls"),
		TEXT("Back") };

	const TCHAR* const QualityHints[FUghSettings::QualityLevels] = {
		TEXT("The fastest (a third to half resolution, kept at 60 fps): the sky's light, the sun's shadows, plain hair"),
		TEXT("Lumen's simpler light, softer shadows, firelight without shadows, coarse volumetric fog, plain hair"),
		TEXT("Lumen's light, sharp shadows, steady firelight, volumetric fog, hair cards, the sea's reflections at half"),
		TEXT("Everything at its best: the diorama as it was made, the firelight's shadows dancing"),
	};

	/** The value of `Values` (sorted by `Less`, not empty) next to `Current` that way (`Direction` 1 or -1), round. */
	template <typename T, typename TLess>
	T Cycle(const TArray<T>& Values, const T& Current, int32 Direction, TLess Less)
	{
		auto Same = [&](const T& Value) { return !Less(Value, Current) && !Less(Current, Value); };
		const int32 Exact = Values.IndexOfByPredicate(Same);
		if (Exact != INDEX_NONE)
		{
			return Values[(Exact + Direction + Values.Num()) % Values.Num()];
		}
		// between two of them: the first one above it, or the last one below it
		const int32 Above = Values.IndexOfByPredicate([&](const T& Value) { return Less(Current, Value); });
		if (Direction > 0)
		{
			return Values[Above == INDEX_NONE ? 0 : Above];
		}
		const int32 Below = (Above == INDEX_NONE ? Values.Num() : Above) - 1;
		return Values[Below < 0 ? Values.Num() - 1 : Below];
	}

	bool ResolutionLess(const FIntPoint& A, const FIntPoint& B)
	{
		return A.X * A.Y < B.X * B.Y || (A.X * A.Y == B.X * B.Y && A.X < B.X);
	}

	int32 StepVolume(int32 Volume, int32 Direction)
	{
		return FMath::Clamp(Volume + Direction * FUghSettings::VolumeStep, 0, 100);
	}
}

EUghUpscaler FUghDisplayOptions::UpscalerOf(const FUghSettings& Settings) const
{
	return Settings.Upscaler == EUghUpscaler::Dlss && !bDlss ? EUghUpscaler::Fsr : Settings.Upscaler;
}

FIntPoint FUghDisplayOptions::ResolutionOf(const FUghSettings& Settings) const
{
	return Settings.Resolution == FIntPoint::ZeroValue ? Current : Settings.Resolution;
}

int32 FUghDisplayOptions::WindowModeOf(const FUghSettings& Settings) const
{
	return Settings.WindowMode < 0 ? CurrentWindowMode : Settings.WindowMode;
}

FUghSettingsMenu::EResult FUghSettingsMenu::HandleKey(const FKey& Key, FUghSettings& Settings,
	const FUghDisplayOptions& Options)
{
	const bool bEnter = Key == EKeys::Enter;
	if (Key == EKeys::Escape || Key == FUghControls::BackKey() || (bEnter && Row == ERow::Back))
	{
		return EResult::Back;
	}
	if (bEnter && Row == ERow::Controls)
	{
		return EResult::Controls;
	}
	if (Key == EKeys::Up || Key == EKeys::Down)
	{
		const int32 Step = Key == EKeys::Down ? 1 : RowCount - 1;
		Row = static_cast<ERow>((static_cast<int32>(Row) + Step) % RowCount);
		return EResult::None;
	}
	if (Key == EKeys::Left || Key == EKeys::Right || bEnter)
	{
		return Change(Key == EKeys::Left ? -1 : 1, Settings, Options) ? EResult::Changed : EResult::None;
	}
	return EResult::None;
}

bool FUghSettingsMenu::Change(int32 Direction, FUghSettings& Settings, const FUghDisplayOptions& Options) const
{
	const FUghSettings Before = Settings;
	switch (Row)
	{
	case ERow::Quality:
		Settings.Quality = FMath::Clamp(Settings.Quality + Direction, 0, FUghSettings::QualityLevels - 1);
		break;
	case ERow::Upscaler:
	{
		TArray<int32> Kinds = { int32(EUghUpscaler::Fsr), int32(EUghUpscaler::Tsr) };
		if (Options.bDlss)
		{
			Kinds.Insert(int32(EUghUpscaler::Dlss), 0);
		}
		Settings.Upscaler = EUghUpscaler(Cycle(Kinds, int32(Options.UpscalerOf(Settings)), Direction, TLess<int32>()));
		break;
	}
	case ERow::FrameGeneration:
		Settings.FrameGeneration = Cycle(Options.FrameGenerations, Settings.FrameGeneration, Direction, TLess<int32>());
		break;
	case ERow::Resolution:
		if (!Options.Resolutions.IsEmpty())
		{
			Settings.Resolution = Cycle(Options.Resolutions, Options.ResolutionOf(Settings), Direction, ResolutionLess);
		}
		break;
	case ERow::Window:
		Settings.WindowMode = (Options.WindowModeOf(Settings) + Direction + 3) % 3;
		break;
	case ERow::Volume: Settings.Volume = StepVolume(Settings.Volume, Direction); break;
	case ERow::Music: Settings.Music = StepVolume(Settings.Music, Direction); break;
	case ERow::Effects: Settings.Effects = StepVolume(Settings.Effects, Direction); break;
	case ERow::Ambience: Settings.Ambience = StepVolume(Settings.Ambience, Direction); break;
	case ERow::Intro: Settings.bIntro = !Settings.bIntro; break;
	case ERow::Shake: Settings.bShake = !Settings.bShake; break;
	case ERow::Ghost: Settings.bGhost = !Settings.bGhost; break;
	default: break;
	}
	return !(Settings == Before);
}

const TCHAR* FUghSettingsMenu::LabelOf(ERow Row)
{
	return Labels[static_cast<int32>(Row)];
}

FString FUghSettingsMenu::ValueOf(ERow Row, const FUghSettings& Settings, const FUghDisplayOptions& Options)
{
	switch (Row)
	{
	case ERow::Quality: return FUghSettings::QualityName(Settings.Quality);
	case ERow::Upscaler: return FUghSettings::UpscalerName(Options.UpscalerOf(Settings));
	case ERow::FrameGeneration:
		return Options.FrameGenerations.Num() > 1 ? FString(FUghSettings::FrameGenerationName(Settings.FrameGeneration))
			: FString(TEXT("Not supported"));
	case ERow::Resolution:
	{
		const FIntPoint Resolution = Options.ResolutionOf(Settings);
		return FString::Printf(TEXT("%d × %d"), Resolution.X, Resolution.Y);
	}
	case ERow::Window: return FUghSettings::WindowModeName(Options.WindowModeOf(Settings));
	case ERow::Volume: return FString::Printf(TEXT("%d %%"), Settings.Volume);
	case ERow::Music: return FString::Printf(TEXT("%d %%"), Settings.Music);
	case ERow::Effects: return FString::Printf(TEXT("%d %%"), Settings.Effects);
	case ERow::Ambience: return FString::Printf(TEXT("%d %%"), Settings.Ambience);
	case ERow::Intro: return Settings.bIntro ? TEXT("On") : TEXT("Off");
	case ERow::Shake: return Settings.bShake ? TEXT("On") : TEXT("Off");
	case ERow::Ghost: return Settings.bGhost ? TEXT("On") : TEXT("Off");
	default: return FString();
	}
}

float FUghSettingsMenu::LevelOf(ERow Row, const FUghSettings& Settings)
{
	switch (Row)
	{
	case ERow::Volume: return Settings.Volume / 100.f;
	case ERow::Music: return Settings.Music / 100.f;
	case ERow::Effects: return Settings.Effects / 100.f;
	case ERow::Ambience: return Settings.Ambience / 100.f;
	default: return -1;
	}
}

FString FUghSettingsMenu::HintOf(ERow Row, const FUghSettings& Settings, const FUghDisplayOptions& Options)
{
	switch (Row)
	{
	case ERow::Quality: return QualityHints[FMath::Clamp(Settings.Quality, 0, FUghSettings::QualityLevels - 1)];
	case ERow::Upscaler:
		return Options.bDlss ? TEXT("Drawn smaller (by the quality), upscaled: NVIDIA's DLSS, AMD's FSR or the engine's TSR")
			: TEXT("Drawn smaller (by the quality) and upscaled: AMD's FSR or the engine's TSR (DLSS needs an RTX)");
	case ERow::FrameGeneration:
		return Options.FrameGenerations.Num() > 1 ? TEXT("DLSS makes frames between the drawn ones (with DLSS)")
			: TEXT("DLSS frame generation needs an NVIDIA RTX 40 or 50");
	case ERow::Resolution: return TEXT("The resolution of the screen (fullscreen) or of the window");
	case ERow::Window: return TEXT("Fullscreen, a borderless window over the screen, or a window");
	case ERow::Volume: return TEXT("Everything heard (Page Up and Page Down in the game too)");
	case ERow::Music: return TEXT("The original's music in the menu and the levels");
	case ERow::Effects: return TEXT("The original's sounds of the game");
	case ERow::Ambience: return TEXT("The sea, the jungle, the wind and the rain, the fires, the splashes, the lianas");
	case ERow::Intro: return TEXT("At each level's start the camera flies over the sea to the stone it is carved into");
	case ERow::Shake:
		return TEXT("A crash, a stone on an enemy, a fall into the sea, a bump into the edge shake the camera a little");
	case ERow::Ghost: return TEXT("A see-through copter flies your best run of the level beside you (its replay)");
	case ERow::Controls: return TEXT("The pilots' keys and the gamepads");
	default: return TEXT("Back to the title");
	}
}
