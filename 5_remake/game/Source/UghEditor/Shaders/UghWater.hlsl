// The water (UghMaterials::Water): the body of the custom node of M_UghWater, a Single Layer Water material, which
// UghMakeAssets reads from this file. Its inputs: Position (world, cm), VertexNormal (world: up on the surface,
// towards the camera on the sea's cut near the camera), CameraPosition (world), PixelDepth and BehindDepth (how
// far from the camera the water and the scene behind it are, along its view), Time (seconds), WaterLevel (the
// world's z of the surface), Rain (0 .. 1), Wind (-1 .. 1: where the wind blows along x), Caustics (how bright the
// caustics are, 0 none), Sun (where the sunlight goes, world) and Ring0 .. Ring5 (what floats on the water: xyz its
// place on the surface, w how much it stirs it, 0 nothing). It returns the colour of the foam; WaterNormal (world),
// WaterOpacity (the foam's cover), WaterRough, WaterSpecular, Scattering and Absorption (the water's coefficients,
// 1/cm) and Behind (the light of what is behind the water: the caustics) are its other outputs.
//
// The surface of a sea: a swell rolling in towards the cliff and chop drifting with the wind, the rings of what swims
// or floats, the rings of raindrops; foam where the surface meets the rock (washing up and down) and around what
// floats; caustics, the light the waves focus, dancing on what lies below it where the sun shone through the surface;
// far from the stone the open sea's higher swell with whitecaps.
// The cut near the camera (seen when the rising water is above the camera) is not a surface: no reflection, only the
// water seen through it. The water is a clear green-blue; what is behind it is seen through as much water as the view
// crosses and as deep as it lies (its light came down through the water), so that the deep is bluer and darker.

#define UGH_HASH(P) frac(sin(dot(P, float2(127.1, 311.7))) * 43758.5453)
#define UGH_NOISE(P, R) { float2 i_ = fmod(floor(P), 289.0); float2 f_ = frac(P); float2 s_ = f_ * f_ * (3 - 2 * f_); \
	R = lerp(lerp(UGH_HASH(i_), UGH_HASH(i_ + float2(1, 0)), s_.x), \
		lerp(UGH_HASH(i_ + float2(0, 1)), UGH_HASH(i_ + float2(1, 1)), s_.x), s_.y); }

const float Pi2 = 6.2831853;
const float Gravity = 980;   // cm/s^2: deep water waves of wavelength L travel at sqrt(g L / 2 pi)
float t = fmod(Time, 1000.0);
bool top = VertexNormal.z > 0.5;

// what is behind the water on this view ray, how much water the view crosses, how deep that lies
float3 toPixel = Position - CameraPosition;
float3 behind = CameraPosition + toPixel * (max(BehindDepth, PixelDepth + 0.01) / max(PixelDepth, 1.0));
float through = length(behind - Position);
float below = max(WaterLevel - behind.z, 0.0);

// the surface's slope: swells and chop (x across, y deep), each a wave of direction, wavelength (cm), steepness
float2 p = Position.xy;
float2 slope = 0;
float storm = abs(Wind);
// the open sea far from the stone (seen only while the camera flies in: the stone, AUghSeaStack, stands 54 m to each
// side of the screen's middle and 52 m deep behind its plane): a higher swell, whitecaps, a rougher surface
float open = smoothstep(2500, 25000, length(max(abs(p - float2(1600, -2600)) - float2(5500, 2700), 0)));
float away = length(toPixel);
// (the swell rolls in towards the cliff: world y is minus the depth)
const float4 waves[6] = { float4(0.2, -1.0, 1300, 0.06), float4(-0.5, -0.8, 750, 0.05), float4(0.8, -0.6, 420, 0.045),
	float4(-0.6, 0.8, 230, 0.04), float4(0.95, -0.3, 120, 0.035), float4(-0.2, 1.0, 60, 0.03) };
