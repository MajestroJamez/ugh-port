// The cliff's surface (UghMaterials::Cliff): the body of the custom node of M_UghCliff, which UghMakeAssets reads from
// this file. Its inputs: Position (world, cm), VertexNormal (world), ScreenUV (the screen of the original, 0 .. 1),
// Shade the vertex colour (r how open the surface is, g how deep behind the slab of the play, b how near below a top
// edge of the rock, a its large patches), Art the drawing of the level (softened), WaterLevel (the world's z of the
// water's surface, cm) and for each layer (Rock, Stone, Grass, Moss, Soil) its maps <Layer>BaseColor, <Layer>Normal,
// <Layer>Roughness (green), <Layer>Height (its relief: the channels of <Layer>HeightMask) and <Layer>Size (metres one
// texture covers). It returns the base colour; CliffNormal (world), CliffRough and CliffOcclusion are its other
// outputs.
//
// Each map is seen from the three axes and blended by the way the surface faces (triplanar: nothing stretches), only
// the projections and layers that count are sampled. The layers: grass on what faces up and along the top edges of
// the face, moss hanging below it, in the crevices and hollows, in some of the large patches, where the drawing is
// green, on the little ledges and slopes, on deep floors and in the wet; soil in the crevices of what faces up; else
// rock - the limestone of fractured blocks where the drawing is warm (its rock), with patches of the grey stone with
// lichen, that stone where the drawing is grey (its cave walls) and deep in the cave; where two meet, the higher
// relief of the two wins (height blend). The limestone is sampled at two scales, the large patches choosing between
// them, so that its tiles do not repeat visibly; the grey stone's relief, finer, lies on all the rock. The limestone
// is greyed (weathered karst: mid grey, a little warm, dark streaks down it), the rock matte; crevices and the
// hollows of the relief are darker, the large patches lighter and darker, warmer and greyer, the rock just at the
// water and under it wet; the drawing shades it a little, so that each level keeps its light and dark areas.

#define UGH_WRAP View.MaterialTextureBilinearWrapedSampler
float3 n = normalize(VertexNormal);
float3 m = Position * 0.01;

// the projections: their weights, coordinates (v down the picture) and the world's directions of u and v
float3 w = pow(abs(n), 4);
w /= w.x + w.y + w.z;
w *= step(0.03, w);
w /= w.x + w.y + w.z;
float3 s = step(0, n) * 2 - 1;
float2 uvX = float2(m.y * s.x, -m.z), uvY = float2(-m.x * s.y, -m.z), uvZ = float2(m.x, -m.y * s.z);
float3 uX = float3(0, s.x, 0), uY = float3(-s.y, 0, 0), uZ = float3(1, 0, 0);
float3 vX = float3(0, 0, -1), vY = float3(0, 0, -1), vZ = float3(0, -s.z, 0);
float2 dxX = ddx(uvX), dyX = ddy(uvX), dxY = ddx(uvY), dyY = ddy(uvY), dxZ = ddx(uvZ), dyZ = ddy(uvZ);

// map T sampled along axis P (X, Y, Z) at K repeats per metre shifted by O; the three blended; a normal map's three in
// the world
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
float3 art = Texture2DSampleLevel(Art, View.MaterialTextureBilinearClampedSampler, ScreenUV, 0).rgb;
float lum = max(dot(art, float3(0.3, 0.59, 0.11)), 0.02);
float green = saturate((art.g - max(art.r, art.b)) / lum * 4);
float warm = saturate((art.r - art.b) / lum * 2.5);
float open = Shade.r, deep = Shade.g, patches = Shade.a;
float back = smoothstep(0.25, 0.6, deep);   // the cave's back wall: grey, darker
// below a top edge, on what faces the camera or up (not the sides of the slab of the play)
float lip = Shade.b * saturate(max(n.y, n.z) * 3);
// wet at the water and up to a metre above it (here more, there less), and under it
float wet = 1 - smoothstep(0, 0.3 + 0.7 * patches, m.z - WaterLevel * 0.01);
// moss: in the crevices and hollows of the face, on its little ledges and the slopes of the cave, in a part of the
// large patches
float mossy = max(max(smoothstep(0.55, 0.15, open) * (1 - back),
	smoothstep(0.45, 0.75, n.z) * smoothstep(0.4, 0.65, 1 - patches) * 0.8),
	smoothstep(0.62, 0.78, 1 - patches) * 0.85);

