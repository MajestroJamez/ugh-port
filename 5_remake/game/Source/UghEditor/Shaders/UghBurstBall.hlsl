// The normal of a translucent puff of a burst (UghMaterials::Burst, the world's), which UghMakeAssets reads from this
// file after UghBurstSpin.hlsl: a puff of dust, smoke or spray is lit as a soft ball, not as the flat quad it is drawn
// on - its side towards a light bright, the far side darker, whichever way the light comes (the sun from above, a
// fire below, a flash). Inputs: UV (the quad, 0 .. 1), Seed, AgeLife, Spin (its turn, as UghBurst.hlsl turns the
// quad), Mode (0 turned to the camera, 1 lying flat: up) and ToCamera (from the pixel to the camera).

if (Mode > 0.5)
{
	return float3(0, 0, 1);   // lying flat on the water
}
float3 toCamera = normalize(ToCamera);
float3 right = normalize(cross(toCamera, float3(0, 0, 1)));
float3 upward = cross(right, toCamera);
float2 corner = UV - 0.5;
float c = cos(angle), s = sin(angle);
float2 r = float2(corner.x * c - corner.y * s, corner.x * s + corner.y * c) * 2;   // as the quad was turned
float d = saturate(dot(r, r));
// a ball, a little flattened towards the camera (soft: a puff has no hard rim)
return normalize((right * r.x - upward * r.y) * 0.8 + toCamera * (sqrt(1 - d) + 0.3));
