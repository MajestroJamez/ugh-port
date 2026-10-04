// The cliff's surface (UghMaterials::Cliff): the body of the custom node of M_UghCliff, which UghMakeAssets reads from
// this file. Its inputs: Position (world, cm), VertexNormal (world), ScreenUV (the screen of the original, 0 .. 1),
// Shade the vertex colour (r how open the surface is, g how deep behind the slab of the play), Art the drawing of the
// level (softened), and for each layer (Rock, Stone, Grass, Moss, Soil) its maps <Layer>BaseColor, <Layer>Normal,
// <Layer>Roughness (green), <Layer>Height (its relief: the channels of <Layer>HeightMask) and <Layer>Size (metres one
// texture covers). It returns the base colour; CliffNormal (world), CliffRough and CliffOcclusion are its other
// outputs.
//
// Each map is seen from the three axes and blended by the way the surface faces (triplanar: nothing stretches), only
// the projections and layers that count are sampled. The layers: grass on what faces up, soil on slopes, moss where
// the drawing is green and on deep floors, else rock - warm where the drawing is warm (its rock), grey where it is
// grey (its cave walls); where two meet, the higher relief of the two wins (height blend). Large patches of it are
// lighter or darker (no tiles repeating), the drawing tints it all a little, so that each level keeps its colours; the
// warm rock is muted towards sandstone.

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

// map T sampled along axis P (X, Y, Z) at K repeats per metre; the three blended; a normal map's three in the world
#define UGH_SAMPLE(T, P, K) Texture2DSampleGrad(T, UGH_WRAP, uv##P * (K), dx##P * (K), dy##P * (K))
#define UGH_TRI(T, K, R) R = 0; \
	[branch] if (w.x > 0) R += w.x * UGH_SAMPLE(T, X, K); \
	[branch] if (w.y > 0) R += w.y * UGH_SAMPLE(T, Y, K); \
	[branch] if (w.z > 0) R += w.z * UGH_SAMPLE(T, Z, K);
#define UGH_TRI_NORMAL(T, K, R) R = 0; \
	[branch] if (w.x > 0) { float2 t = UGH_SAMPLE(T, X, K).xy * 2 - 1; R += w.x * (t.x * uX + t.y * vX); } \
	[branch] if (w.y > 0) { float2 t = UGH_SAMPLE(T, Y, K).xy * 2 - 1; R += w.y * (t.x * uY + t.y * vY); } \
	[branch] if (w.z > 0) { float2 t = UGH_SAMPLE(T, Z, K).xy * 2 - 1; R += w.z * (t.x * uZ + t.y * vZ); }

// what the drawing says: green (grass, vines), warm (its rock) or grey (its cave walls), and how light
float3 art = Texture2DSampleLevel(Art, View.MaterialTextureBilinearClampedSampler, ScreenUV, 0).rgb;
float lum = max(dot(art, float3(0.3, 0.59, 0.11)), 0.02);
float green = saturate((art.g - max(art.r, art.b)) / lum * 4);
float warm = saturate((art.r - art.b) / lum * 2.5);
float open = Shade.r, deep = Shade.g;
float back = smoothstep(0.25, 0.6, deep);   // the cave's back wall: grey, darker

// the layers' shares: Rock, Stone, Grass, Moss, Soil
float W[5];
W[2] = smoothstep(0.55, 0.8, n.z) * (1 - 0.6 * deep);
W[3] = saturate(max(green * 0.8, smoothstep(0.3, 0.7, n.z) * deep) - W[2]);
W[4] = smoothstep(0.25, 0.55, n.z) * saturate(1 - W[2] - W[3]) * 0.8;
float rest = saturate(1 - W[2] - W[3] - W[4]);
W[0] = rest * warm * (1 - back);
W[1] = rest - W[0];

// where two meet, the one whose relief is higher there
float H[5] = { 0, 0, 0, 0, 0 };
float4 h;
#define UGH_HEIGHT(I, Layer) [branch] if (W[I] > 0.005) { UGH_TRI(Layer##Height, 1 / Layer##Size, h); \
	H[I] = dot(h.rgb, Layer##HeightMask.rgb); }
UGH_HEIGHT(0, Rock)
UGH_HEIGHT(1, Stone)
UGH_HEIGHT(2, Grass)
UGH_HEIGHT(3, Moss)
UGH_HEIGHT(4, Soil)
float top = 0;
[unroll] for (int i = 0; i < 5; ++i) { top = max(top, W[i] > 0.005 ? W[i] + H[i] : 0); }
float sum = 0;
[unroll] for (int j = 0; j < 5; ++j) { W[j] = W[j] > 0.005 ? max(W[j] + H[j] - top + 0.25, 0) : 0; sum += W[j]; }

float3 base = 0, bump = 0;
float rough = 0;
float4 c, r;
float3 nn;
#define UGH_LAYER(I, Layer) [branch] if (W[I] > 0) { float k = 1 / Layer##Size; \
	UGH_TRI(Layer##BaseColor, k, c); UGH_TRI_NORMAL(Layer##Normal, k, nn); UGH_TRI(Layer##Roughness, k, r); \
	base += W[I] * c.rgb; bump += W[I] * nn.xyz; rough += W[I] * r.g; }
UGH_LAYER(0, Rock)
UGH_LAYER(1, Stone)
UGH_LAYER(2, Grass)
UGH_LAYER(3, Moss)
UGH_LAYER(4, Soil)
base /= sum;
bump /= sum;
rough /= sum;

// large patches lighter and darker (the rock's relief seen eight times larger), so its tiles do not repeat visibly
UGH_TRI(RockHeight, 1 / (RockSize * 8), h);
base *= lerp(0.75, 1.25, saturate(dot(h.rgb, RockHeightMask.rgb)));
// the drawing's hue and lightness a little, deep in the cave a little darker
float3 hue = art / lum;
base *= lerp(1, hue, 0.25) * lerp(1, saturate(lum * 2.5), 0.15) * lerp(1, 0.55, back);
// the warm rock muted towards sandstone (as the scanned cliffs of the cave are)
base = lerp(base, dot(base, float3(0.3, 0.59, 0.11)) * float3(1.3, 1.0, 0.68), 0.8 * W[0] / sum);
CliffNormal = normalize(n + bump);
CliffRough = rough;
CliffOcclusion = lerp(0.5, 1, open);
return base;
