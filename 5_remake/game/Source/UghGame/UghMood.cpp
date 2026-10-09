#include "UghMood.h"

#include "UghAssets.h"

namespace
{
	using EKind = FUghMood::EKind;

	/**
	 * The moods. The sun always comes from the front left (its light into the cave, the shadows of the ledges on the
	 * back wall), lower and warmer towards the evening; the night's moon is cool and dim, the exposure brighter, so
	 * that the campfires light the cave. A storm: a dim cool light under an overcast sky, a dense grey fog, a mist over
	 * the water.
	 */
	const FUghMood Moods[] = {
		{ EKind::Day, TEXT("day"), -55, -60, FLinearColor(1.f, 0.95f, 0.88f), 10.f, UghAssets::SkyDay,
			FLinearColor::White, 1.4f, 0.008f, FLinearColor(0.55f, 0.62f, 0.72f), 1.f, 0.f, 2.1f, 1.6f, 1.4f, 1.f,
			1.2f, 0.6f, FLinearColor(0.85f, 0.9f, 1.f) },
		{ EKind::Evening, TEXT("evening"), -32, -72, FLinearColor(1.f, 0.78f, 0.55f), 8.f, UghAssets::SkyEvening,
			FLinearColor::White, 1.3f, 0.012f, FLinearColor(0.6f, 0.5f, 0.42f), 1.2f, 0.f, 1.6f, 1.2f, 1.3f, 1.05f,
			1.2f, 0.6f, FLinearColor(0.9f, 0.8f, 0.68f) },
		{ EKind::Dusk, TEXT("dusk"), -12, -78, FLinearColor(1.f, 0.6f, 0.38f), 3.5f, UghAssets::SkyDusk,
			FLinearColor::White, 0.5f, 0.02f, FLinearColor(0.45f, 0.3f, 0.3f), 1.4f, 0.f, 1.4f, 0.6f, 0.5f, 1.15f,
			1.2f, 0.55f, FLinearColor(0.65f, 0.52f, 0.5f) },
		{ EKind::Night, TEXT("night"), -40, -55, FLinearColor(0.55f, 0.66f, 1.f), 0.6f, UghAssets::SkyNight,
			FLinearColor::White, 0.3f, 0.012f, FLinearColor(0.08f, 0.1f, 0.15f), 1.f, 0.f, 0.6f, 0.15f, 0.05f, 1.3f,
			0.9f, 0.5f, FLinearColor(0.22f, 0.27f, 0.4f) },
		{ EKind::Storm, TEXT("storm"), -45, -65, FLinearColor(0.7f, 0.8f, 1.f), 3.5f, UghAssets::SkyStorm,
			FLinearColor(0.45f, 0.48f, 0.52f), 1.6f, 0.04f, FLinearColor(0.3f, 0.33f, 0.38f), 0.6f, 0.08f, 1.15f, 0.3f,
			1.6f, 1.1f, 1.3f, 0.6f, FLinearColor(0.65f, 0.7f, 0.78f) },
	};
	/** The calm levels' moods through one day (UghMood::LevelsADay). */
	constexpr EKind Day[] = { EKind::Day, EKind::Day, EKind::Evening, EKind::Evening, EKind::Dusk, EKind::Night };
	static_assert(UE_ARRAY_COUNT(Day) == UghMood::LevelsADay);
}

const FUghMood& UghMood::Of(int32 Level, int32 Wind)
{
	const EKind Kind = Wind != 0 ? EKind::Storm : Day[((Level % LevelsADay) + LevelsADay) % LevelsADay];
	check(Moods[static_cast<int32>(Kind)].Kind == Kind);
	return Moods[static_cast<int32>(Kind)];
}
