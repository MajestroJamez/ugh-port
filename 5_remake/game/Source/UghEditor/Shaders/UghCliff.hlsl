// The cliff's surface (UghMaterials::Cliff): the body of the custom node of M_UghCliff, which UghMakeAssets reads from
// this file. Its inputs: Position (world, cm), VertexNormal (world), ScreenUV (the screen of the original, 0 .. 1),
// Shade the vertex colour (r how open the surface is, g how deep behind the slab of the play, b how near below a top
// edge of the rock, a its large patches), Art the drawing of the level (softened; its alpha 1 - the paths trodden to the
// cave entrances, FUghTurf), WaterLevel (the world's z of the
// water's surface, cm) and for each layer (Rock, Stone, Grass, Moss, Soil) its maps <Layer>BaseColor, <Layer>Normal,
// <Layer>Roughness (green), <Layer>Height (its relief: the channels of <Layer>HeightMask) and <Layer>Size (metres one
// texture covers). It returns the base colour; CliffNormal (world), CliffRough and CliffOcclusion are its other
// outputs.
//
// The whole stone is one boulder of dark grey slate (step 24e): its strata dip to the right by the stone's dip (the
// same as the beds of FUghRockField and FUghStackField, UghRockNoise::StrataDip), so the maps are seen from the three
// axes of the strata's frame (triplanar: nothing stretches, the plates of the scanned rock lie along the strata on every
// wall) and blended by the way the surface faces, only the projections and layers that count are sampled. The layers:
// grass on what faces up and along the top edges of the face, moss here and there - hanging below the grass, in deep
// crevices, in a few of the large patches, where the drawing is green, on the little ledges and slopes, on deep floors
// and at the tide line -; soil in the crevices of what faces up, under the grass's ragged edge, in bare patches and on the
// paths, stones lying in it; the grass lush, olive or dry by its patches (step 30b); else the slate (rock and stone, the same scanned plates at
// two sizes; the rock's at two scales the large patches choose between, so that its tiles do not repeat visibly); where
// two meet, the higher relief of the two wins (height blend). The slate is greyed to a dark blue grey (darker than the
// figures, so that they stand out of it), each of its beds its own shade and its flakes along them too, dark seams
// between the beds, the gaps between its plates deep and dark, thin white quartz veins along the strata (in pieces,
// wandering, a few crossing them) and pale wisps; matte; wet in a band over the water following its surface (step 30d); the drawing
// shades it a little, so that each level keeps its light and dark areas.

#define UGH_WRAP View.MaterialTextureBilinearWrapedSampler
#define UGH_HASH(P) frac(sin(dot(P, float2(127.1, 311.7))) * 43758.5453)
#define UGH_NOISE(P, R) { float2 i_ = fmod(floor(P), 289.0); float2 f_ = frac(P); float2 s_ = f_ * f_ * (3 - 2 * f_); \
	R = lerp(lerp(UGH_HASH(i_), UGH_HASH(i_ + float2(1, 0)), s_.x), \
		lerp(UGH_HASH(i_ + float2(0, 1)), UGH_HASH(i_ + float2(1, 1)), s_.x), s_.y); }
// the strata's frame: turned about the world's y (the depth) so that its z is across the strata (the dip: 0.14, the
// tangent of the angle; cos, sin)
const float dipC = 0.990338, dipS = 0.138647;
#define UGH_ROT(A) float3((A).x * dipC + (A).z * dipS, (A).y, (A).z * dipC - (A).x * dipS)
#define UGH_UNROT(A) float3((A).x * dipC - (A).z * dipS, (A).y, (A).z * dipC + (A).x * dipS)

float3 n = normalize(VertexNormal);
float3 m = Position * 0.01;
float3 nr = UGH_ROT(n), mr = UGH_ROT(m);

// the projections in the strata's frame: their weights, coordinates (v down the picture) and the frame's directions of
// u and v
float3 w = pow(abs(nr), 4);
w /= w.x + w.y + w.z;
w *= step(0.03, w);
w /= w.x + w.y + w.z;
float3 s = step(0, nr) * 2 - 1;
float2 uvX = float2(mr.y * s.x, -mr.z), uvY = float2(-mr.x * s.y, -mr.z), uvZ = float2(mr.x, -mr.y * s.z);
float3 uX = float3(0, s.x, 0), uY = float3(-s.y, 0, 0), uZ = float3(1, 0, 0);
float3 vX = float3(0, 0, -1), vY = float3(0, 0, -1), vZ = float3(0, -s.z, 0);
float2 dxX = ddx(uvX), dyX = ddy(uvX), dxY = ddx(uvY), dyY = ddy(uvY), dxZ = ddx(uvZ), dyZ = ddy(uvZ);

