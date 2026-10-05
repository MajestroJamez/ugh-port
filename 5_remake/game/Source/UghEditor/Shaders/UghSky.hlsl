// The sky (UghMaterials::Sky): the body of the custom node of M_UghSky, which UghMakeAssets reads from this file. Its
// inputs: ToCamera the camera vector (world, from the dome to the camera), Sky the picture of the whole sky (a cube:
// the import makes one of its long-lat picture), Seen how bright the camera sees it (the sky light's capture sees it
// as it is: its light suits the scene, its look the exposure of the scene only when scaled). It returns the sky's
// colour seen that way.

float3 d = -normalize(ToCamera);
return TextureCubeSampleLevel(Sky, SkySampler, d, 0).rgb * (View.RealTimeReflectionCapture > 0 ? 1 : Seen);
