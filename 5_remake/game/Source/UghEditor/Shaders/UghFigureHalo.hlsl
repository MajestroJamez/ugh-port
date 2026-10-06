// The figures' halo (UghMaterials::FigureHalo): the body of the custom node of the post-process M_UghFigureHalo, which
// UghMakeAssets reads from this file. Its inputs: Radius (how far the halo reaches, of the view's height), Darken (how
// much the background darkens at most, 0 .. 1); Color and Figures are the scene textures it fetches (the scene's
// colour, the custom depth the figures are drawn into: UghFigureLook), wired only so that the material binds them.
// It returns the scene's colour.
//
// Around a figure (a pixel near what the custom depth has, but not the figure itself) the background darkens, the
// more the nearer the figure, unless the background is dark already: a rock face gets a soft dark halo behind the
// figure, a dark cave none. A figure partly behind a plant keeps its halo around what is seen of it.

float3 color = SceneTextureFetch(PPI_PostProcessInput0, float2(0, 0)).rgb;
float figure = SceneTextureFetch(PPI_CustomDepth, float2(0, 0)).r;
// the custom depth is far beyond the stage (its sky) where no figure is drawn
const float Nothing = 500000;
float pixels = Radius * GetSceneTextureViewSize(PPI_CustomDepth).y;   // in the custom depth's texels
[branch] if (figure < Nothing || pixels < 0.5 || Darken <= 0)
{
	return color * View.OneOverPreExposure;   // the figure (the engine multiplies what this returns by it)
}
// three rings of eight taps around (each turned a third of a step: no gap a thin arm slips through), the inner
// ones weighing more
float cover = 0, weight = 0, closest = 0;
[unroll] for (int i = 0; i < 24; ++i)
{
	float ring = i / 8;   // 0 the outer
	float a = i * 0.785398 + ring * 0.261799;
	float w = 1 + 0.3 * ring;
	float2 offset = float2(cos(a), sin(a)) * pixels * (1 - 0.35 * ring);
	weight += w;
	[branch] if (SceneTextureFetch(PPI_CustomDepth, offset).r < Nothing)
	{
		cover += w;
		closest = max(closest, 0.35 + 0.325 * ring);   // the outer ring a third, the inner one all
	}
}
[branch] if (cover == 0)
{
	return color * View.OneOverPreExposure;
}
// how near the figure is: the nearest ring it reaches, more where it covers more (a thin arm, a whole body)
float near = saturate(closest * 0.75 + cover / weight * 2);
// not where the background is dark already (a cave behind the figure; the colour is as the exposure makes it, mid
// grey about 0.18)
float need = smoothstep(0.02, 0.1, Luminance(color));
return color * (1 - Darken * near * need) * View.OneOverPreExposure;