// the layers' shares: Rock, Stone, Grass, Moss, Soil
float W[5];
W[2] = max(smoothstep(0.75, 0.92, n.z) * (1 - 0.6 * deep), smoothstep(0.3, 0.65, lip + 0.3 * (patches - 0.5)));
W[3] = saturate(max(max(max(green * 0.8, smoothstep(0.3, 0.7, n.z) * deep), mossy),
	max(smoothstep(0.0, 0.25, lip), wet * smoothstep(0.45, 0.7, patches))) - W[2]);
W[4] = smoothstep(0.45, 0.75, n.z + 0.25 * (1 - open)) * saturate(1 - W[2] - W[3]) * 0.8;
float rest = saturate(1 - W[2] - W[3] - W[4]);
W[1] = rest * saturate(max(1 - warm, back) + 0.8 * smoothstep(0.5, 0.72, patches));
W[0] = rest - W[1];

// the limestone at two scales: where the large patches (and the reliefs) say, the one or the other
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
float sum = 0, relief = 0;
[unroll] for (int j = 0; j < 5; ++j)
{
	W[j] = W[j] > 0.005 ? max(W[j] + H[j] - top + 0.25, 0) : 0;
	sum += W[j];
	relief += W[j] * H[j];
}
relief /= sum;

float3 base = 0, bump = 0;
float rough = 0;
float4 c, r;
float3 nn;
#define UGH_MAPS(Layer, K, O, Share) UGH_TRI(Layer##BaseColor, K, O, c); UGH_TRI_NORMAL(Layer##Normal, K, O, nn); \
	UGH_TRI(Layer##Roughness, K, O, r); base += (Share) * c.rgb; bump += (Share) * nn; rough += (Share) * r.g;
[branch] if (W[0] > 0)
{
	[branch] if (swap < 0.99) { UGH_MAPS(Rock, kRock, 0, W[0] * (1 - swap)) }
	[branch] if (swap > 0.01) { UGH_MAPS(Rock, kRock2, shift, W[0] * swap) }
}
#define UGH_LAYER(I, Layer) [branch] if (W[I] > 0) { UGH_MAPS(Layer, 1 / Layer##Size, 0, W[I]) }
UGH_LAYER(1, Stone)
UGH_LAYER(2, Grass)
UGH_LAYER(3, Moss)
UGH_LAYER(4, Soil)
// the grey stone's relief, finer, on all the rock
float rock = (W[0] + W[1]) / sum;
[branch] if (rock > 0.05)
{
	UGH_TRI_NORMAL(StoneNormal, 3.3 / StoneSize, shift, nn);
	bump += 0.6 * rock * sum * nn;
}
base /= sum;
bump /= sum;
rough /= sum;

// weathered karst limestone: the blocks greyed to a mid, a little warm grey (darker than the figures, so that they
// stand out of it; more contrast: darker cracks and weathering), the stone with lichen a little greyer too
float grey = dot(base, float3(0.3, 0.59, 0.11));
float3 limestone = 0.16 * pow(max(grey / 0.25, 0), 1.4) * float3(1.04, 1.0, 0.92);
base = lerp(base, limestone, (W[0] + 0.3 * W[1]) / sum);
// large patches lighter and darker, warmer and greyer
base *= lerp(0.8, 1.2, patches) * lerp(float3(0.97, 0.99, 1.02), float3(1.03, 1.0, 0.96), patches);
// dark streaks of weathering down the limestone (where rain water runs): its relief stretched downwards
float streak = dot(Texture2DSampleLevel(RockHeight, UGH_WRAP, float2((m.x + m.y) * 0.3, m.z * 0.03), 3).rgb,
	RockHeightMask.rgb);
base *= lerp(1, 0.6, smoothstep(0.42, 0.25, streak) * W[0] / sum * (1 - wet));
// crevices of the rock and hollows of the relief darker
base *= lerp(0.35, 1, open) * lerp(0.65, 1.08, saturate(relief * 1.5));
// the drawing's lightness a little, deep in the cave darker
base *= lerp(1, saturate(lum * 2.5), 0.1) * lerp(1, 0.5, back);
// matte: dry rock never glossy; wet: darker, glossy
rough = max(rough, 0.75 * rock);
base *= lerp(1, 0.5, wet);
rough = lerp(rough, 0.3, 0.8 * wet);

CliffNormal = normalize(n + 1.8 * bump);
CliffRough = rough;
CliffOcclusion = lerp(0.3, 1, open) * lerp(0.75, 1, saturate(relief * 1.5));
return base;
