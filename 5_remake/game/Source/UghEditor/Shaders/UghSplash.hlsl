// A raindrop's splash (UghMaterials::Splash): the body of the custom node of M_UghSplash, which UghMakeAssets reads
// from this file. The mesh (AUghRain) is a quad for each spot where drops hit, standing on a ledge or on the water,
// facing the camera; its inputs: UV (the quad's coordinates: u across, v down from its top), Seed (0 .. 1 of the
// spot) and Time (seconds). It returns how much of the splash covers the point (0 .. 1): now and then a drop hits
// the spot (each its own way) and for a moment a flat ring spreads at its foot while a few droplets fly up and out.

#define UGH_HASH(P) frac(sin(dot(P, float2(127.1, 311.7))) * 43758.5453)

const float Lasts = 0.22;   // seconds
float period = 0.45 + Seed * 0.6;
float age = frac(fmod(Time, 1000.0) / period + Seed * 7.3) * period / Lasts;
float2 p = float2(UV.x - 0.5, 1 - UV.y);   // from the foot of the quad's middle, up
float cover = 0;
[branch] if (age < 1)
{
	float fade = 1 - age;
	float ring = abs(length(float2(p.x, (p.y - 0.06) * 5)) - age * 0.42);
	cover += saturate(1 - ring * 22) * fade;
	[unroll] for (int k = 0; k < 5; ++k)
	{
		float s = UGH_HASH(float2(Seed * 31 + k, k * 1.7));
		float2 thrown = float2(((k + 0.5) / 5 - 0.5) * 0.9 + (s - 0.5) * 0.25, 1.3 + s * 0.7);
		float2 at = float2(thrown.x * age * 0.5, 0.06 + (thrown.y * age - 1.7 * age * age) * 0.5);
		cover += saturate(1 - length(p - at) * 28) * fade;
	}
}
return saturate(cover);
