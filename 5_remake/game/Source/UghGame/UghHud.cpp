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
	/** Behind the status line and the keys: a dark band, readable where the rock reaches the top of the screen. */
	constexpr float StatusBandHeight = 76;
	const FLinearColor StatusBand(0.f, 0.f, 0.f, 0.55f);
	/** The shadow of the centred texts, pixels down and right. */
	const FLinearColor TextShadow(0.f, 0.f, 0.f, 0.7f);
	constexpr float ShadowOffset = 2;

	/** The menu: where its rows start and how far apart they are (a part of the height). */
	constexpr float MenuTop = 0.36f, MenuRowStep = 0.07f;
	const FLinearColor Chosen = FLinearColor::Yellow, NotChosen = FLinearColor::White;
}

void AUghHud::DrawHUD()
{
	Super::DrawHUD();
	const AUghGameMode* Mode = GetWorld()->GetAuthGameMode<AUghGameMode>();
	if (!Mode || !Canvas)
	{
		return;
	}
	if (!Mode->GetProblem().IsEmpty())
	{
		DrawCentred(TEXT("No game: ") + Mode->GetProblem(), Canvas->ClipY / 2, FLinearColor::Red, TextScale);
		return;
	}
	if (Mode->IsInMenu())
	{
		DrawMenu(*Mode);
		return;
	}
	const ugh_logic_view& View = Mode->GetSimulation().GetCurrent();
	DrawRect(StatusBand, 0, 0, Canvas->ClipX, StatusBandHeight);
	DrawText(FString::Printf(TEXT("Level %d   Lives %d   Score %u   x%d   Energy %d %%"), View.level + 1, View.lives,
		View.score, View.multiplier, FMath::RoundToInt(100.0 * View.energy / FullEnergy)),
		FLinearColor::White, 20, 16, GEngine->GetLargeFont(), TextScale);
	DrawText(FString::Printf(TEXT("%s, %s"), FUghKeyboard::Help(), AUghGameMode::KeysHelp()), FLinearColor::Gray, 20,
		48, GEngine->GetSmallFont(), TextScale);
	// the frontend's settings on the right of the status line
	const FString Settings = FString::Printf(TEXT("%s  |  volume %d %%"), *Mode->GetUpscaler().Describe(),
		Mode->GetVolumePercent());
	float Width = 0, Height = 0;
	GetTextSize(Settings, Width, Height, GEngine->GetSmallFont(), TextScale);
	DrawText(Settings, FLinearColor::Gray, Canvas->ClipX - Width - 20, 20, GEngine->GetSmallFont(), TextScale);
	if (View.phase == UGH_LOGIC_PHASE_CAPTION)
	{
		DrawCentred(FString::Printf(TEXT("Level %d"), View.level + 1), Canvas->ClipY * 0.4f, FLinearColor::Yellow,
			2 * TextScale);
		DrawCentred(TEXT("Press a key"), Canvas->ClipY * 0.55f, FLinearColor::White, TextScale);
	}
}

/** The title, how the last game ended, the rows (the chosen one yellow), the keys. */
void AUghHud::DrawMenu(const AUghGameMode& Mode)
{
	const FUghMenu& Menu = Mode.GetMenu();
	const FUghGameChoice Choice = Menu.GetChoice();
	const float Height = Canvas->ClipY;
	DrawCentred(TEXT("UGH!"), Height * 0.14f, FLinearColor::Yellow, 3 * TextScale);
	if (!Mode.GetLastGame().IsEmpty())
	{
		DrawCentred(Mode.GetLastGame(), Height * 0.26f, FLinearColor::White, TextScale);
	}
	const int32 Levels = Mode.GetPasswords().LevelCount(Choice.Players);
	const FString Level = Menu.GetPassword().IsEmpty() ? TEXT("")
		: Menu.IsPasswordKnown() ? FString::Printf(TEXT("  - level %d"), Choice.FirstLevel + 1)
		: FString(TEXT("  - unknown"));
	const FString Rows[FUghMenu::RowCount] = {
		FString::Printf(TEXT("%s (%d levels)"), Choice.Players == 2 ? TEXT("Team: two copters") : TEXT("One player"), Levels),
		FString::Printf(TEXT("Difficulty: %s"), FUghMenu::DifficultyName(Choice.Difficulty)),
		FString::Printf(TEXT("Password: %s_%s"), *Menu.GetPassword(), *Level) };
	for (int32 Row = 0; Row < FUghMenu::RowCount; ++Row)
	{
		const bool bChosen = static_cast<int32>(Menu.GetRow()) == Row;
		DrawCentred(bChosen ? TEXT("> ") + Rows[Row] + TEXT(" <") : Rows[Row], Height * (MenuTop + Row * MenuRowStep),
			bChosen ? Chosen : NotChosen, TextScale);
	}
	DrawCentred(FUghMenu::KeysHelp(), Height * (MenuTop + (FUghMenu::RowCount + 1) * MenuRowStep), FLinearColor::Gray,
		TextScale);
	DrawCentred(FUghKeyboard::Help(), Height * (MenuTop + (FUghMenu::RowCount + 2) * MenuRowStep), FLinearColor::Gray,
		TextScale);
	DrawCentred(FString::Printf(TEXT("PgUp/PgDn: volume %d %%"), Mode.GetVolumePercent()),
		Height * (MenuTop + (FUghMenu::RowCount + 3) * MenuRowStep), FLinearColor::Gray, TextScale);
}

void AUghHud::DrawCentred(const FString& Text, float Y, const FLinearColor& Color, float Scale)
{
	UFont* Font = GEngine->GetLargeFont();
	float Width = 0, Height = 0;
	GetTextSize(Text, Width, Height, Font, Scale);
	// a shadow: readable over the scene too (a level's flight, the menu)
	DrawText(Text, TextShadow, (Canvas->ClipX - Width) / 2 + ShadowOffset, Y + ShadowOffset, Font, Scale);
	DrawText(Text, Color, (Canvas->ClipX - Width) / 2, Y, Font, Scale);
}
