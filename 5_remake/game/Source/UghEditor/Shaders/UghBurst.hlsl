// The motion of a burst of an event (UghMaterials::Burst, Bits, Glint): the body of the custom node that moves its
// particles (world position offset), which UghMakeAssets reads from this file after UghBurstSpin.hlsl. The mesh
// (AUghEffects) is a cloud of tiny quads at the burst's place: a quad's corner is UV (u across, v down, 0 .. 1), its
// middle the point CornerSize units back from its corner, Seed its own. Its inputs besides: Position (the vertex in
// the world, cm), CameraPosition, AgeLife (the node of UghMakeAssets) and the parameters of UghMaterials::Burst. It
// returns how far the vertex moves: each particle thrown from within Box along a cone around Direction, slowed by
// the air, falling (or rising), rocking aside, resting on Floor; turned to the camera (and stretched along its way),
// lying flat or tumbling; a point before and after its life.

const float CornerSize = 1;
float2 corner = UV - 0.5;
float3 origin = Position - float3(corner.x, 0, -corner.y) * CornerSize;
float age = AgeLife.x;
if (age <= 0 || age >= 1)
{
	return origin - Position;
}
float3 dir = normalize(Direction + float3(0, 0, 1e-4));
float3 side = abs(dir.z) < 0.99 ? normalize(cross(dir, float3(0, 0, 1))) : float3(1, 0, 0);
float3 up = cross(side, dir);
float cosA = 1 - Spread * h.x;
float sinA = sqrt(saturate(1 - cosA * cosA));
float phi = h.y * 6.2832;
float3 v = (dir * cosA + (side * cos(phi) + up * sin(phi)) * sinA) * Speed * sqrt(Scale) * (0.35 + 0.65 * h.z);
v.z *= Lift;
// linear drag: the way a unit of speed carries it by now, and the fall towards its terminal speed
float k = max(Drag, 0.05);
float e = (1 - exp(-k * t)) / k;
float3 start = (float3(h.w, frac(h.x * 13.3), frac(h.y * 17.9)) * 2 - 1) * Box * Scale;
float3 p = origin + start + v * e - float3(0, 0, Gravity) * (t - e) / k;
p.x += Flutter * Scale * sin(t * (2.5 + 2 * h.y) + h.z * 6.2832) * saturate(t * 2);
p.z = max(p.z, Floor);
float3 vel = v * exp(-k * t) - float3(0, 0, Gravity) * e;
float size = Size * Scale * lerp(1, Grow, age);
float3 toCamera = normalize(CameraPosition - p);
float c = cos(angle), s = sin(angle);
float3 offset;
if (Mode > 1.5)
{
	// tumbling about its own axis, shrinking away at its end (a cut-out bit cannot fade)
	float3 q = float3(corner.x, 0, -corner.y) * size * saturate((1 - age) * 8);
	offset = q * c + cross(axis, q) * s + axis * dot(axis, q) * (1 - c);
}
else if (Mode > 0.5)
{
	offset = float3(corner.x, corner.y, 0) * size;   // lying flat (a ring on the water)
}
else
{
	float3 along = vel - toCamera * dot(vel, toCamera);
	if (Stretch > 0 && length(along) > 1)
	{
		along = normalize(along);
		float3 across = normalize(cross(along, toCamera));
		offset = across * corner.x * size - along * corner.y * size * (1 + Stretch * length(vel) / 500);
	}
	else
	{
		// turned its own way, slowly turning on (Spin)
		float3 right = normalize(cross(toCamera, float3(0, 0, 1)));
		float3 upward = cross(right, toCamera);
		float2 r = float2(corner.x * c - corner.y * s, corner.x * s + corner.y * c);
		offset = (right * r.x - upward * r.y) * size;
	}
}
return p + offset - Position;
