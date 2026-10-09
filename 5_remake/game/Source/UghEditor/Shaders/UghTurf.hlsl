// The turf's grass hanging over the rock's edges (UghMaterials::Turf, FUghTurf): the body of the custom node of
// M_UghTurf. Its inputs: UV (u across a card, v from its root 0 to its tip 1), Shade the vertex colour (r the card's
// seed, g how far along it, b how trodden), Position (world, cm). It returns the base colour; TurfMask (the blades:
// masked at 0.5) and TurfRough are its other outputs.
//
// A card is Blades blades side by side, each its own length (the ends ragged), width (tapering to its tip), lean and
// shade; their colour the cliff's grass's patches (UghCliff.hlsl: lush, olive, dry - the same noise of the world's x
// and height), dark at the roots, the trodden ones drier and paler.

#define UGH_HASH(P) frac(sin(dot(P, float2(127.1, 311.7))) * 43758.5453)
#define UGH_NOISE(P, R) { float2 i_ = fmod(floor(P), 289.0); float2 f_ = frac(P); float2 s_ = f_ * f_ * (3 - 2 * f_); \
	R = lerp(lerp(UGH_HASH(i_), UGH_HASH(i_ + float2(1, 0)), s_.x), \
		lerp(UGH_HASH(i_ + float2(0, 1)), UGH_HASH(i_ + float2(1, 1)), s_.x), s_.y); }

const float Blades = 3;
float seed = Shade.r, trodden = Shade.b;
float across = UV.x * Blades;
float blade = floor(across);
float h1 = UGH_HASH(float2(blade + 0.5, seed * 97.0));
float h2 = UGH_HASH(float2(blade + 7.3, seed * 57.0));
float h3 = UGH_HASH(float2(blade + 3.1, seed * 31.0));
// along this blade (it ends at 1): the long and the short ones
float len = lerp(0.35, 1.0, pow(h1, 0.7));
float v = UV.y / len;
// its middle leaning aside towards the tip, its width tapering
float lean = (h2 - 0.5) * 1.2 * v * v;
float halfWidth = lerp(0.12, 0.3, h3) * (1 - 0.85 * pow(saturate(v), 1.4));
float off = abs(frac(across) - 0.5 - lean);
TurfMask = saturate((halfWidth - off) / max(fwidth(across), 1e-3) + 0.5) * step(v, 1);

// the cliff's grass patches (UghCliff.hlsl)
float3 m = Position * 0.01;
float q1, q2;
UGH_NOISE(float2(m.x * 0.45, m.z * 0.35) + 3.7, q1);
UGH_NOISE(float2(m.x * 1.9, m.z * 1.3) + 11.3, q2);
float dry = saturate(smoothstep(0.3, 0.8, 0.7 * q1 + 0.3 * q2) + 0.25 * (h2 - 0.5) + 0.3 * (seed - 0.5) + 0.5 * trodden);
const float3 lush = float3(0.035, 0.075, 0.014), olive = float3(0.075, 0.08, 0.022), straw = float3(0.17, 0.13, 0.045);
float3 c = dry < 0.5 ? lerp(lush, olive, dry * 2) : lerp(olive, straw, dry * 2 - 1);
// each blade its shade, the roots in the dark of the tuft
c *= lerp(0.7, 1.25, h1 * 0.6 + h3 * 0.4) * lerp(0.35, 1.0, saturate(UV.y * 3 + 0.1));
TurfRough = 0.7;
return c;
