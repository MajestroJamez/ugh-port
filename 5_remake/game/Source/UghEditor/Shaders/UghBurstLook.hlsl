// The look of a burst's particle (UghMaterials::Burst, Bits, Glint): the body of the custom node that draws it, which
// UghMakeAssets reads from this file. Inputs: UV (the quad, 0 .. 1), Seed (its own, 0 .. 1), AgeLife (how far it is in
// its life 0 .. 1, seconds it lives), Shape (UghBursts::EShape), Where (the pixel in the world) and WaterLevel
// (nothing under the water's surface). It returns the colour (times the parameter Color) and how much of what is
// behind it the particle covers (alpha, 0 .. 1).

float2 d = UV - 0.5;
float2 q = d * 2;   // -1 .. 1, y down the quad
float r = length(q);
float a = atan2(d.y, d.x);
float age = AgeLife.x;
float fade = smoothstep(0, 0.06, age) * pow(saturate(1 - age), 1.3);
float3 color = 1;
float cover = 0;
int shape = (int)round(Shape);
if (shape == 0 || shape == 9)
{
	// a soft ragged puff (dust, smoke, spray), billowing as it grows; fire: hot white-yellow cooling to a deep orange
	float rag = 0.16 * sin(a * 3 + Seed.x * 40 + age * 2) + 0.09 * sin(a * 7 + Seed.y * 50 - age * 3);
	cover = pow(saturate(1 - r * (1 + rag)), 1.6) * fade;
	color = 0.85 + 0.25 * saturate(1 - r);
	if (shape == 9)
	{
		// licking tongues and holes, a hot core
		float licks = sin(q.x * 7 + Seed.x * 30 + age * 6) * sin(q.y * 6 + Seed.y * 20 - age * 9) +
			0.5 * sin((q.x + q.y) * 13 + Seed.x * 50 - age * 7);
		cover = saturate((1 - r * (1 + rag)) * 1.8 - 0.35 - 0.3 * licks) * fade;
		float heat = saturate(1.2 - r * 1.1 - age * 1.3 + 0.2 * licks);
		color = lerp(float3(0.9, 0.18, 0.03), float3(1, 0.62, 0.22), heat) + float3(0.6, 0.5, 0.35) * heat * heat;
	}
}
else if (shape == 1)
{
	cover = pow(saturate(1 - r), 0.7) * pow(saturate(1 - age), 0.6);   // a drop, a speck
}
else if (shape == 2)
{
	// a ring spreading on the water, broken here and there
	cover = saturate(1 - abs(r - 0.8) / 0.14) * (0.65 + 0.35 * sin(a * 9 + Seed.x * 30)) * pow(saturate(1 - age), 1.5);
}
else if (shape == 3)
{
	// a leaf: pointed at both ends, its midrib and veins paler, green to autumn brown
	float w = 0.42 * (1 - q.y * q.y) * (1 + 0.1 * q.y);
	cover = abs(q.x) < w ? 1 : 0;
	float vein = saturate(1 - abs(q.x) * 25) + 0.4 * saturate(1 - abs(frac((abs(q.x) * 1.4 - q.y) * 3) - 0.5) * 12);
	color = lerp(float3(0.1, 0.22, 0.04), float3(0.38, 0.27, 0.08), Seed.x * Seed.x) * (0.85 + 0.4 * vein);
}
else if (shape == 4)
{
	// a feather: the quill at the bottom, the vane on both sides narrowing to the tip, its barbs, soft at the edge
	float along = (1 - q.y) / 2;   // 0 quill .. 1 tip
	float w = 0.55 * sqrt(saturate(along * (1.1 - along) * 4.2)) * (along > 0.12 ? 1 : 0);
	float barbs = 0.7 + 0.3 * sin((abs(q.x) * 1.5 + q.y) * 45 + Seed.y * 9);
	float shaft = saturate(1 - abs(q.x) * 25);
	cover = max(abs(q.x) < w * (0.8 + 0.2 * barbs) ? 1 : 0, shaft * (along < 0.98 ? 1 : 0));
	// pale grey to tawny with darker bars across, paler at the shaft
	float bars = 0.85 + 0.15 * sin(along * 22 + Seed.x * 6);
	color = lerp(float3(0.85, 0.82, 0.76), float3(0.55, 0.45, 0.34), Seed.x) * bars * barbs * (1 + 0.3 * shaft);
}
else if (shape == 5)
{
	// a cowrie shell: a glossy dome with a toothed slit down its middle, speckled brown at its back
	float e = length(q / float2(0.62, 0.92));
	cover = e < 1 ? 1 : 0;
	float slit = saturate(1 - abs(q.x + 0.06 * sin(q.y * 3)) * 18) * (abs(q.y) < 0.75 ? 1 : 0);
	float teeth = slit * (0.5 + 0.5 * sin(q.y * 45));
	float speckle = step(0.8, frac(sin(dot(floor(q * 9), float2(12.9, 78.2)) + Seed.x * 10) * 43758.5));
	color = float3(0.95, 0.88, 0.74) * (1 - 0.65 * teeth) * (1 - 0.35 * speckle * saturate(e * 1.4 - 0.3)) *
		(0.8 + 0.25 * (1 - e));
}
else if (shape == 6)
{
	// a splinter of wood or a chip of stone: angular, uneven
	float edge = 0.62 + 0.22 * cos(floor(a * 0.95 + Seed.x * 6) * 2.3 + Seed.y * 20) + 0.1 * cos(a * 5 + Seed.x * 30);
	cover = r < edge ? 1 : 0;
	color = (0.7 + 0.5 * Seed.y) * (0.85 + 0.3 * saturate(1 - r / edge));
}
else if (shape == 7)
{
	// a glint: a bright core and four rays, twinkling
	float rays = pow(saturate(1 - abs(q.x) * 12), 2) * saturate(1 - abs(q.y)) +
		pow(saturate(1 - abs(q.y) * 12), 2) * saturate(1 - abs(q.x));
	cover = (0.8 * rays + pow(saturate(1 - r), 3)) * (0.6 + 0.4 * sin(age * 30 + Seed.x * 60)) * fade;
}
else
{
	// a petal: a rounded teardrop, pink, white, yellow or orange
	float w = 0.55 * sqrt(saturate((1 - q.y) / 2)) * saturate((1 + q.y) * 3);
	cover = abs(q.x) < w ? 1 : 0;
	float hue = frac(Seed.x * 3.7);
	color = hue < 0.3 ? float3(0.9, 0.35, 0.5) : hue < 0.55 ? float3(0.95, 0.93, 0.88)
		: hue < 0.8 ? float3(0.95, 0.75, 0.15) : float3(0.95, 0.45, 0.12);
	color *= 0.8 + 0.25 * (1 - abs(q.x) / max(w, 0.01));
}
return float4(color, cover * (Where.z > WaterLevel - 2 ? 1 : 0));   // (a ring lies on the surface)
