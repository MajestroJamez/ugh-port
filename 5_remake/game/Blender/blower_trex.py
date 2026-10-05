"""The blower of the remake from Jan's T-rex (a static mesh from Fab, kept local: assets/3d/fab/trex; its low poly
mesh, 28 thousand vertices, with its PBR maps) as blower_trex.glb: rigged here, lying asleep on its belly.

The standing model (metres, its head towards -Y) gets a skeleton (BONES: the spine, the neck, the head and its jaw,
the tail, the legs, the little arms; a non-deforming bone at the nostrils, where the game puffs the dust out) and
Blender's automatic weights (the lower jaw's vertices all the jaw's). It is posed lying down asleep (lying: the body
on the ground, the legs folded under it as a resting bird's, the tail along the ground curling away from the camera,
the head resting on the ground turned a little towards the camera, the jaw closed, the eyes shut) and that pose
becomes its rest. SCALE makes it the size of the blower's sprite (32 x 22 px) on the screen, its tail reaching
behind. Origin: on the ground under the middle of what the game's camera sees of it, its body no nearer the camera
than NEAR (the tail curls away behind the slab of the play). Actions (each a loop):

- blow: it breathes in deeply (the ribs swell, the head lifts a little: the original's first 3 of its 10 sprites, the
  copters drawn in) and snorts out hard (the head thrusts forward, the ribs empty fast, then slowly: the rest, the
  copters blown away; the game puffs dust out of its nostrils then),
- stunned: it shakes its head, dazed.

    blender -b --factory-startup --python-exit-code 1 --python blower_trex.py -- <folder> <trex folder>
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import bpy  # noqa: E402
import numpy  # noqa: E402
from mathutils import Matrix, Vector  # noqa: E402

import ugh_kit as kit  # noqa: E402
import ugh_rig  # noqa: E402
from ugh_rig import Pose, turn  # noqa: E402

FOLDER, SOURCE = kit.arguments()[:2]
LOW_POLY = os.path.join(SOURCE, "trex_export", "trex_lowpoly")
SCALE = 2.25
BREATH_IN = 0.3   # of the loop: the original's first 3 of 10 sprites
TOWARD_CAMERA = 25   # degrees: how the game turns the blower, looking left, towards the camera (UghFigurePlace)
NEAR = 0.6        # m: nothing of it nearer the camera than this (the figures' slab is 0.2 deep; the enemies reach 0.6)


def scaled(point):
    return tuple(SCALE * c for c in point)


# the standing model's skeleton (metres of the FBX; its middle is at x 0.07 .. 0.1, its tail curls towards -X)
STANDING = {
    "root": ((0.07, 0, -0.38), (0.07, 0, -0.28), None),
    "hips": ((0.07, 0.12, 0.15), (0.08, -0.06, 0.17), "root"),
    "chest": ((0.08, -0.06, 0.17), (0.1, -0.32, 0.17), "hips"),
    "ribs": ((0.09, -0.02, 0.03), (0.1, -0.28, 0.03), "chest"),
    "neck1": ((0.1, -0.32, 0.17), (0.105, -0.45, 0.22), "chest"),
    "neck2": ((0.105, -0.45, 0.22), (0.105, -0.57, 0.25), "neck1"),
    "head": ((0.105, -0.57, 0.25), (0.105, -0.95, 0.24), "neck2"),
    "jaw": ((0.105, -0.6, 0.15), (0.105, -0.87, 0.05), "head"),
    "nostrils": ((0.105, -0.93, 0.235), (0.105, -1.0, 0.235), "head"),
    "tail1": ((0.06, 0.12, 0.12), (0.046, 0.3, 0.1), "hips"),
    "tail2": ((0.046, 0.3, 0.1), (0.0, 0.47, 0.08), "tail1"),
    "tail3": ((0.0, 0.47, 0.08), (-0.09, 0.62, 0.05), "tail2"),
    "tail4": ((-0.09, 0.62, 0.05), (-0.2, 0.76, 0.0), "tail3"),
    "tail5": ((-0.2, 0.76, 0.0), (-0.3, 0.87, -0.08), "tail4"),
    "tail6": ((-0.3, 0.87, -0.08), (-0.36, 0.96, -0.25), "tail5"),
    # the leg on +X (.L) stands forward, the one on -X (.R) back
    "thigh.L": ((0.19, -0.01, 0.14), (0.23, -0.1, -0.06), "hips"),
    "shin.L": ((0.23, -0.1, -0.06), (0.24, 0.07, -0.235), "thigh.L"),
    "foot.L": ((0.24, 0.07, -0.235), (0.25, -0.02, -0.34), "shin.L"),
    "toes.L": ((0.25, -0.02, -0.34), (0.26, -0.2, -0.37), "foot.L"),
    "thigh.R": ((-0.06, 0.02, 0.14), (-0.13, -0.025, -0.08), "hips"),
    "shin.R": ((-0.13, -0.025, -0.08), (-0.18, 0.19, -0.2), "thigh.R"),
    "foot.R": ((-0.18, 0.19, -0.2), (-0.2, 0.14, -0.35), "shin.R"),
    "toes.R": ((-0.2, 0.14, -0.35), (-0.21, 0.015, -0.38), "foot.R"),
    "arm.L": ((0.18, -0.31, 0.05), (0.2, -0.34, -0.04), "chest"),
    "forearm.L": ((0.2, -0.34, -0.04), (0.2, -0.37, -0.11), "arm.L"),
    "arm.R": ((0.02, -0.31, 0.05), (0.0, -0.34, -0.04), "chest"),
    "forearm.R": ((0.0, -0.34, -0.04), (0.0, -0.37, -0.11), "arm.R"),
}
BONES = {name: (scaled(head), scaled(tail), parent) for name, (head, tail, parent) in STANDING.items()}
GROUND = -0.38 * SCALE
DROP = 0.29 * SCALE          # the body comes down this far: its belly on the ground
JAW_CLOSES = 0.62            # radians: the open jaw shut
# its eyes in its texture (UV): their middles, how far they reach
EYES = ((0.064, 0.0582), (0.8255, 0.7647))
EYE_RADIUS = 0.0075
# the lower jaw: below the line from its hinge between the open jaws, in front of the hinge (standing, unscaled)
HINGE = (-0.6, 0.155)        # y, z
JAW_LINE = math.radians(-9)  # the line's slope, forward and down


def image(role, colour=False):
    return kit.load_image(os.path.join(LOW_POLY, f"trex_lowpoly_{role}.png"), colour=colour)


def shut_eyes():
    """Its base colour with the eyes shut: each eye's disc in the texture (EYES) painted over in the skin around it,
    darker towards its rim (the shut lids in their socket); the new map is <folder>/trex_eyes_shut.png."""
    colour = image("basecolor", True)
    width, height = colour.size
    pixels = numpy.array(colour.pixels[:], dtype=numpy.float32).reshape(height, width, 4)
    rows, columns = numpy.mgrid[0:height, 0:width]
    for u, v in EYES:
        away = numpy.hypot((columns + 0.5) / width - u, (rows + 0.5) / height - v) / EYE_RADIUS
        skin = numpy.median(pixels[(away > 1.1) & (away < 1.8)], axis=0)
        inside = away < 1.1
        cover = numpy.clip((1.1 - away[inside]) * 4, 0, 1)[:, None]
        lid = skin * (0.75 + 0.25 * numpy.cos(numpy.clip(away[inside], 0, 1) * math.pi / 2))[:, None]
        pixels[inside] = pixels[inside] * (1 - cover) + lid * cover
    colour.pixels[:] = pixels.ravel()
    colour.filepath_raw = os.path.join(FOLDER, "trex_eyes_shut.png")
    colour.file_format = "PNG"
    colour.save()
    return colour


def material():
    return kit.material("trex", shut_eyes(), image("normal"), roughness=image("roughness"))


def imported():
    kit.clear_scene()
    bpy.ops.import_scene.fbx(filepath=os.path.join(LOW_POLY, "SM_trex_lowpoly.fbx"))
    figure = next(each for each in bpy.data.objects if each.type == "MESH")
    bpy.ops.object.select_all(action="DESELECT")
    figure.select_set(True)
    bpy.context.view_layer.objects.active = figure
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    figure.data.transform(Matrix.Scale(SCALE, 4))
    figure.data.materials.clear()
    figure.data.materials.append(material())
    for polygon in figure.data.polygons:
        polygon.use_smooth = True
    figure.name = "blower_trex"
    return figure


def only_marks(armature, figure):
    """The nostrils' bone only marks where they are: its weights go to the head."""
    armature.data.bones["nostrils"].use_deform = False
    head, nostrils = figure.vertex_groups["head"], figure.vertex_groups["nostrils"]
    for vertex in figure.data.vertices:
        for entry in vertex.groups:
            if entry.group == nostrils.index:
                head.add([vertex.index], entry.weight, "ADD")
    figure.vertex_groups.remove(nostrils)


