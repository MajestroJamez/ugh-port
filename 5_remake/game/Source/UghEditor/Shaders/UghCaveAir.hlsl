// The air of the cave (UghMaterials::CaveAir, step 30c): the body of the custom node of M_UghCaveAir, which UghMakeAssets
// reads from this file. The card (AUghCaveAir) stands across the whole screen a little behind the figures' room; what
// lies behind it is seen through the cave's air. Its inputs: Position (world, cm: x across, z up; the screen's pixel
// x, y is x / 10, 192 - z / 10), Thick (how far behind the card the scene is, along the view, cm), Time (seconds),
// Air (the map of UghCaveAir::Map over the screen: r how sheltered the air is, g how much a shaft of the sun lights it,
// b the fires' glow, a how much of it is the cave's air), WaterLevel (the world's z of the surface), Direction (the
// shafts' way on the screen, pixels: x right, y down) and the colours as the exposure shows them: Haze (the air's own,
// the light of the shade), Shaft (the sun's or the moon's), FireGlow (the fires'). It returns the light the air adds
// (as the exposure shows it; the node after it makes it the scene's); CaveOpacity how much of what is behind it hides.
//
// A haze the thicker the deeper the cave reaches behind the card (the back wall far: hazy; a wall near: clear), more in
// sheltered air than in the open, drifting in slow wisps; shafts streaking down along the light's way, where the map
// says the light reaches, broken into beams across it (as through the leaves and gaps above); the fires' warm glow
// in the air around them, breathing with the flames. Never brighter than the figures (UghMood::Shade: dim).

#define UGH_HASH(P) frac(sin(dot(P, float2(127.1, 311.7))) * 43758.5453)
#define UGH_NOISE(P, R) { float2 i_ = fmod(floor(P), 289.0); float2 f_ = frac(P); float2 s_ = f_ * f_ * (3 - 2 * f_); \
	R = lerp(lerp(UGH_HASH(i_), UGH_HASH(i_ + float2(1, 0)), s_.x), \
		lerp(UGH_HASH(i_ + float2(0, 1)), UGH_HASH(i_ + float2(1, 1)), s_.x), s_.y); }

float t = fmod(Time, 1000.0);
float2 px = float2(Position.x * 0.1, 192 - Position.z * 0.1);
float2 uv = px / float2(320, 192);
float4 map = Texture2DSampleLevel(Air, View.MaterialTextureBilinearClampedSampler, uv, 0);
float inside = step(0, uv.x) * step(uv.x, 1) * step(0, uv.y) * step(uv.y, 1);
// (the air over the water only: the card goes on under it, hidden by the water)
float air = map.a * inside * saturate((Position.z - WaterLevel) / 60);
// the deeper the cave behind, the hazier (half of it 3 m behind the card)
float deep = 1 - exp(-Thick / 430);

// the haze: slow wisps drifting across, two sizes
float w1, w2;
UGH_NOISE(px * 0.045 + float2(t * 0.12, t * 0.025), w1);
UGH_NOISE(px * 0.12 - float2(t * 0.07, -t * 0.04), w2);
float wisps = saturate(0.55 + 1.1 * (0.65 * w1 + 0.35 * w2 - 0.5));
float haze = air * deep * lerp(0.35, 1, map.r) * wisps;

// the shafts: along the light's way, beams across it drifting slowly (leaves stirring above)
float2 way = normalize(Direction.xy + float2(0, 1e-4));
float across = dot(px, float2(-way.y, way.x));
float s1, s2, s3;
UGH_NOISE(float2(across * 0.11 + t * 0.04, 1.7), s1);
UGH_NOISE(float2(across * 0.37 - t * 0.06, 7.3), s2);
UGH_NOISE(float2(across * 0.05, dot(px, way) * 0.02 + t * 0.03), s3);
float beams = smoothstep(0.4, 0.85, 0.55 * s1 + 0.3 * s2 + 0.15 * s3);
float shaft = air * map.g * beams * (0.35 + 0.65 * deep) * lerp(0.7, 1.1, w1);

// the fires: a warm glow around them, breathing with the flames (slow and quick)
float f1, f2;
UGH_NOISE(float2(t * 1.3, 3.1), f1);
UGH_NOISE(float2(t * 4.7, 9.7), f2);
float glow = air * map.b * (0.8 + 0.25 * f1 + 0.15 * f2) * (0.4 + 0.6 * deep);

CaveOpacity = haze * 0.5;
return Haze * haze + Shaft * shaft + FireGlow * glow;
