"""The tree of the remake (the original's swaying tree with a face, which drops a fruit when a passenger bounces off
it) from a scanned hornbeam (Megascans' European hornbeam sapling of Epic's Electric Dreams sample, exported by
electric-dreams.ps1 to assets/3d/electricdreams; local only) as tree_hornbeam.glb.

An old pollarded hornbeam about 2.6 m high and 3.2 m across its crown (the original's sprite is 32 x 24 px), facing -Y,
its origin on the ground at the foot of its trunk: a squat, fluted trunk flaring into roots, splitting into limbs
(metaballs; its bark the scan's, baked from the hornbeam's tiling bark seen from three sides, darker in its hollows)
with a face carved into its bark (deep eye sockets under heavy brows with wet eyes in them, a knot of a nose, a
gaping mouth); the crown two of the scanned saplings' crowns (their twigs and leaves, cut out) on its limbs, a few
ripe wild apples hanging among the leaves; lids of bark over the eyes only the bone lids shows. Actions (each a loop):

- sway: the crown sways, the limbs follow; it blinks at the end of the loop (the original's 14 sprites a loop show
  the eyes shut in the last one),
- shaken: the eyes shut, the crown shaking (a passenger bounced off it; also when it has no fruit left).

    blender -b --factory-startup --python-exit-code 1 --python tree_hornbeam.py -- <folder> <electricdreams folder>
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import bmesh  # noqa: E402
import bpy  # noqa: E402
import numpy  # noqa: E402
from mathutils import Matrix, Vector  # noqa: E402

import creature_kit as creature  # noqa: E402
import ugh_bake as bake  # noqa: E402
import ugh_blobs as blobs  # noqa: E402
import ugh_kit as kit  # noqa: E402
import ugh_rig  # noqa: E402
from ugh_rig import Pose, turn  # noqa: E402

FOLDER, SCANS = kit.arguments()[:2]
SAPLING = "SM_EuropeanHornbeam_Sapling_03"
SLOTS = ("bark", "leaves", "twigs", "fruit", "eye", "lid")
RESOLUTION = 0.025
MAP_SIZE = 2048
BARK_TILE = 0.45           # m of trunk one picture of the bark covers
BLINK = 13 / 14            # of the sway loop: the eyes are shut from here on
# the crowns: each a sapling's above CUT (its own metres) on the limbs, scaled, turned about Z, at its foot
CUT = 0.32
CROWNS = (((1.55, 1.55, 1.0), 0.0, (0, 0.05, 0.95)), ((1.35, 1.35, 0.9), 2.4, (0.05, 0.15, 1.1)))
# the face: eyes, the brows over them, the nose, the mouth (metres, the trunk's front is about y -0.3)
EYES = ((0.12, -0.24, 0.86), (-0.12, -0.24, 0.86))
EYE_RADIUS = 0.045
MOUTH = (0, -0.27, 0.6)
FRUITS = 8
BONES = {
    "root": ((0, 0, 0), (0, 0, 0.1), None),
    "trunk": ((0, 0, 0.1), (0, 0, 0.95), "root"),
    "upper": ((0, 0, 0.95), (0, 0, 1.4), "trunk"),
    "crown": ((0, 0, 1.4), (0, 0, 2.6), "upper"),
    "lids": ((0, -0.3, 0.86), (0, -0.3, 0.96), "trunk"),
    **ugh_rig.mirrored({"branch.L": ((0.1, 0, 1.2), (1.3, 0, 1.9), "upper")}),
}
CROWN_BONES = ("upper", "crown", "branch.L", "branch.R")


def trunk():
    """The trunk, its roots, its limbs and the face carved into it (metaballs)."""
    holder, data = blobs.metaballs("Trunk", RESOLUTION)
    blobs.limb(data, [(0, 0, 0.05), (0.02, 0.02, 0.5), (0, 0, 0.95), (0, 0.02, 1.15)], (0.36, 0.3, 0.26))
    for angle, reach in ((0.3, 0.55), (1.5, 0.45), (2.6, 0.5), (3.8, 0.5), (5.0, 0.55)):   # roots, flaring
        out = Vector((math.cos(angle), math.sin(angle), 0))
        blobs.limb(data, [Vector((0, 0, 0.5)) + out * 0.15, out * 0.38 + Vector((0, 0, 0.08)),
                          out * reach + Vector((0, 0, -0.12))], (0.16, 0.07))
    for angle, rise in ((0.4, 0.55), (2.0, 0.6), (3.4, 0.5), (4.9, 0.55)):   # limbs into the crown
        out = Vector((math.cos(angle), 0.7 * math.sin(angle), 0))
        blobs.limb(data, [Vector((0, 0, 1.0)), out * 0.35 + Vector((0, 0, 1.0 + rise * 0.6)),
                          out * 0.75 + Vector((0, 0, 1.0 + rise))], (0.15, 0.08))
    for x in (-0.35, -0.12, 0.12, 0.35):   # flutes down the trunk
        blobs.capsule(data, (x * 0.8, -0.18, 0.1), (x * 0.6, -0.2, 0.8), 0.09)
    for eye in EYES:
        blobs.ball(data, eye, 0.085, negative=True)   # its socket
        blobs.ellipsoid(data, Vector(eye) + Vector((0, -0.04, 0.09)), (0.1, 0.05, 0.045))   # its brow
    blobs.ellipsoid(data, (0, -0.33, 0.74), (0.05, 0.06, 0.07))   # the nose, a knot
    blobs.ellipsoid(data, MOUTH, (0.13, 0.12, 0.06), negative=True)   # the mouth
    blobs.ellipsoid(data, Vector(MOUTH) + Vector((0, -0.04, -0.09)), (0.13, 0.05, 0.035))   # its lower lip
    return holder


def bark_graph(nodes, links):
    """The tiling bark seen from three sides in the trunk's space, darker in the hollows (the face's above all)."""
    folder = os.path.join(SCANS, SAPLING)
    coordinates = nodes.new("ShaderNodeTexCoord")
    scaled = nodes.new("ShaderNodeVectorMath")
    scaled.operation = "SCALE"
    scaled.inputs["Scale"].default_value = 1 / BARK_TILE
    links.new(coordinates.outputs["Object"], scaled.inputs[0])

    def picture(name, colour):
        texture = nodes.new("ShaderNodeTexImage")
        texture.image = kit.load_image(os.path.join(folder, name), colour=colour, size=MAP_SIZE)
        texture.projection = "BOX"
        texture.projection_blend = 0.3
        links.new(scaled.outputs[0], texture.inputs["Vector"])
        return texture
    albedo, mask = picture("Tileable_Albedo.png", True), picture("Tileable_Mask.png", False)
    channels = nodes.new("ShaderNodeSeparateColor")
    links.new(mask.outputs["Color"], channels.inputs["Color"])
    hollows = nodes.new("ShaderNodeAmbientOcclusion")
    hollows.samples = 16
    hollows.inputs["Distance"].default_value = 0.3
    shade = nodes.new("ShaderNodeMix")
    shade.data_type = "RGBA"
    shade.blend_type = "MULTIPLY"
    shade.inputs["Factor"].default_value = 1
    links.new(albedo.outputs["Color"], shade.inputs[6])
    links.new(bake.math(nodes, links, "POWER", hollows.outputs["AO"], 1.6), shade.inputs[7])
    roughness = bake.math(nodes, links, "MULTIPLY", channels.outputs["Green"], 0.95)
    return shade.outputs[2], roughness, channels.outputs["Blue"]


def bark(made):
    colour, normal, roughness = bake.bake(made, FOLDER, "tree_bark", MAP_SIZE, bark_graph, bump_distance=0.03)
    return kit.material("bark", colour, normal, roughness=roughness)


def scan_material(name, prefix, alpha=False):
    """A material of the sapling's maps <prefix>_Albedo, _Normal (DirectX's: green flipped), _Mask (roughness in
    green, cut out by red where `alpha`)."""
    folder = os.path.join(SCANS, SAPLING)
    colour = kit.load_image(os.path.join(folder, prefix + "_Albedo.png"), size=MAP_SIZE)
    mask = kit.load_image(os.path.join(folder, prefix + "_Mask.png"), colour=False, size=MAP_SIZE)
    if alpha:
        pixels = numpy.array(colour.pixels[:], dtype=numpy.float32).reshape(MAP_SIZE, MAP_SIZE, 4)
        pixels[..., 3] = numpy.array(mask.pixels[:], dtype=numpy.float32).reshape(MAP_SIZE, MAP_SIZE, 4)[..., 0]
        colour = kit.save_image(FOLDER, "tree_" + name, pixels)
    normal = kit.load_image(os.path.join(folder, prefix + "_Normal.png"), colour=False, size=MAP_SIZE, flip_green=True)
    return kit.material(name, colour, normal, roughness=mask, alpha_cutoff=0.5 if alpha else None, double_sided=alpha)


def crowns():
    """The saplings' crowns (above CUT) as placed by CROWNS, their slots the tree's."""
    bpy.ops.wm.obj_import(filepath=os.path.join(SCANS, SAPLING, SAPLING + ".obj"), forward_axis="Y", up_axis="Z")
    sapling = bpy.context.selected_objects[0]
    if sapling.data.has_custom_normals:
        bpy.context.view_layer.objects.active = sapling
        bpy.ops.mesh.customdata_custom_splitnormals_clear()
    slot_of = {index: SLOTS.index("leaves" if slot.material.name.startswith("TwoSided") else "twigs")
               for index, slot in enumerate(sapling.material_slots)}
    source = bmesh.new()
    source.from_mesh(sapling.data)
    bmesh.ops.delete(source, geom=[face for face in source.faces if face.calc_center_median().z < CUT], context="FACES")
    for face in source.faces:
        face.material_index = slot_of[face.material_index]
        face.smooth = True
    bmesh.ops.translate(source, vec=Vector((0, 0, -CUT)), verts=source.verts)
    bpy.data.objects.remove(sapling)
    bm = bmesh.new()
    for scale, angle, foot in CROWNS:
        mesh = bpy.data.meshes.new("crown")
        source.to_mesh(mesh)
        mesh.transform(Matrix.Translation(foot) @ Matrix.Rotation(angle, 4, "Z") @ Matrix.Diagonal((*scale, 1)))
        bm.from_mesh(mesh)
        bpy.data.meshes.remove(mesh)
    source.free()
    return bm


def fruits(bm):
    """FRUITS ripe fruits hanging under leaves on the front and the lower half of the crown, apart."""
    rng = numpy.random.default_rng(7)
    slot = SLOTS.index("leaves")
    places = [face.calc_center_median() for face in bm.faces if face.material_index == slot]
    low = min(p.z for p in places)
    wanted = [p for p in places if p.y < -0.45 and p.z < low + 0.75]
    chosen = []
    for index in rng.permutation(len(wanted)):
        point = wanted[index]
        if all((point - other).length > 0.35 for other in chosen):
            chosen.append(point)
        if len(chosen) == FRUITS:
            break
    fruit = SLOTS.index("fruit")
    for point in chosen:
        kit.blob(bm, point - Vector((0, 0.03, 0.07)), (0.05, 0.05, 0.055), seed=len(chosen), bumps=0.05,
                 material_index=fruit)


def fruit_material():
    """A ripe wild apple: a red blush over yellow-green, flecked, waxy."""
    size = 256
    blush = numpy.clip(kit.noise(size, 2.5, seed=71) * 1.6 - 0.3, 0, 1)
    flecks = (kit.noise(size, 0.6, seed=72) > 0.72).astype(numpy.float32)
    colour = kit.colour_ramp(blush, (0.55, 0.52, 0.22), (0.42, 0.11, 0.07)) * (1 - 0.2 * flecks[..., None])
    colour += numpy.array((0.12, 0.1, 0.03)) * flecks[..., None]
    return kit.material("fruit", kit.save_image(FOLDER, "tree_fruit", colour), roughness=0.55)


def face(bm, body):
    """Wet eyes in the sockets (out of their backs) and the lids of bark over them (shut;
    the bone lids shows them only to blink)."""
    for eye in EYES:
        hit, point, _, _ = body.ray_cast(Vector((eye[0], -5, eye[2])), Vector((0, 1, 0)))
        if not hit:
            raise RuntimeError(f"no bark in front of the eye at {eye}")
        middle = point - Vector((0, 0.25 * EYE_RADIUS, 0))   # in the socket, bulging out of its back
        look = Vector((0.15 * math.copysign(1, eye[0]), -1, 0.05))
        creature.wet_eye(bm, middle, EYE_RADIUS, look, SLOTS.index("eye"))
        creature.lid(bm, middle, 1.12 * EYE_RADIUS, look, -0.8, SLOTS.index("lid"))


def sway(t):
    pose = Pose()
    pose.turn["upper"] = turn(y=0.03 * math.sin(t))
    pose.turn["crown"] = turn(y=0.05 * math.sin(t - 0.5), x=0.02 * math.sin(2 * t))
    for side, name in ugh_rig.sides():
        pose.turn[f"branch.{name}"] = turn(y=0.03 * math.sin(t - 0.8) - side * 0.02 * math.sin(2 * t))
    if t / (2 * math.pi) < BLINK - 1e-6:
        creature.shrink(pose, "lids")
    return pose


def shaken(t):
    pose = Pose()
    pose.turn["trunk"] = turn(y=0.012 * math.sin(4 * t))
    pose.turn["upper"] = turn(y=0.04 * math.sin(4 * t + 0.5))
    pose.turn["crown"] = turn(y=0.07 * math.sin(4 * t + 1), x=0.03 * math.sin(6 * t))
    for side, name in ugh_rig.sides():
        pose.turn[f"branch.{name}"] = turn(y=side * 0.05 * math.sin(6 * t + 1))
    return pose


ACTIONS = {"sway": (sway, 56), "shaken": (shaken, 24)}


def build():
    kit.clear_scene()
    body = blobs.to_mesh(trunk(), SLOTS.index("bark"), [], 0.4)
    bark_material = bark(body)
    lid_material = bark_material.copy()
    lid_material.name = "lid"
    slots = [bark_material, scan_material("leaves", "TwoSided", alpha=True), scan_material("twigs", "Tileable"),
             fruit_material(), creature.wet_eye_material(FOLDER, "tree_eye"), lid_material]
    body.data.materials.clear()
    for material in slots:
        body.data.materials.append(material)
    bm = crowns()
    fruits(bm)
    face(bm, body)
    rest = kit.mesh_object("Crown", bm, slots)
    rest.data.uv_layers[0].name = body.data.uv_layers[0].name   # one UV map once joined
    kit.surface_uvs(rest, SLOTS.index("lid"), body)
    figure = blobs.join([body, rest], "tree_hornbeam")
    armature = ugh_rig.build("tree_hornbeam", BONES, figure, falloff=0.1,
                             slot_bones={"lid": "lids", "eye": "trunk"},
                             slot_allowed={"leaves": CROWN_BONES, "twigs": CROWN_BONES, "fruit": CROWN_BONES,
                                           "bark": [name for name in BONES if name != "lids"]})
    ugh_rig.make(armature, BONES, ACTIONS)
    kit.export(os.path.join(FOLDER, "tree_hornbeam.glb"), [armature, figure])


build()