def jaw_weights(figure):
    """The lower jaw (below the line between the open jaws, in front of its hinge) follows the jaw alone."""
    groups = figure.vertex_groups
    jaw = groups["jaw"].index
    slope = math.tan(JAW_LINE)
    for vertex in figure.data.vertices:
        y, z = vertex.co.y / SCALE, vertex.co.z / SCALE
        ahead = HINGE[0] - y
        below = z < HINGE[1] + slope * ahead
        if ahead > 0 and below and abs(vertex.co.x / SCALE - 0.105) < 0.12:
            for entry in list(vertex.groups):
                groups[entry.group].remove([vertex.index])
            groups[jaw].add([vertex.index], 1.0, "REPLACE")


def lying():
    """Asleep on its belly (see above), in the standing skeleton's space."""
    pose = Pose()
    pose.move["root"] = Vector(BONES["root"][0]) - Vector((0, 0, DROP))
    pose.turn["chest"] = turn(x=0.06)
    pose.aim["neck1"] = Vector((0.05, -1, -0.45))
    pose.aim["neck2"] = Vector((0.2, -1, -0.25))
    pose.aim["head"] = Vector((0.3, -1, -0.12))
    pose.turn["jaw"] = turn(x=-JAW_CLOSES)
    for name, direction in (("tail1", (0, 1, -0.55)), ("tail2", (-0.2, 1, -0.2)), ("tail3", (-0.5, 1, -0.05)),
                            ("tail4", (-0.9, 1, 0)), ("tail5", (-1, 0.5, 0)), ("tail6", (-1, 0.1, 0.05))):
        pose.aim[name] = Vector(direction)
    for side, x in ((".L", 0.27), (".R", -0.2)):
        hip = Vector(BONES["thigh" + side][0]) - Vector((0, 0, DROP))
        ankle = Vector((x * SCALE, hip.y + 0.13 * SCALE, GROUND + 0.05 * SCALE))
        pose.reach["thigh" + side] = (ankle, (0, -1, 0.3))
        pose.aim["foot" + side] = Vector((0, -1, -0.12))
        pose.aim["toes" + side] = Vector((0, -1, 0.02))
        pose.aim["arm" + side] = Vector((0, -0.6, -1))
        pose.aim["forearm" + side] = Vector((0, -1, -0.2))
    return pose


