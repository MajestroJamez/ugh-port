// A campfire's flame (UghMaterials::Fire): the body of the custom node of M_UghFire, which UghMakeAssets reads from
// this file. Its inputs: UV the card's coordinates (u across, v down from its top), Time (seconds), Seed (a number
// 0 .. 1 of the card: each flickers its own way), Wind (-1 .. 1: the flame leans that way), Intensity. It returns the
// light the flame gives off (additive, unlit): tongues of fire licking upwards, eaten by noise that rises with time,
// pale in their hottest core, yellow, orange and dark red at their edges, a few sparks above them.

#define UGH_HASH(P) frac(sin(dot(P, float2(127.1, 311.7))) * 43758.5453)
// value noise at point P into R: the corners' hashes blended smoothly (the cells wrap: sin stays precise)
#define UGH_NOISE(P, R) { float2 i_ = fmod(floor(P), 289.0); float2 f_ = frac(P); float2 s_ = f_ * f_ * (3 - 2 * f_); \
	R = lerp(lerp(UGH_HASH(i_), UGH_HASH(i_ + float2(1, 0)), s_.x), \
		lerp(UGH_HASH(i_ + float2(0, 1)), UGH_HASH(i_ + float2(1, 1)), s_.x), s_.y); }

float up = 1 - UV.y;                  // 0 at the bottom of the card, 1 at its top
float t = fmod(Time, 600.0) + Seed * 17.0;
float n1, n2, n3;
UGH_NOISE(float2(UV.x * 4.0 + Seed * 9, up * 3.0 - t * 2.6), n1);
UGH_NOISE(float2(UV.x * 9.0 - Seed * 5, up * 7.0 - t * 4.1), n2);
UGH_NOISE(float2(UV.x * 18.0 + Seed * 3, up * 14.0 - t * 6.3), n3);
float n = n1 * 0.55 + n2 * 0.3 + n3 * 0.15;

// the tongues: a teardrop narrowing upwards, its middle wavering and leaning with the wind, eaten by the rising noise
float across = UV.x - 0.5 - Wind * up * up * 0.25 - (n2 - 0.5) * 0.25 * up;
float width = 0.45 * pow(saturate(1 - up), 0.8) * saturate(up * 6 + 0.25);
float shape = saturate(1 - abs(across) / max(width, 0.001));
float heat = saturate((shape * (1.2 - up * 0.9) - (1 - n) * (0.35 + 0.6 * up)) * 1.6);

// dark red at the edges, orange, yellow, pale in the hottest core
float3 colour = lerp(float3(0.6, 0.05, 0.0), float3(1.0, 0.35, 0.03), saturate(heat * 2.2));
colour = lerp(colour, float3(1.0, 0.75, 0.2), saturate(heat * 2.2 - 0.9));
colour = lerp(colour, float3(1.0, 0.95, 0.7), saturate(heat * 3 - 2.4));

// sparks: rising specks above the tongues
float2 cell = float2(UV.x * 9, up * 6 - t * 1.5);
float spark = UGH_HASH(fmod(floor(cell), 289.0) + Seed * 31);
float2 inCell = frac(cell) - 0.5;
float sparks = step(0.93, spark) * saturate(1 - length(inCell) * 6) * saturate(up * 2 - 0.5) * saturate((1 - up) * 3);

// bright well out to the flame's edges (additive: a faint edge would vanish on a lit wall)
return (colour * sqrt(heat) + float3(1.0, 0.55, 0.15) * sparks) * Intensity;
