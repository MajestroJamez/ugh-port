#include "UghBursts.h"

namespace
{
	using namespace UghBursts;

	/** The colours of what bursts: water, foam, dust, smoke, debris, stone, sparks, gold, air. */
	const FLinearColor Water(0.8f, 0.88f, 0.92f), Foam(0.92f, 0.95f, 0.97f), Dirt(0.5f, 0.43f, 0.34f),
		Soot(0.17f, 0.155f, 0.14f), Wood(0.22f, 0.15f, 0.09f), Stone(0.5f, 0.47f, 0.43f), Ember(1.f, 0.55f, 0.18f),
		Gold(1.f, 0.82f, 0.45f), Air(0.75f, 0.72f, 0.66f);
	constexpr float Fall = 980;   // cm/s^2: a unit is about a centimetre (a passenger is 145 units tall)

	// (Water is lit as the dust is - the drops, the spray, the foam, the rings: at night it is dim and takes the fires'
	// light; only fire, sparks and glints are light, Glint.)
	const FPart SplashParts[] = {
		{ .Shape = EShape::Drop, .Count = 80, .Life = 0.9f, .Stagger = 0.1f, .Speed = 560, .Spread = 0.45f,
			.Gravity = Fall, .Drag = 0.4f, .Size = 5, .Stretch = 1.2f, .Box = { 15, 6, 2 }, .Color = Water,
			.Strength = 0.7f },
		{ .Count = 16, .Life = 1.3f, .Stagger = 0.15f, .Speed = 180, .Spread = 0.8f, .Lift = 0.7f, .Gravity = 60,
			.Drag = 2, .Size = 26, .Grow = 3, .Spin = 0.5f, .Box = { 20, 8, 4 }, .Color = Foam, .Strength = 0.5f },
		{ .Shape = EShape::Ring, .Mode = EMode::Flat, .Count = 3, .Life = 1.6f, .Stagger = 0.5f, .Size = 50, .Grow = 4,
			.Color = Foam, .Strength = 0.6f } };
	const FPart SurfParts[] = {
		{ .Shape = EShape::Drop, .Count = 200, .Life = 1.4f, .Stagger = 3.5f, .Speed = 800, .Spread = 0.5f,
			.Gravity = Fall, .Drag = 0.6f, .Size = 7, .Stretch = 1, .Box = { 1400, 40, 5 }, .Color = Water,
			.Strength = 0.6f },
		{ .Count = 60, .Life = 2.5f, .Stagger = 3.5f, .Speed = 260, .Spread = 0.7f, .Lift = 0.8f, .Gravity = 40,
			.Drag = 1.2f, .Size = 80, .Grow = 3.5f, .Spin = 0.3f, .Box = { 1400, 40, 10 }, .Color = Foam,
			.Strength = 0.5f } };
	const FPart ExplosionParts[] = {
		{ .Blend = EBlend::Glint, .Shape = EShape::Fire, .Count = 24, .Life = 0.8f, .Stagger = 0.12f, .Speed = 280,
			.Spread = 2, .Gravity = -250, .Drag = 3, .Size = 36, .Grow = 3, .Spin = 1, .Box = { 30, 10, 20 },
			.Strength = 1.6f },
		{ .Count = 26, .Life = 2.6f, .Stagger = 0.4f, .Speed = 200, .Spread = 1.3f, .Gravity = -110, .Drag = 1.6f,
			.Size = 45, .Grow = 3.5f, .Spin = 0.4f, .Box = { 30, 10, 20 }, .Color = Soot, .Strength = 0.7f },
		{ .Blend = EBlend::Bits, .Shape = EShape::Chunk, .Mode = EMode::Tumbling, .Count = 26, .Life = 1.7f,
			.Speed = 750, .Spread = 1.3f, .Gravity = Fall, .Drag = 0.4f, .Size = 16, .Spin = 14, .Box = { 20, 8, 15 },
			.Color = Wood, .Roughness = 0.85f },
		{ .Blend = EBlend::Glint, .Shape = EShape::Drop, .Count = 50, .Life = 0.9f, .Stagger = 0.1f, .Speed = 850,
			.Spread = 1.6f, .Gravity = 700, .Drag = 1.3f, .Size = 3, .Stretch = 2.5f, .Color = Ember,
			.Strength = 6 } };
	const FPart DustParts[] = {
		{ .Count = 34, .Life = 1.6f, .Stagger = 0.1f, .Speed = 450, .Spread = 1, .Lift = 0.35f, .Gravity = -30,
			.Drag = 3, .Size = 28, .Grow = 3.5f, .Spin = 0.5f, .Box = { 90, 15, 3 }, .Color = Dirt, .Strength = 0.7f },
		{ .Blend = EBlend::Bits, .Shape = EShape::Chunk, .Mode = EMode::Tumbling, .Count = 12, .Life = 0.8f,
			.Speed = 320, .Spread = 0.8f, .Lift = 0.8f, .Gravity = Fall, .Drag = 0.5f, .Size = 5, .Spin = 10,
			.Box = { 80, 10, 2 }, .Color = Stone } };
	const FPart ShellParts[] = {
		{ .Blend = EBlend::Bits, .Shape = EShape::Shell, .Mode = EMode::Tumbling, .Count = 12, .Life = 1.4f,
			.Stagger = 0.15f, .Speed = 560, .Spread = 0.45f, .Gravity = Fall, .Drag = 0.3f, .Size = 15, .Spin = 9,
			.Box = { 10, 5, 5 }, .Roughness = 0.3f },
		{ .Blend = EBlend::Glint, .Shape = EShape::Star, .Count = 10, .Life = 0.6f, .Stagger = 0.3f, .Speed = 150,
			.Spread = 2, .Drag = 2, .Size = 14, .Box = { 20, 10, 15 }, .Color = Gold, .Strength = 3 } };
	const FPart GlintParts[] = {
		{ .Blend = EBlend::Glint, .Shape = EShape::Star, .Count = 18, .Life = 0.9f, .Stagger = 0.15f, .Speed = 260,
			.Spread = 2, .Gravity = -60, .Drag = 2.2f, .Size = 16, .Box = { 15, 8, 15 }, .Color = Gold, .Strength = 4,
			.bTinted = true },
		{ .Blend = EBlend::Glint, .Shape = EShape::Drop, .Count = 30, .Life = 1.3f, .Stagger = 0.3f, .Speed = 120,
			.Spread = 2, .Gravity = -90, .Drag = 1, .Size = 3, .Box = { 25, 10, 25 }, .Color = Gold, .Strength = 5,
			.bTinted = true } };
	const FPart FeatherParts[] = {
		{ .Blend = EBlend::Bits, .Shape = EShape::Feather, .Mode = EMode::Tumbling, .Count = 12, .Life = 2.8f,
			.Stagger = 0.2f, .Speed = 280, .Spread = 2, .Gravity = 180, .Drag = 2.8f, .Size = 24, .Flutter = 25,
			.Spin = 5, .Box = { 25, 10, 10 }, .Roughness = 0.8f },
		{ .Count = 8, .Life = 1, .Speed = 120, .Spread = 2, .Drag = 2, .Size = 12, .Grow = 2, .Box = { 15, 8, 8 },
			.Color = Air, .Strength = 0.35f } };
	const FPart DowndraftParts[] = {
		{ .Count = 10, .Life = 0.9f, .Stagger = 0.9f, .Speed = 320, .Spread = 0.5f, .Direction = { 0, 0, -1 },
			.Drag = 2.5f, .Size = 20, .Grow = 3, .Spin = 0.5f, .Box = { 60, 10, 5 }, .Color = Air, .Strength = 0.25f },
		{ .Blend = EBlend::Bits, .Shape = EShape::Feather, .Mode = EMode::Tumbling, .Count = 4, .Life = 1.8f,
			.Stagger = 1.8f, .Speed = 120, .Spread = 0.6f, .Direction = { 0, 0, -1 }, .Gravity = 150, .Drag = 3,
			.Size = 10, .Flutter = 20, .Spin = 4, .Box = { 50, 10, 5 } } };
	const FPart GustParts[] = {
		{ .Blend = EBlend::Bits, .Shape = EShape::Leaf, .Mode = EMode::Tumbling, .Count = 22, .Life = 1.6f,
			.Stagger = 0.3f, .Speed = 1000, .Spread = 0.25f, .Direction = { -1, 0, 0.15f }, .Gravity = 200,
			.Drag = 1.2f, .Size = 18, .Flutter = 20, .Spin = 9, .Box = { 20, 10, 15 }, .bFacing = true },
		{ .Shape = EShape::Drop, .Count = 26, .Life = 0.8f, .Stagger = 0.4f, .Speed = 1300, .Spread = 0.15f,
			.Direction = { -1, 0, 0.05f }, .Drag = 1.5f, .Size = 4, .Stretch = 5, .Box = { 20, 15, 30 }, .Color = Air,
			.Strength = 0.35f, .bFacing = true },
		{ .Count = 14, .Life = 1.3f, .Speed = 700, .Spread = 0.3f, .Direction = { -1, 0, 0.1f }, .Drag = 2, .Size = 24,
			.Grow = 3, .Spin = 0.5f, .Box = { 15, 10, 10 }, .Color = Dirt, .Strength = 0.5f, .bFacing = true } };
	const FPart ThudParts[] = {
		{ .Count = 28, .Life = 1.3f, .Speed = 400, .Spread = 1, .Lift = 0.3f, .Drag = 3, .Size = 26, .Grow = 3,
			.Spin = 0.5f, .Box = { 40, 10, 3 }, .Color = Dirt, .Strength = 0.7f },
		{ .Blend = EBlend::Bits, .Shape = EShape::Chunk, .Mode = EMode::Tumbling, .Count = 10, .Life = 1,
			.Speed = 380, .Spread = 0.6f, .Gravity = Fall, .Drag = 0.3f, .Size = 7, .Spin = 12, .Box = { 20, 8, 3 },
			.Color = Stone } };
	const FPart LeafParts[] = {
		{ .Blend = EBlend::Bits, .Shape = EShape::Leaf, .Mode = EMode::Tumbling, .Count = 22, .Life = 3.2f,
			.Stagger = 0.6f, .Speed = 120, .Spread = 2, .Gravity = 220, .Drag = 2.5f, .Size = 18, .Flutter = 30,
			.Spin = 6, .Box = { 60, 20, 30 } } };
	const FPart RustleParts[] = {
		{ .Blend = EBlend::Bits, .Shape = EShape::Leaf, .Mode = EMode::Tumbling, .Count = 7, .Life = 2.6f,
			.Stagger = 0.5f, .Speed = 70, .Spread = 1.5f, .Gravity = 200, .Drag = 2.5f, .Size = 15, .Flutter = 30,
			.Spin = 6, .Box = { 30, 15, 15 } } };
	const FPart CelebrationParts[] = {
		{ .Blend = EBlend::Bits, .Shape = EShape::Petal, .Mode = EMode::Tumbling, .Count = 36, .Life = 3,
			.Stagger = 0.3f, .Speed = 750, .Spread = 0.6f, .Gravity = 450, .Drag = 1.6f, .Size = 14, .Flutter = 25,
			.Spin = 8, .Box = { 20, 10, 10 }, .Roughness = 0.6f },
		{ .Blend = EBlend::Glint, .Shape = EShape::Star, .Count = 24, .Life = 1.2f, .Stagger = 0.6f, .Speed = 500,
			.Spread = 1.2f, .Gravity = -30, .Drag = 2, .Size = 16, .Box = { 20, 10, 10 }, .Color = Gold,
			.Strength = 4 } };

