// The blower's snort (UghMaterials::Puff): the body of the custom node of M_UghPuff that moves its puffs (world
// position offset), which UghMakeAssets reads from this file. The mesh (FUghFigureModels) is a cloud of tiny quads at
// the blower's nostrils, each a puff of dust and breath: a quad's corner is UV (u across, v down, 0 .. 1), its middle
// the point CornerSize units back from its corner, Seed (0 .. 1, the second UV) its own. Its inputs besides: Position
// (the vertex in the world, cm), CameraPosition, Age (0 .. 1: how far the puff is in its life, the node of
// UghMakeAssets), Direction (the world's way the nostrils blow), Reach and Size (units). It returns how far the vertex
// moves: each puff shoots out along Direction, slowing down, fanning out aside and up, growing, turned to the camera.

const float CornerSize = 1;
float2 corner = UV - 0.5;
float3 middle = Position - float3(corner.x, 0, -corner.y) * CornerSize;
float3 along = normalize(Direction);
float3 side = normalize(cross(along, float3(0, 0, 1)));
float3 rise = cross(side, along);
float angle = Seed.x * 6.2832;
float far = Reach * (1 - (1 - Age) * (1 - Age)) * (0.6 + 0.4 * Seed.y);
float3 puff = middle + along * far + (side * cos(angle) + rise * (0.4 + 0.6 * sin(angle))) * Size * 0.45 * Age;
float size = Size * (0.2 + 0.8 * Age);
float3 toCamera = normalize(CameraPosition - puff);
float3 right = normalize(cross(toCamera, float3(0, 0, 1)));
float3 up = cross(right, toCamera);
return puff + (right * corner.x - up * corner.y) * size - Position;
