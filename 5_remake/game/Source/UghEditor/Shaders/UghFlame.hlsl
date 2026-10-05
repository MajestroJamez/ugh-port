// A flame on a card (UghMaterials::Flame): the body of the custom node of M_UghFlame, which UghMakeAssets reads from
// this file. It plays a flipbook of a simulated flame (UghFlames: Columns x Rows frames, row by row, the flame's foot
// at the bottom of each). Its inputs: UV the card's coordinates (u across, v down from its top), Time (seconds), Seed
// (0 .. 1 of the card: each plays from its own frame), Facing (how squarely the camera sees the card, 0 .. 1),
// Flipbook (the texture), Columns, Rows, Rate (frames a second), Wind (-1 .. 1: the flame leans that way), Intensity.
// It returns the light the flame gives off (additive, unlit): two frames blended by the time between them, the
// flame leaning with the wind and shimmering a little, a card seen edge on fading out (the cards cross).

float frames = Columns * Rows;
float t = fmod(Time, 1000.0) * Rate + Seed * frames;
float f0 = fmod(floor(t), frames);
float f1 = fmod(f0 + 1, frames);
float up = 1 - UV.y;
float2 uv = UV;
uv.x -= Wind * up * up * 0.22;
uv.x += sin(up * 11.0 - Time * 6.0 + Seed * 40.0) * 0.008 * up;
uv = clamp(uv, 0.004, 0.996);   // never into the next frame
float2 cell = 1.0 / float2(Columns, Rows);
float2 uv0 = (float2(fmod(f0, Columns), floor(f0 / Columns)) + uv) * cell;
float2 uv1 = (float2(fmod(f1, Columns), floor(f1 / Columns)) + uv) * cell;
float3 light = lerp(Texture2DSample(Flipbook, FlipbookSampler, uv0).rgb,
	Texture2DSample(Flipbook, FlipbookSampler, uv1).rgb, frac(t));
return light * Intensity * saturate(Facing * 2.2 - 0.25);
