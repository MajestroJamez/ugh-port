#include "UghHud.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "UghGameMode.h"
#include "UghKeyboard.h"

namespace
{
	constexpr float TextScale = 1.5f;
	constexpr int32 FullEnergy = UGH_LOGIC_FULL_ENERGY;
}

void AUghHud::DrawHUD()
{
	Super::DrawHUD();
	const AUghGameMode* Mode = GetWorld()->GetAuthGameMode<AUghGameMode>();
	if (!Mode || !Canvas)
	{
		return;
	}
	UFont* Font = GEngine->GetLargeFont();
	if (!Mode->GetProblem().IsEmpty())
	{
		DrawCentred(TEXT("No game: ") + Mode->GetProblem(), Canvas->ClipY / 2, FLinearColor::Red, TextScale);
		return;
	}
	const FUghSimulation& Simulation = Mode->GetSimulation();
	const ugh_logic_view& View = Simulation.GetCurrent();
	DrawText(FString::Printf(TEXT("Level %d   Lives %d   Score %u   x%d   Energy %d %%"), View.level + 1, View.lives,
		View.score, View.multiplier, FMath::RoundToInt(100.0 * View.energy / FullEnergy)),
		FLinearColor::White, 20, 16, Font, TextScale);
	DrawText(FString::Printf(TEXT("%s, %s  |  %s"), FUghKeyboard::Help(), AUghGameMode::KeysHelp(),
		*Mode->GetUpscaler().Describe()), FLinearColor::Gray, 20, 48, GEngine->GetSmallFont(), TextScale);

	if (Simulation.IsOver())
	{
		DrawCentred(Simulation.GetResult() == UGH_LOGIC_ALL_LEVELS_DONE ? TEXT("All levels done!") : TEXT("Game over"),
			Canvas->ClipY * 0.4f, FLinearColor::Yellow, 2 * TextScale);
		DrawCentred(AUghGameMode::GameOverKeysHelp(), Canvas->ClipY * 0.55f, FLinearColor::White, TextScale);
	}
	else if (View.phase == UGH_LOGIC_PHASE_CAPTION)
	{
		DrawCentred(FString::Printf(TEXT("Level %d"), View.level + 1), Canvas->ClipY * 0.4f, FLinearColor::Yellow,
			2 * TextScale);
		DrawCentred(TEXT("Press a key"), Canvas->ClipY * 0.55f, FLinearColor::White, TextScale);
	}
}

void AUghHud::DrawCentred(const FString& Text, float Y, const FLinearColor& Color, float Scale)
{
	UFont* Font = GEngine->GetLargeFont();
	float Width = 0, Height = 0;
	GetTextSize(Text, Width, Height, Font, Scale);
	DrawText(Text, Color, (Canvas->ClipX - Width) / 2, Y, Font, Scale);
}
