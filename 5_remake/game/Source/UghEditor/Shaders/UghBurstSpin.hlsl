// What the motion of a burst's particle (UghBurst.hlsl) and the light of a tumbling bit (the normal of M_UghBits)
// share; UghMakeAssets puts it before both. From the particle's Seed (0 .. 1, the second UV) its own random numbers
// h, from AgeLife (how far it is in its life 0 .. 1, how long it lives in seconds) its time t, and its tumble: the
// axis and the angle a tumbling quad is turned by (Spin rad/s from its own start).

float4 h = frac(sin(float4(dot(Seed, float2(127.1, 311.7)), dot(Seed, float2(269.5, 183.3)),
	dot(Seed, float2(419.2, 371.9)), dot(Seed, float2(37.7, 91.3)))) * 43758.5453);
float t = AgeLife.x * AgeLife.y;
float3 axis = normalize(h.xyz - 0.5 + 0.001);
float angle = Spin * t + h.w * 6.2832;