[unroll] for (int i = 0; i < 6; ++i)
{
	float2 d = normalize(waves[i].xy + float2(Wind * 1.5, 0));
	float k = Pi2 / waves[i].z;
	float phase = k * dot(d, p) - sqrt(Gravity * k) * t;
	// (on the open sea a wave fades out from 30 to 60 of its wavelengths away: no shimmer of the small ones far off)
	float lod = lerp(1, saturate(2 - away / (waves[i].z * 30)), open);
	slope += d * waves[i].w * (1 + storm * 1.5) * lod * cos(phase);
}
// (seen at a grazing angle its waves would mirror what is below the horizon - the sky light has nothing there: it lies
// smoother far off)
slope *= lerp(1, 2.2 * pow(saturate(-normalize(toPixel).z / 0.2), 1.5), open);
// the rings of what floats: ripples running out of it
float foam = 0;
const float4 rings[6] = { Ring0, Ring1, Ring2, Ring3, Ring4, Ring5 };
[unroll] for (int r = 0; r < 6; ++r)
{
	float2 d = p - rings[r].xy;
	float dist = max(length(d), 0.001);
	float k = Pi2 / 22, fade = rings[r].w * exp(-dist / 140) * saturate(dist / 15);
	slope += d / dist * fade * 0.35 * cos(k * dist - Pi2 * 1.6 * t);
	foam += rings[r].w * saturate(1 - dist / 45) * 0.7;
}
// raindrops: each cell of a grid a drop now and then, a ring spreading from it (two grids, offset)
[unroll] for (int g = 0; g < 2; ++g)
{
	float2 cellP = p / 18 + g * 0.5;
	float2 cell = fmod(floor(cellP), 289.0);
	float seed = UGH_HASH(cell + g * 7);
	float age = frac(t * 1.7 + seed);
	float2 d = frac(cellP) - (0.25 + 0.5 * float2(seed, UGH_HASH(cell + 3.1)));
	float dist = max(length(d), 0.001);
	float radius = age * 0.45;
	float band = saturate(1 - abs(dist - radius) * 14) * (1 - age);
	slope += d / dist * band * Rain * 0.6 * sin((dist - radius) * 40);
}
float3 waved = normalize(float3(-slope, 1));

// foam where the surface meets the rock (the view ray soon meets it below) and around what floats, broken by noise
float n1, n2;
UGH_NOISE(p * 0.07 + float2(t * 0.15, t * 0.05), n1);
UGH_NOISE(p * 0.21 - float2(t * 0.1, t * 0.2), n2);
// (the waves wash up and down the rock: the foam's band breathes)
float wash = 35 + 25 * sin(t * 0.9 + p.x * 0.004 + n1 * 2);
float shore = 1 - saturate(through / wash);
foam = saturate((saturate(shore * 1.2 + foam) - (n1 * 0.6 + n2 * 0.4) * 0.8) * 1.8);
float n3;
UGH_NOISE(p * 0.012 + float2(t * 0.05, 0), n3);
foam = max(foam, open * smoothstep(0.86, 0.96, n3) * smoothstep(0.5, 0.8, n1) * 0.6);

// caustics: where the sunlight lying on what is behind came through the surface, the waves' pattern there
float3 sun = normalize(Sun);
// (a pattern repeating every 2 pi of q, which the iterations keep seamless, far from 0 where it is fine)
float2 q = frac((behind - sun * (behind.z - WaterLevel) / min(sun.z, -0.2)).xy * 0.03 / Pi2) * Pi2 - 250;
float c = 1.0, focus = 0.005;
float2 j = q;
[unroll] for (int n = 0; n < 4; ++n)
{
	float s = t * 0.35 * (1 - 3.5 / (n + 1));
	j = q + float2(cos(s - j.x) + sin(s + j.y), sin(s - j.y) + cos(s + j.x));
	c += 1 / length(float2(q.x / (sin(j.x + s) / focus), q.y / (cos(j.y + s) / focus)));
}
c = min(pow(abs(1.17 - pow(c / 4, 1.4)), 8), 3);
// under the surface, fading with depth and deep in the cave (in its shadow)
float lit = saturate(below / 15) * exp(-below / 350) * saturate(1 - (-behind.y - 150) / 250);   // world y = -depth
Behind = 1 + Caustics * c * lit;

// the water: absorbing red most, scattering green-blue. The engine dims what is behind by the way its light came down
// from the height of this point of the water: on the cut that is the depth of the cut's point, which the view's way
// through the water takes on here; the light scattered there came down as deep, the deep is darker; and only the
// first metres the view crosses are lit (the engine would light them all: a flooded cave would glow)
float deep = max(WaterLevel - Position.z, 0.0);
float scale = (through + deep) / max(through, 1.0);
Absorption = float3(0.0035, 0.0011, 0.0009) * scale;
// (the open sea has no floor: lit as though its view crossed a few metres, a deep blue)
float crossed = 200 / (200 + lerp(through, min(through, 400.0), open));
Scattering = float3(0.0008, 0.0016, 0.0017) * lerp(1, float3(0.6, 0.85, 1.1), open) * scale * exp(-deep * 0.0015) *
	crossed;

WaterNormal = top ? waved : normalize(VertexNormal);
WaterOpacity = top ? foam * 0.7 : 0;
WaterRough = top ? 0.03 + Rain * 0.08 + foam * 0.6 + 0.1 * open : 1;
WaterSpecular = top ? 0.5 : 0;
return float3(0.75, 0.78, 0.78);