	/** A body falling from high into the water (a flung passenger): a crown of drops, a column of spray, rings. */
	const FPart PlungeParts[] = {
		{ .Shape = EShape::Drop, .Count = 140, .Life = 1.1f, .Stagger = 0.08f, .Speed = 760, .Spread = 0.35f,
			.Gravity = Fall, .Drag = 0.3f, .Size = 7, .Stretch = 1.5f, .Box = { 20, 10, 2 }, .Color = Water,
			.Strength = 0.7f },
		{ .Count = 26, .Life = 1.5f, .Stagger = 0.12f, .Speed = 380, .Spread = 0.25f, .Lift = 0.9f, .Gravity = 500,
			.Drag = 1.2f, .Size = 30, .Grow = 3, .Spin = 0.5f, .Box = { 15, 10, 4 }, .Color = Foam, .Strength = 0.55f },
		{ .Count = 18, .Life = 1.2f, .Stagger = 0.1f, .Speed = 220, .Spread = 1.2f, .Lift = 0.4f, .Gravity = 300,
			.Drag = 2, .Size = 26, .Grow = 3, .Spin = 0.5f, .Box = { 25, 10, 4 }, .Color = Foam, .Strength = 0.45f },
		{ .Shape = EShape::Ring, .Mode = EMode::Flat, .Count = 4, .Life = 2.2f, .Stagger = 0.45f, .Size = 60, .Grow = 5,
			.Color = Foam, .Strength = 0.6f } };

