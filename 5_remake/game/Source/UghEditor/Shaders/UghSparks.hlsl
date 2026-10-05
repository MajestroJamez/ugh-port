// A fire's sparks (UghMaterials::Sparks): the body of the custom node of M_UghSparks that moves them (world position
// offset), which UghMakeAssets reads from this file. The mesh (UghFireParts::Sparks) is a cloud of tiny quads at a
// fire's foot, each a spark: a quad's corner is UV (u across, v down, 0 .. 1), its middle the point CornerSize units
// back from its corner (where the spark rises from), Seed (0 .. 1, the second UV) its own. Its inputs besides:
// Position (the vertex in the world, cm), CameraPosition, Age (0 .. 1: how far the spark is in its life, the node of
// UghMakeAssets), Wind (-1 .. 1), Rise (how high it flies, units), Spread (how far aside), Size (units). It returns
// how far the vertex moves: each spark shoots up out of the flame, slowing, swirling and drifting with the wind, a
// short streak along its way, turned to the camera, shrinking as it cools.

const float CornerSize = 1;
float2 corner = UV - 0.5;
float3 middle = Position - float3(corner.x, 0, -corner.y) * CornerSize;
float rise = Rise * (0.45 + 0.55 * Seed.y);
float height = rise * Age * (1.6 - 0.6 * Age);
float swirl = 6.0 + Seed.x * 5.0;
float3 spark = middle + float3(
	(Seed.x - 0.5) * Spread * Age + sin(Age * swirl + Seed.y * 30.0) * Spread * 0.25 * Age + Wind * height * 0.7,
	cos(Age * swirl * 0.8 + Seed.x * 20.0) * Spread * 0.25 * Age,
	height);
// along its way now (its speed: up, slowing, and aside)
float3 way = normalize(float3(Wind * 0.7 + cos(Age * swirl + Seed.y * 30.0) * 0.3, 0.001, 1.6 - 1.2 * Age));
float3 toCamera = normalize(CameraPosition - spark);
float3 across = normalize(cross(way, toCamera));
float size = Size * (1.0 - 0.6 * Age);
return spark + across * corner.x * size - way * corner.y * size * 3.0 - Position;