def pose_bones(armature, pose):
    """The armature's pose bones set to `pose` (armature space, parents first)."""
    posed = ugh_rig.solve(armature, BONES, pose)
    for name in BONES:
        bone = armature.pose.bones[name]
        bone.matrix = posed[name] @ Matrix.Diagonal((*pose.scale.get(name, (1, 1, 1)), 1))
        bpy.context.view_layer.update()


def rested(armature, figure):
    """The lying pose becomes the rest: the mesh deformed into it, the bones' rest moved there."""
    pose_bones(armature, lying())
    bpy.ops.object.select_all(action="DESELECT")
    figure.select_set(True)
    bpy.context.view_layer.objects.active = figure
    bpy.ops.object.modifier_apply(modifier=figure.modifiers[0].name)
    bpy.ops.object.select_all(action="DESELECT")
    armature.select_set(True)
    bpy.context.view_layer.objects.active = armature
    bpy.ops.object.mode_set(mode="POSE")
    bpy.ops.pose.armature_apply(selected=False)
    bpy.ops.object.mode_set(mode="OBJECT")
    modifier = figure.modifiers.new("rig", "ARMATURE")
    modifier.object = armature


def grounded(armature, figure):
    """Its lowest point on the ground; as the game shows it (looking left, turned TOWARD_CAMERA towards the camera,
    UghFigurePlace) the middle of what the camera sees at the origin and nothing nearer the camera than NEAR."""
    turn_rad = math.radians(TOWARD_CAMERA)
    across = Vector((math.sin(turn_rad), math.cos(turn_rad)))    # the screen's x of a point (x, y)
    nearer = Vector((math.cos(turn_rad), -math.sin(turn_rad)))   # towards the camera
    points = [vertex.co for vertex in figure.data.vertices]
    seen = [across.dot(p.to_2d()) for p in points]
    near = max(nearer.dot(p.to_2d()) for p in points)
    axes = Matrix(((across.x, across.y), (nearer.x, nearer.y)))
    shift = axes.inverted() @ Vector((-(min(seen) + max(seen)) / 2, NEAR - near))
    print(f"blower_trex: {max(seen) - min(seen):.2f} m wide on the screen")
    moved = Matrix.Translation((shift.x, shift.y, -min(p.z for p in points)))
    figure.data.transform(moved)
    bpy.ops.object.select_all(action="DESELECT")
    armature.select_set(True)
    bpy.context.view_layer.objects.active = armature
    bpy.ops.object.mode_set(mode="EDIT")
    for bone in armature.data.edit_bones:
        bone.transform(moved)
    bpy.ops.object.mode_set(mode="OBJECT")