	/**
	 * A copter falling into the water (FUghDunks; its body some 2 m wide): a column of water shooting up and falling
	 * back, a crown of drops thrown out all round, fine drops high up, spray rolling over the surface, foam lying on it,
	 * rings running out.
	 */
	const FPart DunkParts[] = {
		{ .Count = 50, .Life = 2, .Stagger = 0.12f, .Speed = 1000, .Spread = 0.14f, .Lift = 1, .Gravity = 900,
			.Drag = 0.6f, .Size = 60, .Grow = 3, .Spin = 0.4f, .Box = { 60, 15, 6 }, .Color = Foam, .Strength = 0.55f },
		{ .Shape = EShape::Drop, .Count = 260, .Life = 1.6f, .Stagger = 0.08f, .Speed = 1100, .Spread = 0.6f,
			.Gravity = Fall, .Drag = 0.3f, .Size = 9, .Stretch = 1.5f, .Box = { 100, 16, 3 }, .Color = Water,
			.Strength = 0.7f },
		{ .Shape = EShape::Drop, .Count = 100, .Life = 1.9f, .Stagger = 0.15f, .Speed = 1400, .Spread = 0.15f,
			.Gravity = Fall, .Drag = 0.5f, .Size = 6, .Stretch = 1.2f, .Box = { 50, 12, 3 }, .Color = Water,
			.Strength = 0.7f },
		{ .Count = 34, .Life = 1.8f, .Stagger = 0.12f, .Speed = 600, .Spread = 1.4f, .Lift = 0.35f, .Gravity = 200,
			.Drag = 2, .Size = 55, .Grow = 3.5f, .Spin = 0.5f, .Box = { 100, 16, 4 }, .Color = Foam, .Strength = 0.45f },
		{ .Mode = EMode::Flat, .Count = 24, .Life = 3.6f, .Stagger = 0.25f, .Speed = 240, .Spread = 1, .Lift = 0,
			.Drag = 1.5f, .Size = 80, .Grow = 2.5f, .Spin = 0.3f, .Box = { 110, 40, 0 }, .Color = Foam,
			.Strength = 0.65f },
		{ .Shape = EShape::Ring, .Mode = EMode::Flat, .Count = 5, .Life = 3.2f, .Stagger = 0.6f, .Size = 160,
			.Grow = 5, .Color = Foam, .Strength = 0.6f } };
	/** Foam boiling up where a copter comes up again (FUghDunks): foam on the water, a few drops, a ring (as Dunk). */
	const FPart BoilParts[] = {
		{ .Mode = EMode::Flat, .Count = 12, .Life = 2.4f, .Stagger = 0.3f, .Speed = 130, .Spread = 1, .Lift = 0,
			.Drag = 1.5f, .Size = 55, .Grow = 2.5f, .Spin = 0.3f, .Box = { 70, 20, 0 }, .Color = Foam, .Strength = 0.6f },
		{ .Shape = EShape::Drop, .Count = 40, .Life = 0.8f, .Stagger = 0.3f, .Speed = 350, .Spread = 0.6f,
			.Gravity = Fall, .Drag = 0.5f, .Size = 6, .Stretch = 1, .Box = { 70, 10, 2 }, .Color = Water,
			.Strength = 0.65f },
		{ .Shape = EShape::Ring, .Mode = EMode::Flat, .Count = 2, .Life = 2.4f, .Stagger = 0.4f, .Size = 110,
			.Grow = 4, .Color = Foam, .Strength = 0.6f } };