// map T sampled along axis P (X, Y, Z) at K repeats per metre shifted by O; the three blended; a normal map's three in
// the strata's frame
#define UGH_SAMPLE(T, P, K, O) Texture2DSampleGrad(T, UGH_WRAP, uv##P * (K) + (O), dx##P * (K), dy##P * (K))
#define UGH_TRI(T, K, O, R) R = 0; \
	[branch] if (w.x > 0) R += w.x * UGH_SAMPLE(T, X, K, O); \
	[branch] if (w.y > 0) R += w.y * UGH_SAMPLE(T, Y, K, O); \
	[branch] if (w.z > 0) R += w.z * UGH_SAMPLE(T, Z, K, O);
#define UGH_TRI_NORMAL(T, K, O, R) R = 0; \
	[branch] if (w.x > 0) { float2 t = UGH_SAMPLE(T, X, K, O).xy * 2 - 1; R += w.x * (t.x * uX + t.y * vX); } \
	[branch] if (w.y > 0) { float2 t = UGH_SAMPLE(T, Y, K, O).xy * 2 - 1; R += w.y * (t.x * uY + t.y * vY); } \
	[branch] if (w.z > 0) { float2 t = UGH_SAMPLE(T, Z, K, O).xy * 2 - 1; R += w.z * (t.x * uZ + t.y * vZ); }

// what the drawing says: green (grass, vines), warm (its rock) or grey (its cave walls), and how light
float4 artA = Texture2DSampleLevel(Art, View.MaterialTextureBilinearClampedSampler, ScreenUV, 0);
float3 art = artA.rgb;
// its alpha: 1 - the paths trodden to the cave entrances (FUghTurf)
float path = 1 - artA.a;
float lum = max(dot(art, float3(0.3, 0.59, 0.11)), 0.02);
float green = saturate((art.g - max(art.r, art.b)) / lum * 4);
float warm = saturate((art.r - art.b) / lum * 2.5);
float open = Shade.r, deep = Shade.g, patches = Shade.a;
float back = smoothstep(0.25, 0.6, deep);   // the cave's back wall: darker
// below a top edge, on what faces the camera or up (not the sides of the slab of the play)
float lip = Shade.b * saturate(max(n.y, n.z) * 3);
// wet (step 30d): the band the sea washes follows its surface as it rises - under it, a film of water just above it
// (glossy, darkest; the waves wash up and down it), above that wet rock up to 0.5 .. 1.6 m (higher in some of the large
// patches, in streaks running down from its top), a dark tide line of algae at the surface
float above = m.z - WaterLevel * 0.01;   // metres above the surface
float streaks, washing;
UGH_NOISE(float2(m.x * 9, m.z * 0.8), streaks);
UGH_NOISE(float2(m.x * 0.9 + 4.1, 1.3), washing);
float bandTop = 0.45 + 0.9 * patches + 0.6 * (streaks - 0.5);
float wet = 1 - smoothstep(0.55 * bandTop, bandTop, above);
float wash = 0.14 + 0.1 * sin(View.GameTime * 0.9 + m.x * 0.4 + washing * 6);
float film = 1 - smoothstep(0.4 * wash, wash, above);
float tide = (1 - smoothstep(0.04, 0.3, above)) * smoothstep(-0.4, -0.05, above);
// moss only here and there: in the deep crevices of the face, on its little ledges and the slopes of the cave, in a few
// of the large patches
float mossy = max(max(0.7 * smoothstep(0.45, 0.12, open) * (1 - back),
	smoothstep(0.45, 0.75, n.z) * smoothstep(0.45, 0.7, 1 - patches) * 0.8),
	smoothstep(0.72, 0.84, 1 - patches) * 0.8);

