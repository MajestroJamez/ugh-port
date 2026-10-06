// The materials of the diorama: made by the editor's commandlet UghMakeAssets, used by the game.
#pragma once

#include "CoreMinimal.h"

/**
 * The materials the game uses, as asset paths and parameter names: the commandlet UghMakeAssets (module UghEditor)
 * writes them to Content/Generated on every build.ps1, the game makes dynamic instances of them.
 */
namespace UghMaterials
{
	inline const TCHAR* ColorParameter = TEXT("Color");
	inline const TCHAR* ArtParameter = TEXT("Art");
	inline const TCHAR* OpacityParameter = TEXT("Opacity");
	inline const TCHAR* IntensityParameter = TEXT("Intensity");
	inline const TCHAR* WindParameter = TEXT("Wind");
	inline const TCHAR* BaseColorParameter = TEXT("BaseColor");
	inline const TCHAR* NormalParameter = TEXT("Normal");
	inline const TCHAR* RoughnessParameter = TEXT("Roughness");
	inline const TCHAR* OcclusionParameter = TEXT("Occlusion");
	inline const TCHAR* HeightParameter = TEXT("Height");
	inline const TCHAR* TilingParameter = TEXT("Tiling");
	inline const TCHAR* SkyParameter = TEXT("Sky");
	inline const TCHAR* SkySeenParameter = TEXT("SkySeen");
	inline const TCHAR* SizeParameter = TEXT("Size");
	inline const TCHAR* HeightMaskParameter = TEXT("HeightMask");
	inline const TCHAR* WaterLevelParameter = TEXT("WaterLevel");
	inline const TCHAR* RainParameter = TEXT("Rain");
	inline const TCHAR* CausticsParameter = TEXT("Caustics");
	inline const TCHAR* SunParameter = TEXT("Sun");
	inline const TCHAR* const RingParameters[] = { TEXT("Ring0"), TEXT("Ring1"), TEXT("Ring2"), TEXT("Ring3"),
		TEXT("Ring4"), TEXT("Ring5") };
	inline const TCHAR* const FallParameters[] = { TEXT("Fall0"), TEXT("Fall1") };
	inline const TCHAR* SkyReflectionParameter = TEXT("SkyReflection");
	inline const TCHAR* TopParameter = TEXT("Top");
	inline const TCHAR* BottomParameter = TEXT("Bottom");
	inline const TCHAR* LeftParameter = TEXT("Left");
	inline const TCHAR* RightParameter = TEXT("Right");
	inline const TCHAR* SpeedParameter = TEXT("Speed");
	inline const TCHAR* LengthParameter = TEXT("Length");
	inline const TCHAR* WidthParameter = TEXT("Width");
	inline const TCHAR* SaturationParameter = TEXT("Saturation");
	inline const TCHAR* MossParameter = TEXT("Moss");
	inline const TCHAR* RiseParameter = TEXT("Rise");
	inline const TCHAR* SpreadParameter = TEXT("Spread");
	inline const TCHAR* FlipbookParameter = TEXT("Flipbook");
	inline const TCHAR* GlowParameter = TEXT("Glow");
	inline const TCHAR* UndersideParameter = TEXT("Underside");
	inline const TCHAR* BlowParameter = TEXT("Blow");
	inline const TCHAR* DirectionParameter = TEXT("Direction");
	inline const TCHAR* ReachParameter = TEXT("Reach");
	/** The bursts of the events (UghBursts): their motion and look, as UghBurst.hlsl and UghBurstLook.hlsl name them. */
	inline const TCHAR* ElapsedParameter = TEXT("Elapsed");
	inline const TCHAR* LifeParameter = TEXT("Life");
	inline const TCHAR* StaggerParameter = TEXT("Stagger");
	inline const TCHAR* RepeatParameter = TEXT("Repeat");
	inline const TCHAR* LiftParameter = TEXT("Lift");
	inline const TCHAR* GravityParameter = TEXT("Gravity");
	inline const TCHAR* DragParameter = TEXT("Drag");
	inline const TCHAR* GrowParameter = TEXT("Grow");
	inline const TCHAR* StretchParameter = TEXT("Stretch");
	inline const TCHAR* ModeParameter = TEXT("Mode");
	inline const TCHAR* FlutterParameter = TEXT("Flutter");
	inline const TCHAR* SpinParameter = TEXT("Spin");
	inline const TCHAR* BoxParameter = TEXT("Box");
	inline const TCHAR* ScaleParameter = TEXT("Scale");
	inline const TCHAR* FloorParameter = TEXT("Floor");
	inline const TCHAR* ShapeParameter = TEXT("Shape");

