// A fire's thin plume of smoke (UghMaterials::Smoke): the body of the custom node of M_UghSmoke that moves its puffs
// (world position offset), which UghMakeAssets reads from this file. The mesh (UghFireParts::Smoke) is a cloud of tiny
// quads at the tip of a flame, each a puff: a quad's corner is UV (u across, v down, 0 .. 1), its middle the point
// CornerSize units back from its corner (where the puff rises from), Seed (0 .. 1, the second UV) its own. Its inputs
// besides: Position (the vertex in the world, cm), CameraPosition, Age (0 .. 1: how far the puff is in its life),
// Wind (-1 .. 1), Rise (how high it goes, units), Size (units, when it is gone). It returns how far the vertex
// moves: each puff rises, wavering, drifting with the wind (more the higher), growing, turned to the camera.

const float CornerSize = 1;
float2 corner = UV - 0.5;
float3 middle = Position - float3(corner.x, 0, -corner.y) * CornerSize;
float height = Rise * Age * (0.75 + 0.25 * Seed.y);
float3 puff = middle + float3(
	sin(Age * 4.0 + Seed.x * 20.0) * Size * 0.25 * Age + (Seed.x - 0.5) * Size * 0.3 * Age + Wind * height * 1.2,
	cos(Age * 3.0 + Seed.y * 20.0) * Size * 0.2 * Age,
	height);
float size = Size * (0.2 + 0.8 * Age);
float3 toCamera = normalize(CameraPosition - puff);
float3 right = normalize(cross(toCamera, float3(0, 0, 1)));
float3 up = cross(right, toCamera);
return puff + (right * corner.x - up * corner.y) * size - Position;