// the turf (step 30b): the grass's patches lush, olive and dry (the same as FUghTurf's blades, UghTurf.hlsl), its edge
// over the face ragged - tongues hanging further here, less there, in strands - with a band of earth and roots under
// it; bare patches of earth on the tops; on the paths trodden to the cave entrances bare packed soil, the edge worn
float q1, q2, ragged, strands, bare;
UGH_NOISE(float2(m.x * 0.45, m.z * 0.35) + 3.7, q1);
UGH_NOISE(float2(m.x * 1.9, m.z * 1.3) + 11.3, q2);
UGH_NOISE(float2(m.x * 3.1, m.z * 0.7) + 23.1, ragged);
UGH_NOISE(float2(m.x * 23, m.z * 1.5) + 5.9, strands);
UGH_NOISE(float2(m.x * 2.3, m.y * 2.3) + 31.7, bare);
float dry = saturate(smoothstep(0.3, 0.8, 0.7 * q1 + 0.3 * q2) + 0.4 * path);
float edge = lip + 0.35 * (ragged - 0.5) + 0.18 * (strands - 0.5) + 0.3 * (patches - 0.5);
float grassLip = smoothstep(0.42, 0.55, edge) * (1 - smoothstep(0.2, 0.7, path)) * smoothstep(0.3, 0.6, n.z);
float tops = smoothstep(0.75, 0.92, n.z);
float worn = smoothstep(0.15, 0.6, path) * max(tops, smoothstep(0.1, 0.3, lip));
float bareTop = smoothstep(0.66, 0.78, bare) * tops * (1 - deep);
float soilBand = saturate(max(smoothstep(0.22, 0.36, edge + 0.15 * (ragged - 0.5)) * (1 - grassLip), worn) + bareTop);

// the layers' shares: Rock, Stone, Grass, Moss, Soil
float W[5];
W[2] = max(tops * (1 - 0.6 * deep) * (1 - worn) * (1 - bareTop), grassLip);
W[3] = saturate(max(max(max(green * 0.8, smoothstep(0.3, 0.7, n.z) * deep), mossy),
	max(smoothstep(0.0, 0.25, lip), tide * smoothstep(0.45, 0.75, patches))) - W[2] - soilBand);
W[4] = max(smoothstep(0.45, 0.75, n.z + 0.25 * (1 - open)) * 0.8, soilBand) * saturate(1 - W[2] - W[3]);
float rest = saturate(1 - W[2] - W[3] - W[4]);
W[1] = rest * saturate(max(1 - warm, back) + 0.8 * smoothstep(0.5, 0.72, patches));
W[0] = rest - W[1];

