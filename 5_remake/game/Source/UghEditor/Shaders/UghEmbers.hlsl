// Burning wood (UghMaterials::Embers): the body of the custom node of M_UghEmbers, which UghMakeAssets reads from this
// file: the charred logs of a campfire, its coals, a torch's head. Its inputs: Position (world, cm), Normal (the
// surface's, world), Time (seconds), Glow (how bright its embers glow, 0 none), Underside (0 .. 1: how much more they
// glow where the surface faces down or aside - towards the bed of the fire - than up). It returns the charcoal's colour
// (black and grey, cracked, a little ash), the light of its embers in EmbersLight (orange in the cracks, yellow in
// the hottest, breathing slowly, each spot its own way) and its roughness in EmbersRough.

#define UGH_HASH3(P) frac(sin(dot(P, float3(127.1, 311.7, 74.7))) * 43758.5453)
// value noise at point P into R: the corners' hashes blended smoothly (the cells wrap: sin stays precise)
#define UGH_NOISE3(P, R) { float3 i_ = fmod(floor(P), 289.0); float3 f_ = frac(P); float3 s_ = f_ * f_ * (3 - 2 * f_); \
	R = lerp(lerp(lerp(UGH_HASH3(i_), UGH_HASH3(i_ + float3(1, 0, 0)), s_.x), \
		lerp(UGH_HASH3(i_ + float3(0, 1, 0)), UGH_HASH3(i_ + float3(1, 1, 0)), s_.x), s_.y), \
		lerp(lerp(UGH_HASH3(i_ + float3(0, 0, 1)), UGH_HASH3(i_ + float3(1, 0, 1)), s_.x), \
		lerp(UGH_HASH3(i_ + float3(0, 1, 1)), UGH_HASH3(i_ + float3(1, 1, 1)), s_.x), s_.y), s_.z); }

float3 p = Position * 0.35;   // cracks about 3 cm apart
float n1, n2, breath;
UGH_NOISE3(p, n1);
UGH_NOISE3(p * 2.7 + 13.1, n2);
UGH_NOISE3(Position * 0.08 + float3(0, 0, fmod(Time, 1000.0) * 0.6), breath);
// the cracks of charcoal: where the noise crosses its middle (the blocks between them)
float crack = pow(saturate(1 - abs(n1 * 0.7 + n2 * 0.3 - 0.5) * 7), 2);
float facing = lerp(1, saturate(0.65 - Normal.z), Underside);
float heat = Glow * facing * (0.35 + 0.9 * breath * breath);
float glow = heat * (crack + 0.12 * n2);
EmbersLight = float3(1.0, 0.22, 0.03) * glow + float3(1.0, 0.55, 0.15) * glow * glow * 0.4;
EmbersRough = lerp(0.9, 0.6, n2);
// black charcoal, grey ash on its tops and blocks, darker in the cracks
float ash = saturate((Normal.z - 0.4) * 2) * smoothstep(0.55, 0.8, n2);
return lerp(float3(0.025, 0.022, 0.02), float3(0.18, 0.17, 0.16), ash) * (1 - crack * 0.7) * (0.7 + 0.6 * n1);