def rising(p):
    """0 .. 1 smoothly over 0 .. 1."""
    p = min(max(p, 0.0), 1.0)
    return p * p * (3 - 2 * p)


def blow(t):
    p = t / (2 * math.pi)
    if p < BREATH_IN:
        breath = rising(p / BREATH_IN)
        snort = 0.0
    else:
        after = (p - BREATH_IN) / (1 - BREATH_IN)
        breath = 0.55 * (1 - rising(after * 4)) + 0.45 * (1 - rising(after))
        snort = math.sin(math.pi * min(after * 3, 1.0))
    pose = Pose()
    pose.scale["ribs"] = (1 + 0.1 * breath, 1, 1 + 0.12 * breath)
    pose.turn["neck1"] = turn(x=-0.07 * breath + 0.03 * snort)
    pose.turn["head"] = turn(x=-0.04 * breath + 0.06 * snort)
    return pose


def stunned(t):
    pose = Pose()
    pose.turn["neck1"] = turn(x=-0.12)
    pose.turn["neck2"] = turn(z=0.16 * math.sin(2 * t))
    pose.turn["head"] = turn(y=0.22 * math.sin(3 * t), z=0.08 * math.sin(4 * t))
    return pose


ACTIONS = {"blow": (blow, 60), "stunned": (stunned, 48)}


def build():
    figure = imported()
    armature = ugh_rig.build("blower_trex", BONES, figure, heat=True)
    only_marks(armature, figure)
    jaw_weights(figure)
    rested(armature, figure)
    grounded(armature, figure)
    ugh_rig.make(armature, BONES, ACTIONS)
    kit.export(os.path.join(FOLDER, "blower_trex.glb"), [armature, figure])


build()
