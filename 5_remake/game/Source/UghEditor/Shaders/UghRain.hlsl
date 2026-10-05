// The rain's streaks (UghMaterials::Rain): the body of the custom node of M_UghRain that moves them (world position
// offset), which UghMakeAssets reads from this file. The mesh (AUghRain) is a cloud of tiny quads, each a drop: a
// quad's corner is UV (u across, v down, 0 .. 1), its middle the point CornerSize units back from its corner,
// Seed.x (0 .. 1, the second UV) its own. Its inputs besides: Position (the vertex in the world, cm),
// CameraPosition, Time (seconds), Wind (-1 .. 1: the drops fall that way along x as much as down, as the logic's
// raindrops), WaterLevel (the world's z of the water's surface: a drop below it is gone), Top, Bottom, Left and Right
// (where the rain falls, the world's z and x: a drop falling out of it comes back in at the other side), Speed (cm/s
// along its way), Length and Width (of a streak, cm). It returns how far the vertex moves: each drop falls through
// the rain's box, a streak along its way turned towards the camera.

const float CornerSize = 1;
float2 corner = UV - 0.5;
float3 middle = Position - float3(corner.x, 0, -corner.y) * CornerSize;   // v down
float3 way = normalize(float3(Wind, 0, -1));
float height = max(Top - Bottom, 1.0), width = max(Right - Left, 1.0);

// how far down it has fallen since it was at the top, and as far with the wind
float fallen = frac((Top - middle.z) / height + fmod(Time, 1000.0) * Speed * -way.z / height + Seed.x) * height;
float3 drop = float3(middle.x + Wind * fallen, middle.y, Top - fallen);
drop.x = Left + frac((drop.x - Left) / width) * width;

float3 toCamera = normalize(CameraPosition - drop);
float3 across = normalize(cross(way, toCamera));
float3 vertex = drop + across * corner.x * Width + way * -corner.y * Length;
// under the water: gone (the quad shrinks to a point)
return (drop.z < WaterLevel ? drop : vertex) - Position;