	/** In the order of EUghBurst. */
	const FBurst Bursts[] = {
		{ TEXT("splash"), SplashParts, false, {}, 18, 0.35, ERest::Water },
		{ TEXT("surf"), SurfParts, false, {}, 60, 2.5, ERest::Water, -200 },
		{ TEXT("explosion"), ExplosionParts, false, { FLinearColor(1.f, 0.55f, 0.25f), 60, 0.7f }, 30, 0.25, ERest::Air },
		{ TEXT("dust"), DustParts, false, {}, 20, 0.4, ERest::Ground },
		{ TEXT("shells"), ShellParts, false, {}, 16, 0.45, ERest::Air },
		{ TEXT("glints"), GlintParts, false, { Gold, 8, 0.6f }, 14, 0.3, ERest::Air },
		{ TEXT("feathers"), FeatherParts, false, {}, 16, 0.6, ERest::Air },
		{ TEXT("downdraft"), DowndraftParts, true, {}, 14, 1, ERest::Air },
		{ TEXT("gust"), GustParts, false, {}, 40, 0.5, ERest::Air },
		{ TEXT("thud"), ThudParts, false, {}, 16, 0.3, ERest::Ground },
		{ TEXT("leaves"), LeafParts, false, {}, 20, 1.2, ERest::Air },
		{ TEXT("celebration"), CelebrationParts, false, { Gold, 10, 1.f }, 30, 0.6, ERest::Air },
		{ TEXT("rustle"), RustleParts, false, {}, 20, 0.8, ERest::Air, -150 },
		{ TEXT("plunge"), PlungeParts, false, {}, 24, 0.35, ERest::Water },
		{ TEXT("dunk"), DunkParts, false, {}, 45, 0.4, ERest::Water },
		{ TEXT("boil"), BoilParts, false, {}, 25, 0.4, ERest::Water } };
	static_assert(UE_ARRAY_COUNT(Bursts) == int32(EUghBurst::Count));
}

const FBurst& UghBursts::Get(EUghBurst Burst)
{
	return Bursts[int32(Burst)];
}

TOptional<EUghBurst> UghBursts::Find(const FString& Name)
{
	for (int32 Burst = 0; Burst < UE_ARRAY_COUNT(Bursts); ++Burst)
	{
		if (Name == Bursts[Burst].Name)
		{
			return EUghBurst(Burst);
		}
	}
	return {};
}

double UghBursts::Lasts(EUghBurst Burst)
{
	double Last = 0;
	for (const FPart& Part : Get(Burst).Parts)
	{
		Last = FMath::Max(Last, double(Part.Stagger + Part.Life));
	}
	return Last;
}
