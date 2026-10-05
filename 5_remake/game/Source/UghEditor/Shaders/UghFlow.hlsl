// Flowing water (UghMaterials::Flow): the body of the custom node of M_UghFlow, which UghMakeAssets reads from this
// file. Its inputs: UV (u across the water 0 .. 1, v the seconds the water has flowed to get there), Flow (x how much
// it falls: 0 a stream lying on the rock, 1 a sheet falling free; y how near its foot it is, 0 .. 1), Time (seconds),
// Position (world) and WaterLevel (the world's z of the sea's surface: under it the water is the sea's). It returns
// its colour; FlowNormal (tangent space: x across, y along its way), FlowOpacity and FlowRough are its other outputs.
//
// The pattern moves with the water (what is at v now was at v - dt dt seconds ago): a stream's ripples and the foam
// along its banks and where it lands from its spring; a falling sheet aerated white with long streaks and gaps between
// its strands, whiter towards its foot, its edges frayed.

#define UGH_HASH(P) frac(sin(dot(P, float2(127.1, 311.7))) * 43758.5453)
#define UGH_NOISE(P, R) { float2 i_ = fmod(floor(P), 289.0); float2 f_ = frac(P); float2 s_ = f_ * f_ * (3 - 2 * f_); \
	R = lerp(lerp(UGH_HASH(i_), UGH_HASH(i_ + float2(1, 0)), s_.x), \
		lerp(UGH_HASH(i_ + float2(0, 1)), UGH_HASH(i_ + float2(1, 1)), s_.x), s_.y); }

float t = fmod(Time, 1000.0);
float fall = saturate(Flow.x), foot = saturate(Flow.y);
// how often its pattern repeats across it and along its way (per second of flow): long streaks where it falls
float2 scale = lerp(float2(5, 7), float2(16, 4), fall);
float2 q = float2(UV.x * scale.x, (UV.y - t) * scale.y);
// its relief (0 .. 1) and how it slopes, by differences
const float e = 0.05;
float h, hu, hv, a, b, c;
UGH_NOISE(q, a);
UGH_NOISE(q * 2.1 + 17, b);
UGH_NOISE(q * 4.3 + 5, c);
h = 0.6 * a + 0.3 * b + 0.1 * c;
UGH_NOISE(q + float2(e, 0), a);
UGH_NOISE((q + float2(e, 0)) * 2.1 + 17, b);
hu = 0.6 * a + 0.3 * b + 0.1 * c;
UGH_NOISE(q + float2(0, e), a);
UGH_NOISE((q + float2(0, e)) * 2.1 + 17, b);
hv = 0.6 * a + 0.3 * b + 0.1 * c;
float2 slope = float2(hu - h, hv - h) / e * lerp(0.25, 0.08, fall);
FlowNormal = normalize(float3(-slope, 1));

// a stream: foam along its banks and where it comes out of its hole (its first half second), flecks on its ripples
float bank = 1 - smoothstep(0.0, 0.22, min(UV.x, 1 - UV.x));
float streamFoam = saturate(bank * 0.55 + (h - 0.6) * 2.5 + saturate(1 - UV.y / 0.5) * 0.7);
// a falling sheet: white, darker streaks, whiter down to its foot; strands with gaps between them, frayed edges
// (and clumps of water tumbling down it, longer the faster it falls)
float clump;
UGH_NOISE(float2(UV.x * 5 + 11, (UV.y - t) * 9), clump);
float fallFoam = saturate(0.4 + (h - 0.45) * 2.4 + (clump - 0.5) * 0.6 + foot * 0.5);
float strands = saturate(0.1 + h * 1.5 + (clump - 0.5) * 0.8) * smoothstep(0.0, 0.2, UV.x) * smoothstep(1.0, 0.8, UV.x);
float foam = lerp(streamFoam, fallFoam, fall);
float opacity = lerp(0.35 + 0.5 * foam, (0.3 + 0.65 * foam) * strands, fall);
// what is under the sea's surface is the sea's
FlowOpacity = opacity * saturate((Position.z - WaterLevel) / 4);
FlowRough = lerp(0.05, 0.45, foam);
return lerp(float3(0.02, 0.05, 0.055), float3(0.8, 0.84, 0.86), foam);
