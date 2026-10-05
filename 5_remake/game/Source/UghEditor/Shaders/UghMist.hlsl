// The spray and mist of a waterfall's foot (UghMaterials::Mist): the body of the custom node of M_UghMist that moves
// its puffs (world position offset), which UghMakeAssets reads from this file. The mesh (AUghFalls) is a cloud of
// tiny quads on the sea's surface where the waterfall pours in, each a puff: a quad's corner is UV (u across, v down,
// 0 .. 1), its middle the point CornerSize units back from its corner, Seed (0 .. 1, the second UV) its own. Its inputs
// besides: Position (the vertex in the world, cm), CameraPosition, Age (0 .. 1: how far the puff is in its life, the
// node of UghMakeAssets), WaterLevel (the world's z of the surface now: the puffs rise from it whatever the height of
// their quads, which only give the mesh its bounds), Rise and Size (units). It returns how far the vertex moves: each
// puff rises from the surface up to Rise, drifting out towards the camera and a little to its side, growing from a
// third of Size, turned to the camera.

const float CornerSize = 1;
float2 corner = UV - 0.5;
float3 middle = Position - float3(corner.x, 0, -corner.y) * CornerSize;
// (world y is minus the depth: out towards the camera is +y)
float3 puff = float3(middle.xy, WaterLevel) + float3((Seed.x - 0.5) * Size * 0.5 * Age,
	Size * (0.3 + 0.7 * Seed.y) * Age, Rise * Age * (0.4 + 0.6 * Seed.y));
float size = Size * (0.35 + 0.9 * Age);
float3 toCamera = normalize(CameraPosition - puff);
float3 right = normalize(cross(toCamera, float3(0, 0, 1)));
float3 up = cross(right, toCamera);
return puff + (right * corner.x - up * corner.y) * size - Position;
