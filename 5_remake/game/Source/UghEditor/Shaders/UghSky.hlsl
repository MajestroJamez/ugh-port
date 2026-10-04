// The sky (UghMaterials::Sky): the body of the custom node of M_UghSky, which UghMakeAssets reads from this file. Its
// inputs: ToCamera the camera vector (world, from the dome to the camera), Sky the picture of the whole sky (long-lat:
// u around, v from the zenith down). It returns the sky's colour seen that way.

float3 d = -normalize(ToCamera);
float2 uv = float2(atan2(d.y, d.x) / (2 * PI) + 0.5, acos(clamp(d.z, -1, 1)) / PI);
return Texture2DSampleLevel(Sky, SkySampler, uv, 0).rgb;