	/** Plasticine: the figures. Parameter Color. */
	inline const TCHAR* Clay = TEXT("/Game/Generated/M_UghClay");
	/**
	 * The rock coloured by the original's drawing of the level (texture parameter Art, mesh UVs): the cliff when the
	 * texture sets of Cliff are not imported.
	 */
	inline const TCHAR* Rock = TEXT("/Game/Generated/M_UghRock");
	/**
	 * The cliff (FUghRockMesh): the layers of imported texture sets mapped onto it from three sides (nothing
	 * stretches), blended by the way the surface faces, by its vertex colours (how open, how deep, how near below a top
	 * edge, its large patches) and by the original's drawing (texture parameter Art, mesh UVs, softened), their heights
	 * deciding where one layer gives way to another. Each layer of CliffLayers has the texture parameters <layer><map>
	 * for the maps of CliffMaps, taken from the scanned surfaces of UghElectricDreams::CliffLayers where they were
	 * copied, else from the instance MI_<id> of its texture set (UghAssets::CliffSets); the scalar <layer>Size (metres
	 * one texture covers) and the vector <layer>HeightMask (which channels of its Height map are its relief: red by
	 * default). The scalar WaterLevel is the world's z of the water's surface (the rock is wet at it and under it).
	 * The shader code is Source/UghEditor/Shaders/UghCliff.hlsl.
	 */
	inline const TCHAR* Cliff = TEXT("/Game/Generated/M_UghCliff");
	inline const TCHAR* const CliffLayers[] = { TEXT("Rock"), TEXT("Stone"), TEXT("Grass"), TEXT("Moss"), TEXT("Soil") };
	inline const TCHAR* const CliffMaps[] = { BaseColorParameter, NormalParameter, RoughnessParameter, HeightParameter };
	/**
	 * The water (Single Layer Water: the engine draws what is behind it through as much water as the view crosses,
	 * refracted, and the surface's reflections with Lumen): on its surface (what faces up) swells and chop drifting
	 * with the wind, the rings of what floats (vector parameters RingParameters: xyz its place on the surface, w how
	 * much it stirs it), of raindrops (scalar Rain 0 .. 1) and foam where it meets the rock and around what floats, a
	 * churning patch of foam where a waterfall pours in (vector parameters FallParameters: xyz the middle of its foot
	 * on the surface, w its width; 0 none); the open sea mirrors the sky itself (texture parameter Sky, the mood's
	 * cube, scalar SkySeen how bright the camera sees it, SkyReflection 1 when Sky is set); its cut (what faces the
	 * camera, seen from under the surface) only lets the water be seen through. Scalars
	 * WaterLevel (the world's z of the surface: what lies deeper is bluer and darker), Wind (-1 .. 1), Caustics (how
	 * bright the caustics on what lies below the surface are), vector Sun (where the sunlight goes). The shader code
	 * is Source/UghEditor/Shaders/UghWater.hlsl.
	 */
	inline const TCHAR* Water = TEXT("/Game/Generated/M_UghWater");
	/**
	 * The rain's streaks: a mesh of tiny quads (AUghRain) moved by the material, each a drop falling through the box
	 * Left .. Right, Bottom .. Top (the world's x and z) at Speed (cm/s) along its way (down and with the Wind as far)
	 * as a streak Length x Width turned to the camera, gone under the WaterLevel; translucent, unlit: Color, Opacity.
	 * The shader code is Source/UghEditor/Shaders/UghRain.hlsl.
	 */
	inline const TCHAR* Rain = TEXT("/Game/Generated/M_UghRain");
	/**
	 * Raindrops' splashes on quads (AUghRain): now and then a flat ring and droplets flying up; a quad whose second
	 * UV's y is 1 stands on the water (moved up by WaterLevel). Translucent, unlit: Color, Opacity. The shader code is
	 * Source/UghEditor/Shaders/UghSplash.hlsl.
	 */
	inline const TCHAR* Splash = TEXT("/Game/Generated/M_UghSplash");
	/**
	 * Flowing water (AUghFalls: a spring's spout, its stream, its waterfall), translucent and lit: its pattern moves
	 * with the water - the first UV's u across it (0 .. 1), v the seconds the water has flowed to get there -, ripples
	 * and foam at the banks of a stream, a falling sheet (the second UV's u: 0 lying, 1 falling) white with long
	 * streaks and gaps, whiter towards its foot (the second UV's v 0 .. 1); gone under the scalar WaterLevel (the world's
	 * z of the water's surface). The shader code is Source/UghEditor/Shaders/UghFlow.hlsl.
	 */
	inline const TCHAR* Flow = TEXT("/Game/Generated/M_UghFlow");
	/**
	 * The spray and the mist where a waterfall pours into the sea: a mesh of quads (AUghFalls) the material moves - each
	 * a puff rising from the scalar WaterLevel (the world's z of the surface) up to Rise (units), drifting out, growing
	 * from a part of Size (units), turned to the camera, fading -; translucent, lit by the air around it: Color, Opacity.
	 * The shader code is Source/UghEditor/Shaders/UghMist.hlsl.
	 */
	inline const TCHAR* Mist = TEXT("/Game/Generated/M_UghMist");
	/** A raindrop of the logic as a streak on a card (u along its way, its head at 1): translucent, Color, Opacity. */
	inline const TCHAR* Raindrop = TEXT("/Game/Generated/M_UghRaindrop");
	/**
	 * A flame on a card (unlit, additive, seen from both sides; card UVs: u across, v down): the flipbook of a simulated
	 * flame (texture parameter Flipbook, UghFlames) playing, two frames blended, each card of instanced ones from its own
	 * frame, fading out where the camera sees it edge on (crossed cards); Intensity, Wind (-1 .. 1, the flame leans that
	 * way). The shader code is Source/UghEditor/Shaders/UghFlame.hlsl.
	 */
	inline const TCHAR* Flame = TEXT("/Game/Generated/M_UghFlame");
	/**
	 * A fire's sparks: a mesh of tiny quads (UghFireParts::Sparks) moved by the material, each a spark shooting up out
	 * of the flame from its quad's middle up to Rise (units), Spread aside, Size big, drifting with the Wind, cooling
	 * from yellow to red; additive, unlit, Intensity. The shader code is Source/UghEditor/Shaders/UghSparks.hlsl.
	 */
	inline const TCHAR* Sparks = TEXT("/Game/Generated/M_UghSparks");
	/**
	 * A fire's thin plume of smoke: a mesh of tiny quads (UghFireParts::Smoke) moved by the material, each a puff
	 * rising from its quad's middle up to Rise (units), growing to Size, drifting with the Wind; translucent, lit:
	 * Color, Opacity. The shader code is Source/UghEditor/Shaders/UghSmoke.hlsl.
	 */
	inline const TCHAR* Smoke = TEXT("/Game/Generated/M_UghSmoke");
	/**
	 * Burning wood (the campfires' logs and coals, the torches' heads): black cracked charcoal, a little grey ash on top,
	 * its embers glowing in the cracks and breathing slowly (Glow how bright, Underside 0 .. 1 how much more where it
	 * faces down or aside). The shader code is Source/UghEditor/Shaders/UghEmbers.hlsl.
	 */
	inline const TCHAR* Embers = TEXT("/Game/Generated/M_UghEmbers");
	/**
	 * The blower's snort: a mesh of quads at its nostrils (FUghFigureModels) the material moves - puffs of dust and
	 * breath leaving one after another as the snort goes on (Blow 0 .. 1: before it 0, nothing shows), each shooting out
	 * along Direction (the world's way the nostrils blow) up to Reach (units), slowing, fanning out, growing to Size,
	 * turned to the camera, fading -; translucent, lit: Color, Opacity. The shader code is
	 * Source/UghEditor/Shaders/UghPuff.hlsl.
	 */
	inline const TCHAR* Puff = TEXT("/Game/Generated/M_UghPuff");
	/**
	 * The flyer's wings: thin skin the light shines through (two-sided foliage: lit from behind they glow warm,
	 * Color tints that light) - its BaseColor and Normal the imported model's (FUghFigureModels).
	 */
	inline const TCHAR* Membrane = TEXT("/Game/Generated/M_UghMembrane");
	/**
	 * The bursts of the events of the logic (AUghEffects, UghBursts): a mesh of tiny quads the material moves, each a
	 * particle of its own (the second UV its seed). Elapsed (seconds since the burst began; each particle starts up
	 * to Stagger later and lives about Life, again and again with Repeat 1), thrown from within Box (a vector: half its
	 * size in units across, in depth, up) at up to Speed along Direction (Spread 0 straight, 1 a hemisphere, 2 every
	 * way; its upward part times Lift), slowed by Drag (1/s), falling with Gravity (cm/s^2, below 0 rising), rocking
	 * Flutter units aside, never below Floor (the world's z); Size growing Grow times, Stretch along its way; Mode 0
	 * turned to the camera (slowly turning by Spin), 1 lying flat, 2 tumbling (Spin rad/s); Scale scales it all. Its
	 * look is Shape (UghBursts::EShape), Color; nothing under WaterLevel.
	 * The shader code is Source/UghEditor/Shaders/UghBurst.hlsl, UghBurstLook.hlsl. Three blends:
	 * Burst translucent and lit (dust, smoke, spray: Opacity), Bits cut out, lit, tumbling with their light (debris,
	 * leaves, feathers, shells: Roughness), Glint additive light (fire, sparks, glints: Intensity).
	 */
	inline const TCHAR* Burst = TEXT("/Game/Generated/M_UghBurst");
	inline const TCHAR* Bits = TEXT("/Game/Generated/M_UghBits");
	inline const TCHAR* Glint = TEXT("/Game/Generated/M_UghGlint");
	/**
	 * A picture on a card (unlit, transparent cut out, its front only: UghShapes::ShowCard), the original's sprites and
	 * the speech bubbles. Texture parameter Art.
	 */
	inline const TCHAR* Sprite = TEXT("/Game/Generated/M_UghSprite");
	/**
	 * A PBR surface of an imported texture set (UghAssets: its instance MI_<id>): texture parameters BaseColor, Normal,
	 * Roughness (its green channel), Occlusion (its red channel: a packed AO/roughness/metal map serves both) and
	 * Height (its red channel: the relief, which shifts the others with the view - parallax), the scalar Tiling
	 * (repeats of the textures per UV unit).
	 */
	inline const TCHAR* Pbr = TEXT("/Game/Generated/M_UghPbr");
	/**
	 * A scanned model of its own textures (mesh UVs), greyed: texture parameters BaseColor, Normal, Roughness (its green
	 * channel); its colour kept Saturation (0 .. 1) and tinted by Color, a dark mossy green on what faces up (Moss
	 * 0 .. 1 of it). The cliffs of the cave's dressing (AUghCliffDressing) in the cliff's limestone.
	 */
	inline const TCHAR* Scan = TEXT("/Game/Generated/M_UghScan");
	/**
	 * The sky around the world: an HDR picture of it (texture parameter Sky, a cube) seen in every direction, tinted
	 * by Color, times Intensity; unlit, the sky light captures it (it lights the scene and shows in reflections), the
	 * camera sees it times SkySeen (its picture is far brighter than the exposure of the scene wants).
	 */
	inline const TCHAR* Sky = TEXT("/Game/Generated/M_UghSky");
	/**
	 * The figures' halo (UghFigureLook), a post-process after the upscaler: around what is drawn into the custom depth
	 * (the figures), Radius (of the view's height) far, the background darkens by up to Darken (0 .. 1) unless it is
	 * dark already (a cave stays as it is). The shader code is Source/UghEditor/Shaders/UghFigureHalo.hlsl.
	 */
	inline const TCHAR* FigureHalo = TEXT("/Game/Generated/M_UghFigureHalo");
	inline const TCHAR* RadiusParameter = TEXT("Radius");
	inline const TCHAR* DarkenParameter = TEXT("Darken");
}