// the rock's slate at two scales: where the large patches (and the reliefs) say, the one or the other
float kRock = 1 / RockSize, kRock2 = 0.71 / RockSize;
float2 shift = float2(0.37, 0.61);
float swap = 0;
// where two layers meet, the one whose relief is higher there
float H[5] = { 0, 0, 0, 0, 0 };
float4 h, h2;
[branch] if (W[0] > 0.005)
{
	UGH_TRI(RockHeight, kRock, 0, h);
	UGH_TRI(RockHeight, kRock2, shift, h2);
	float a = dot(h.rgb, RockHeightMask.rgb), b = dot(h2.rgb, RockHeightMask.rgb);
	swap = saturate((patches - 0.5) * 5 + (b - a) * 2 + 0.5);
	H[0] = lerp(a, b, swap);
}
#define UGH_HEIGHT(I, Layer) [branch] if (W[I] > 0.005) { UGH_TRI(Layer##Height, 1 / Layer##Size, 0, h); \
	H[I] = dot(h.rgb, Layer##HeightMask.rgb); }
UGH_HEIGHT(1, Stone)
UGH_HEIGHT(2, Grass)
UGH_HEIGHT(3, Moss)
UGH_HEIGHT(4, Soil)
float top = 0;
[unroll] for (int i = 0; i < 5; ++i) { top = max(top, W[i] > 0.005 ? W[i] + H[i] : 0); }
float sum = 0, relief = 0, slateRelief = 0;
[unroll] for (int j = 0; j < 5; ++j)
{
	W[j] = W[j] > 0.005 ? max(W[j] + H[j] - top + 0.25, 0) : 0;
	sum += W[j];
	relief += W[j] * H[j];
}
relief /= sum;
slateRelief = (W[0] * H[0] + W[1] * H[1]) / max(W[0] + W[1], 1e-4);

// the slate's colour apart from the other layers' (it is greyed on its own)
float3 slate = 0, base = 0, bump = 0;
float rough = 0;
float4 c, r;
float3 nn;
#define UGH_MAPS(Layer, K, O, Share, Into) UGH_TRI(Layer##BaseColor, K, O, c); UGH_TRI_NORMAL(Layer##Normal, K, O, nn); \
	UGH_TRI(Layer##Roughness, K, O, r); Into += (Share) * c.rgb; bump += (Share) * nn; rough += (Share) * r.g;
[branch] if (W[0] > 0)
{
	[branch] if (swap < 0.99) { UGH_MAPS(Rock, kRock, 0, W[0] * (1 - swap), slate) }
	[branch] if (swap > 0.01) { UGH_MAPS(Rock, kRock2, shift, W[0] * swap, slate) }
}
[branch] if (W[1] > 0) { UGH_MAPS(Stone, 1 / StoneSize, 0, W[1], slate) }
#define UGH_LAYER(I, Layer) [branch] if (W[I] > 0) { UGH_MAPS(Layer, 1 / Layer##Size, 0, W[I], base) }
// the grass lush, olive or dry by its patches, lighter and darker; the soil of a path packed, paler
float3 grassColor = 0, soilColor = 0;
[branch] if (W[2] > 0) { UGH_MAPS(Grass, 1 / GrassSize, 0, W[2], grassColor) }
{
	float grassLum = dot(grassColor, float3(0.3, 0.59, 0.11));
	float3 olive = grassLum * float3(1.4, 1.25, 0.5), straw = grassLum * float3(2.3, 1.7, 0.65);
	grassColor = dry < 0.5 ? lerp(grassColor, olive, dry * 2) : lerp(olive, straw, dry * 2 - 1);
	base += grassColor * lerp(0.75, 1.2, q2);
}
UGH_LAYER(3, Moss)
[branch] if (W[4] > 0) { UGH_MAPS(Soil, 1 / SoilSize, 0, W[4], soilColor) }
base += soilColor * lerp(1, float3(1.3, 1.2, 1.05), worn) * lerp(1, 0.55, saturate(lip * 1.5) * (1 - tops) * (1 - worn));
// the slate's finer plates on all of it
float rockShare = (W[0] + W[1]) / sum;
[branch] if (rockShare > 0.05)
{
	UGH_TRI_NORMAL(StoneNormal, 3.3 / StoneSize, shift, nn);
	bump += 0.6 * rockShare * sum * nn;
}
bump /= sum;
rough /= sum;

// the slate (on every pixel, not in a branch: its derivatives want the neighbours)
{
	// dark blue grey slate: the scanned plates' own light and dark (more contrast), darker than the figures
	float grey = dot(slate / max(W[0] + W[1], 1e-4), float3(0.3, 0.59, 0.11));
	float3 tone = 0.055 * pow(max(grey / 0.032, 0), 1.5) * float3(0.86, 0.97, 1.16);
	// along the strata (metres across them, undulating): thick beds and thin ones in them, each its own shade, a
	// little browner or bluer; the flakes along a thin bed a little apart; all fading where too fine to see
	float2 strata = float2(mr.x + mr.y, mr.z);   // along the strata (on any wall), across them
	float wave, wave2;
	UGH_NOISE(strata * float2(0.35, 0.5), wave);
	UGH_NOISE(strata * float2(1.3, 1.6) + 17, wave2);
	float thick = mr.z / 1.7 + 0.8 * wave;
	float thin = mr.z / 0.32 + 2.2 * wave + 0.7 * wave2;
	float thickBed = floor(thick), thinBed = floor(thin), inThin = frac(thin);
	float flake = floor(strata.x / (0.5 + 1.2 * UGH_HASH(float2(thinBed, 3))) + 9 * UGH_HASH(float2(thinBed, 5)));
	float fineThick = saturate(1 - fwidth(thick) * 3), fineThin = saturate(1 - fwidth(thin) * 2.5);
	float bedShade = lerp(1, lerp(0.7, 1.3, UGH_HASH(float2(thickBed, 11))), fineThick) *
		lerp(1, lerp(0.82, 1.15, UGH_HASH(float2(thinBed, 13))) * lerp(0.9, 1.1, UGH_HASH(float2(flake, thinBed))),
			fineThin);
	float hue = UGH_HASH(float2(thickBed, 19));
	tone *= bedShade * lerp(1, lerp(float3(1.05, 1.0, 0.93), float3(0.95, 1.0, 1.06), hue), fineThick);
	// the seam at the foot of a thin bed: dark, open in some of its flakes only
	float seamWidth = 0.01 + fwidth(thin) * 0.32;
	float seam = (1 - smoothstep(0.004, seamWidth, min(inThin, 1 - inThin) * 0.32)) *
		step(0.35, UGH_HASH(float2(flake, thinBed + 0.5))) * 0.01 / seamWidth;
	tone *= 1 - 0.65 * saturate(seam);
	// pale wisps along the strata (mica, quartz dust)
	float wisp;
	UGH_NOISE(strata * float2(1.2, 16), wisp);
	tone *= 1 + 0.45 * smoothstep(0.68, 0.95, wisp) * saturate(1 - fwidth(strata.y * 16) * 0.8);
	// the gaps between the plates deep and dark, the plates' faces a little lighter
	tone *= lerp(0.4, 1.12, smoothstep(0.4, 0.66, slateRelief));
	// thin white quartz veins: along the strata, wandering, in pieces; a few crossing them
	float meander, meander2, width, pieces, crossing;
	UGH_NOISE(strata * float2(0.6, 0.9) + 41, meander);
	UGH_NOISE(strata * float2(6.5, 5) + 7, meander2);
	UGH_NOISE(strata * 1.7 + 63, width);
	float veinAcross = mr.z / 0.85 + 1.4 * meander + 0.12 * meander2;
	UGH_NOISE(float2(strata.x * 2.6, floor(veinAcross) * 3.7), pieces);
	float veinBlur = fwidth(veinAcross) * 0.85;
	float veinWidth = 0.002 + 0.006 * width;
	float vein = (1 - smoothstep(veinWidth, veinWidth + veinBlur + 0.002, abs(frac(veinAcross) - 0.5) * 0.85)) *
		smoothstep(0.5, 0.62, pieces) * veinWidth / (veinWidth + veinBlur);
	float crossAcross = (strata.x * 0.8 + strata.y * 0.6) / 2.3 + 0.8 * meander2;
	UGH_NOISE(float2(strata.x * 0.6 - strata.y * 0.8, floor(crossAcross) * 5.3) * 1.3, crossing);
	float crossBlur = fwidth(crossAcross) * 2.3;
	vein = max(vein, (1 - smoothstep(veinWidth * 0.6, veinWidth * 0.6 + crossBlur + 0.002,
		abs(frac(crossAcross) - 0.5) * 2.3)) * smoothstep(0.8, 0.9, crossing) * veinWidth / (veinWidth + crossBlur));
	tone = lerp(tone, float3(0.55, 0.57, 0.6) * lerp(0.7, 1.1, meander2), saturate(1.4 * vein));
	base += (W[0] + W[1]) * tone;
}
base /= sum;
// stones lying in the earth (of the band under the grass, the bare patches, the paths): a few in each cell of 14 cm,
// grey brown, each its own shade, a dark rim
{
	float soilShare = W[4] / sum;
	float2 cellAt = float2(m.x, lerp(m.z, m.y, saturate(n.z))) * 7;
	float2 cell = floor(cellAt), inCell = frac(cellAt) - 0.5;
	float2 middle = (float2(UGH_HASH(cell + 1.7), UGH_HASH(cell + 5.3)) - 0.5) * 0.4;
	float hp = UGH_HASH(cell + 0.37);
	float radius = lerp(0.14, 0.38, hp) * step(0.4, UGH_HASH(cell + 9.1));
	float d = length(inCell - middle);
	float stone = smoothstep(radius, radius * 0.75, d) * smoothstep(0.15, 0.5, soilShare);
	float rim = smoothstep(radius * 1.3, radius, d) * (1 - stone) * smoothstep(0.15, 0.5, soilShare);
	base = lerp(base * (1 - 0.45 * rim), float3(0.085, 0.08, 0.072) * lerp(0.55, 1.5, hp), stone);
}
// large patches lighter and darker, warmer and greyer
base *= lerp(0.85, 1.15, patches) * lerp(float3(0.98, 0.99, 1.02), float3(1.02, 1.0, 0.98), patches);
// crevices of the rock and hollows of the relief darker
base *= lerp(0.35, 1, open) * lerp(0.75, 1.05, saturate(relief * 1.5));
// the drawing's lightness a little, deep in the cave darker
base *= lerp(1, saturate(lum * 2.5), 0.1) * lerp(1, 0.5, back);
// matte: dry rock never glossy; wet: darker, glossy, the film of water glossiest; the tide line dark green-brown
rough = max(rough, 0.78 * rockShare);
base *= lerp(1, 0.33, wet) * lerp(1, 0.75, film);
base = lerp(base, float3(0.02, 0.024, 0.016), 0.5 * tide);
rough = lerp(lerp(rough, 0.26, 0.85 * wet), 0.07, film);

CliffNormal = normalize(n + 1.8 * UGH_UNROT(bump));
CliffRough = rough;
CliffOcclusion = lerp(0.3, 1, open) * lerp(0.75, 1, saturate(relief * 1.5)) *
	lerp(1, lerp(0.55, 1, smoothstep(0.4, 0.66, slateRelief)), rockShare);
return base;
