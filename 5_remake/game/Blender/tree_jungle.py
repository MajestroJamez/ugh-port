"""The tree of the remake (the original's swaying tree with a face, which drops a fruit when a passenger bounces off
it) as a jungle fig (step 32g) from scans of Epic's Electric Dreams sample (exported by electric-dreams.ps1 to
assets/3d/electricdreams; local only): tree_jungle.glb.

A strangler fig about 2.5 m high and 3.6 m across its crown (the original's sprite is 32 x 24 px), facing -Y, its
origin on the ground at the foot of its trunk: a squat trunk of stems grown together (metaballs; the netted stems of
a strangler fig), flaring into tall plank buttresses, the front ones over the lip of its ledge (the game grows its
long roots on down the rock's face, FUghTreeRoots), splitting into wide spreading limbs; its bark the scanned bark of
a buttress root baked from three sides, darker in the hollows, with a face carved into it (deep eye sockets under
heavy brows with wet eyes in them, a knot of a nose, a gaping mouth; lids of bark over the eyes only the bone lids
shows). Aerial roots hang from the limbs (some down to the ground as pillars), woody lianas loop between them and
hang down (the slot `roots`: the buttress root's bark tiling along them - the game's roots on the rock wear it too);
the crown is of big glossy taro leaves (the scanned taro's pictures on bent cards), small rosettes of them grow on
the limbs (epiphytes), ripe figs hang under the limbs and among the lowest leaves. Actions (each a loop):

- sway: the crown sways, the limbs follow; it blinks at the end of the loop (the original's 14 sprites a loop show
  the eyes shut in the last one),
- shaken: the eyes shut, the crown shaking (a passenger bounced off it; also when it has no fruit left).

    blender -b --factory-startup --python-exit-code 1 --python tree_jungle.py -- <folder> <electricdreams folder>
"""
import json
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import bmesh  # noqa: E402
import bpy  # noqa: E402
import numpy  # noqa: E402
from mathutils import Quaternion, Vector  # noqa: E402

import creature_kit as creature  # noqa: E402
import ugh_bake as bake  # noqa: E402
import ugh_blobs as blobs  # noqa: E402
import ugh_kit as kit  # noqa: E402
import ugh_rig  # noqa: E402
from ugh_rig import Pose, turn  # noqa: E402

FOLDER, SCANS = kit.arguments()[:2]
TARO = "SM_Taro_02"
BARK = os.path.join(SCANS, "ButtressRoot")
SLOTS = ("bark", "leaves", "roots", "fruit", "eye", "lid")
RESOLUTION = 0.025
MAP_SIZE = 2048
ROOT_MAP = 1024
BARK_TILE = 0.9            # m of trunk one picture of the bark covers
ROOT_TILE = 0.6            # m along an aerial root or a liana
DARKER = 0.3          # the scanned bark darkened (an old fig's, in the shade of the jungle)
ROOT_TINT = (0.66, 0.64, 0.6)   # (sRGB) the tiling bark as dark as the trunk's
BLINK = 13 / 14            # of the sway loop: the eyes are shut from here on
# the face: eyes, the brows over them, the nose, the mouth (metres, the trunk's front is about y -0.3)
EYES = ((0.12, -0.24, 0.86), (-0.12, -0.24, 0.86))
EYE_RADIUS = 0.045
MOUTH = (0, -0.27, 0.6)
# the crown: a flattened dome of leaves (middle, semi-axes), how many, how long (m)
CROWN = ((0, 0.08, 1.85), (1.35, 0.55, 0.45))
LEAVES = 150
LEAF_LENGTH = (0.42, 0.62)
FRUITS = 9
# limbs: their ends (x, y, z) from the top of the trunk, the thinnest radius
LIMBS = (((1.35, 0.05, 1.72), 0.05), ((-1.35, 0.0, 1.75), 0.05), ((0.85, 0.45, 1.95), 0.045),
         ((-0.8, 0.42, 1.9), 0.045), ((0.55, -0.25, 2.05), 0.04), ((-0.5, -0.2, 2.1), 0.04),
         ((0.1, 0.3, 2.25), 0.04))
# buttresses: the angle about Z (0 is +X, -pi/2 the front), how far out on the ground, how high on the trunk, how far
# below the ground at their end (the front ones reach over the lip of the ledge)
BUTTRESSES = ((0.15, 0.85, 0.8, 0.05), (1.1, 0.7, 0.75, 0.05), (2.0, 0.7, 0.75, 0.05), (2.95, 0.85, 0.8, 0.05),
              (3.8, 0.75, 0.75, 0.3), (5.6, 0.75, 0.75, 0.3))
# roots over the front of the ledge from under the trunk, down over its lip (where the game's roots go on): x, how far
# forward, how far aside at their ends
LIP_ROOTS = ((-0.3, 0.25, -0.3), (-0.1, 0.2, -0.12), (0.12, 0.26, 0.14), (0.3, 0.22, 0.32))
BONES = {
    "root": ((0, 0, 0), (0, 0, 0.1), None),
    "trunk": ((0, 0, 0.1), (0, 0, 1.0), "root"),
    "upper": ((0, 0, 1.0), (0, 0, 1.45), "trunk"),
    "crown": ((0, 0, 1.45), (0, 0, 2.5), "upper"),
    "lids": ((0, -0.3, 0.86), (0, -0.3, 0.96), "trunk"),
    **ugh_rig.mirrored({"branch.L": ((0.15, 0, 1.15), (1.5, 0, 1.8), "upper")}),
}
CROWN_BONES = ("upper", "crown", "branch.L", "branch.R")


def limb_points(end):
    """The middle line of a limb from the top of the trunk to `end`, rising and then levelling out."""
    end = Vector(end)
    start = Vector((end.x * 0.08, end.y * 0.08, 1.05))
    middle = start.lerp(end, 0.45) + Vector((0, 0, 0.18))
    return [start, start.lerp(middle, 0.6), middle, middle.lerp(end, 0.55), end]


def trunk():
    """The trunk of stems grown together, its buttresses, its limbs and the face carved into it (metaballs)."""
    holder, data = blobs.metaballs("Trunk", RESOLUTION)
    blobs.limb(data, [(0, 0, 0.05), (0.02, 0.02, 0.5), (0, 0, 0.95), (0, 0.02, 1.15)], (0.36, 0.3, 0.27))
    # the stems of a strangler fig wound round the host, grown into one
    for index in range(5):
        angle = index * 2 * math.pi / 5 + 0.4
        points = []
        for step in range(7):
            height = 0.05 + step * 0.19
            turned = angle + step * 0.32
            if abs(math.sin(turned) + 1) < 0.5 and 0.45 < height < 1.05:   # not across the face
                turned += 0.9
            points.append((0.31 * math.cos(turned), 0.27 * math.sin(turned), height))
        blobs.limb(data, points, (0.1, 0.08, 0.07))
    # the plank buttresses: thin, tall where they leave the trunk, low far out
    for angle, reach, high, sink in BUTTRESSES:
        out = Vector((math.cos(angle), math.sin(angle), 0))
        rotation = Quaternion((0, 0, 1), angle)
        for step in range(8):
            share = step / 7
            height = high * 0.62 * (1 - share) ** 1.2 + 0.05
            at = out * (0.22 + share * (reach - 0.22)) + Vector((0, 0, 0.8 * height - sink * share ** 2))
            blobs.ellipsoid(data, at, (0.1, 0.038 * (1 - 0.4 * share), height), rotation=rotation)
    for end, thinnest in LIMBS:   # limbs spreading wide
        blobs.limb(data, limb_points(end), (0.15, 0.12, 0.09, 0.07, thinnest))
    for eye in EYES:
        blobs.ball(data, eye, 0.085, negative=True)   # its socket
        blobs.ellipsoid(data, Vector(eye) + Vector((0, -0.04, 0.09)), (0.1, 0.05, 0.045))   # its brow
    blobs.ellipsoid(data, (0, -0.33, 0.74), (0.05, 0.06, 0.07))   # the nose, a knot
    blobs.ellipsoid(data, MOUTH, (0.13, 0.12, 0.06), negative=True)   # the mouth
    blobs.ellipsoid(data, Vector(MOUTH) + Vector((0, -0.04, -0.09)), (0.13, 0.05, 0.035))   # its lower lip
    return holder


def bark_picture(nodes, links, scaled, name, colour):
    texture = nodes.new("ShaderNodeTexImage")
    texture.image = kit.load_image(os.path.join(BARK, name), colour=colour, size=MAP_SIZE)
    texture.projection = "BOX"
    texture.projection_blend = 0.3
    links.new(scaled.outputs[0], texture.inputs["Vector"])
    return texture


def bark_graph(nodes, links):
    """The buttress root's bark seen from three sides in the trunk's space, darker in the hollows (the face's above
    all) and in its own crevices (its AO)."""
    coordinates = nodes.new("ShaderNodeTexCoord")
    scaled = nodes.new("ShaderNodeVectorMath")
    scaled.operation = "SCALE"
    scaled.inputs["Scale"].default_value = 1 / BARK_TILE
    links.new(coordinates.outputs["Object"], scaled.inputs[0])
    albedo = bark_picture(nodes, links, scaled, "T_ButtressRoot_01_BC.png", True)
    packed = bark_picture(nodes, links, scaled, "T_ButtressRoot_01_AoRDp.png", False)
    channels = nodes.new("ShaderNodeSeparateColor")
    links.new(packed.outputs["Color"], channels.inputs["Color"])
    hollows = nodes.new("ShaderNodeAmbientOcclusion")
    hollows.samples = 16
    hollows.inputs["Distance"].default_value = 0.3
    shade = bake.math(nodes, links, "MULTIPLY", bake.math(nodes, links, "POWER", hollows.outputs["AO"], 1.8),
                      bake.math(nodes, links, "MULTIPLY", channels.outputs["Red"], DARKER))
    mix = nodes.new("ShaderNodeMix")
    mix.data_type = "RGBA"
    mix.blend_type = "MULTIPLY"
    mix.inputs["Factor"].default_value = 1
    links.new(albedo.outputs["Color"], mix.inputs[6])
    links.new(shade, mix.inputs[7])
    return mix.outputs[2], channels.outputs["Green"], channels.outputs["Blue"]


def bark(made):
    colour, normal, roughness = bake.bake(made, FOLDER, "tree_bark", MAP_SIZE, bark_graph, bump_distance=0.04)
    return kit.material("bark", colour, normal, roughness=roughness)


def roughness_map(image, name, size):
    """The green of `image` (roughness) alone, red 1 and blue 0 (glTF reads blue as metalness: the packed maps of the
    scans have translucency or height there)."""
    pixels = numpy.array(image.pixels[:], dtype=numpy.float32).reshape(size, size, 4)
    made = numpy.ones((size, size, 3), dtype=numpy.float32)
    made[..., 1] = pixels[..., 1]
    made[..., 2] = 0
    return kit.save_image(FOLDER, name, made, colour=False)


def roots_material():
    """The buttress root's bark tiling (aerial roots, lianas; the game's roots down the rock)."""
    colour = kit.load_image(os.path.join(BARK, "T_ButtressRoot_01_BC.png"), size=ROOT_MAP)
    normal = kit.load_image(os.path.join(BARK, "T_ButtressRoot_01_N.png"), colour=False, size=ROOT_MAP,
                            flip_green=True)
    packed = kit.load_image(os.path.join(BARK, "T_ButtressRoot_01_AoRDp.png"), colour=False, size=ROOT_MAP)
    return kit.material("roots", colour, normal, roughness=roughness_map(packed, "tree_roots_roughness", ROOT_MAP),
                        tint=ROOT_TINT)


def taro_maps():
    """The taro's colour, normal and ART maps (red: the leaves cut out, green: roughness): the files its export lists."""
    folder = os.path.join(SCANS, TARO)
    with open(os.path.join(folder, TARO + ".json")) as listed:
        slots = json.load(listed)
    found = {}
    for slot in slots.values():
        for parameter, name in slot["textures"].items():
            key = parameter.lower().replace(" ", "")
            for kind, words in (("colour", ("albedo", "basecolor", "colour", "color")), ("normal", ("normal",)),
                                ("cut", ("art",))):
                if kind not in found and any(word == key if kind == "cut" else word in key for word in words):
                    found[kind] = os.path.join(folder, name)
    print("taro maps", found)
    return found


def leaves_material():
    """Glossy taro leaves cut out of their pictures."""
    maps = taro_maps()
    colour = kit.load_image(maps["colour"], size=MAP_SIZE)
    pixels = numpy.array(colour.pixels[:], dtype=numpy.float32).reshape(MAP_SIZE, MAP_SIZE, 4)
    cut = kit.load_image(maps["cut"], colour=False, size=MAP_SIZE)
    pixels[..., 3] = numpy.array(cut.pixels[:], dtype=numpy.float32).reshape(MAP_SIZE, MAP_SIZE, 4)[..., 0]
    colour = kit.save_image(FOLDER, "tree_leaves", pixels)
    normal = kit.load_image(maps["normal"], colour=False, size=MAP_SIZE, flip_green=True) if "normal" in maps else None
    return kit.material("leaves", colour, normal, roughness=roughness_map(cut, "tree_leaves_roughness", MAP_SIZE),
                        double_sided=True)


# the green leaves' pictures on the taro's maps (u, v of Blender: v up), each from its stalk's end (the notch of the
# heart) to its tip
PICTURES = (((0.029, 0.658), (0.273, 0.980)), ((0.211, 0.336), (0.453, 0.648)), ((0.469, 0.340), (0.684, 0.648)),
            ((0.484, 0.023), (0.707, 0.332)))


def leaf(bm, base, forward, length, picture, droop, slot, fold=0.12):
    """A leaf from its stalk's end at `base` along `forward`, `length` m long (as wide as its picture is), shaped as
    a heart (not cut out by alpha: the game drew the cut-out leaves black or not at all), its picture on it, its midrib
    drooping by `droop` (of its length), its halves folded up a little."""
    (u0, v0), (u1, v1) = PICTURES[picture]
    width = length * (u1 - u0) / (v1 - v0)
    forward = Vector(forward).normalized()
    side = forward.cross(Vector((0, 0, 1)))
    if side.length < 1e-3:
        side = Vector((1, 0, 0))
    side.normalize()
    up = side.cross(forward)
    uv = bm.loops.layers.uv.verify()
    across, along = 4, 6
    grid = []
    for j in range(along + 1):
        t = j / along
        shape = max(math.sin(math.pi * min(1.0, 0.12 + 0.95 * t)) ** 0.7, 0.04)   # broad lobes, a pointed tip
        row = []
        for i in range(across + 1):
            a = (i / across - 0.5) * shape * 0.92
            point = (Vector(base) + forward * (t * length) + side * (a * width) + up * (fold * abs(a) * width)
                     + Vector((0, 0, -droop * length * t * t)))
            row.append((bm.verts.new(point), (u0 + (a + 0.5) * (u1 - u0), v0 + t * (v1 - v0))))
        grid.append(row)
    for j in range(along):
        for i in range(across):
            corners = (grid[j][i], grid[j][i + 1], grid[j + 1][i + 1], grid[j + 1][i])
            face = bm.faces.new([corner[0] for corner in corners])
            face.material_index = slot
            face.smooth = True
            for loop, corner in zip(face.loops, corners):
                loop[uv].uv = corner[1]


def crown(bm, rng):
    """LEAVES leaves on a flattened dome over the limbs, pointing out of it, the lowest drooping most."""
    (cx, cy, cz), (ax, ay, az) = CROWN
    slot = SLOTS.index("leaves")
    placed = 0
    while placed < LEAVES:
        direction = Vector(rng.normal(size=3))
        direction.normalize()
        if direction.z < -0.6:
            continue
        reach = 0.75 + 0.25 * rng.random()
        base = Vector((cx + ax * direction.x * reach, cy + ay * direction.y * reach, cz + az * direction.z * reach))
        # tilted (up, the lowest down): their faces towards the camera above and below, not edge on
        pitch = rng.uniform(0.35, 0.9) if direction.z > -0.1 else -rng.uniform(0.2, 0.6)
        out = Vector((direction.x, direction.y * 0.6 - 0.25, 0)).normalized() + Vector((0, 0, pitch))
        out += Vector(rng.normal(size=3)) * 0.2
        low = 1 - (direction.z + 0.6) / 1.6
        leaf(bm, base, out, rng.uniform(*LEAF_LENGTH), int(rng.integers(len(PICTURES))), 0.15 + 0.35 * low, slot)
        placed += 1


def epiphytes(bm, rng):
    """Small rosettes of leaves on the limbs, near the trunk (bromeliads and the like)."""
    slot = SLOTS.index("leaves")
    for end, _ in LIMBS[:4]:
        point = limb_points(end)[2] + Vector((0, 0, 0.08))
        for index in range(7):
            angle = index * 2 * math.pi / 7 + rng.random()
            out = Vector((math.cos(angle), math.sin(angle), 1.1))
            leaf(bm, point, out, rng.uniform(0.16, 0.24), int(rng.integers(len(PICTURES))), 0.25, slot, fold=0.3)


def hanging(start, end, sag, wobble, rng, steps=12):
    """Points of a rope from `start` to `end` sagging `sag` m in its middle, wobbling a little."""
    start, end = Vector(start), Vector(end)
    points = []
    for step in range(steps + 1):
        t = step / steps
        point = start.lerp(end, t) - Vector((0, 0, sag * 4 * t * (1 - t)))
        if 0 < step < steps:
            point += Vector(rng.normal(size=3)) * wobble
        points.append(point)
    return points


def ropes(bm, rng):
    """Aerial roots hanging from the limbs (the outer ones down to the ground: pillars) and lianas looping between
    them; none in front of the face."""
    slot = SLOTS.index("roots")
    for end, _ in LIMBS[:4]:
        line = limb_points(end)
        for share, pillar in ((0.55, False), (0.85, True), (0.95, False)):
            top = line[2].lerp(line[4], share - 0.3) - Vector((0, 0, 0.05))
            if abs(top.x) < 0.4 and top.y < 0:
                continue
            bottom = Vector((top.x * 1.05, top.y, -0.05 if pillar else rng.uniform(0.55, 1.05)))
            points = hanging(top, bottom, 0, 0.012, rng, steps=10)
            radius = 0.035 if pillar else 0.016
            radii = [radius * (0.8 + 0.4 * t / 10) if pillar else radius * (1 - 0.6 * t / 10) for t in range(11)]
            kit.tube(bm, points, radii, sides=7, uv_length=ROOT_TILE, material_index=slot)
    for end, thinnest in LIMBS:   # twigs forking off each limb's end into the leaves
        tip = Vector(end)
        for turn_by in (-0.5, 0.45):
            out = Vector((tip.x, tip.y, 0)).normalized() if abs(tip.x) + abs(tip.y) > 0.2 else Vector((0, 1, 0))
            out.rotate(Quaternion((0, 0, 1), turn_by))
            twig = [tip - Vector(end).normalized() * 0.08, tip + out * 0.18 + Vector((0, 0, 0.12)),
                    tip + out * 0.38 + Vector((0, 0, 0.2))]
            kit.tube(bm, twig, [thinnest * 0.8, thinnest * 0.55, 0.012], sides=6, uv_length=ROOT_TILE,
                     material_index=slot)
    for x, reach, aside in LIP_ROOTS:   # over the lip of the ledge, fanning out
        points = [Vector((x * 0.7, -0.15, 0.3)), Vector((x * 0.9, -0.3, 0.12)),
                  Vector((x + aside * 0.4, -0.3 - reach * 0.55, 0.02)),
                  Vector((x + aside * 0.75, -0.32 - reach * 0.9, -0.12)), Vector((x + aside, -0.3 - reach, -0.33))]
        points = [p + Vector(rng.normal(size=3)) * 0.015 for p in points]
        kit.tube(bm, points, [0.08, 0.065, 0.05, 0.042, 0.035], sides=8, uv_length=ROOT_TILE, material_index=slot)
    lianas = ((LIMBS[0][0], LIMBS[2][0], 0.7), (LIMBS[1][0], LIMBS[3][0], 0.65), (LIMBS[4][0], LIMBS[0][0], 0.55),
              (LIMBS[5][0], LIMBS[1][0], 0.6))
    for start, end, sag in lianas:
        a, b = Vector(start) * 0.85, Vector(end) * 0.85
        points = hanging(a, b, sag, 0.01, rng, steps=16)
        kit.tube(bm, points, [0.022] * len(points), sides=7, uv_length=ROOT_TILE, material_index=slot, twist=0.4)
    for x in (1.4, -1.38):   # two hanging down from the crown's edges, curling at their ends
        top = Vector((x, 0.1, 1.75))
        points = [top + Vector((0.04 * math.sin(t * 1.3), 0.03 * math.cos(t), -0.11 * t)) for t in range(10)]
        points += [points[-1] + Vector((0.06 * math.sin(a), 0, 0.06 * (math.cos(a) - 1))) for a in (0.8, 1.6, 2.4)]
        kit.tube(bm, points, [0.018 * (1 - 0.5 * i / len(points)) for i in range(len(points))], sides=7,
                 uv_length=ROOT_TILE, material_index=slot)


def fruits(bm, rng):
    """FRUITS ripe figs under the limbs and the lowest leaves, at the sides and in front, none before the face."""
    slot = SLOTS.index("fruit")
    chosen = []
    while len(chosen) < FRUITS:
        end, _ = LIMBS[int(rng.integers(4))]
        line = limb_points(end)
        point = line[2].lerp(line[4], rng.uniform(0.0, 0.9)) + Vector((0, rng.uniform(-0.25, -0.05), -0.1))
        if all((point - other).length > 0.25 for other in chosen):
            chosen.append(point)
    for point in chosen:
        kit.blob(bm, point, (0.045, 0.045, 0.055), seed=len(chosen), bumps=0.05, material_index=slot)


def fruit_material():
    """A ripe fig: a red blush over yellow-green, flecked, waxy."""
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


def leaf_normals(figure):
    """The leaves lit as one mass of leaves (the figures' light comes from the front): their normals out of the crown's
    middle and towards the camera instead of across each card (lit from behind a card turned away looked black)."""
    mesh = figure.data
    slot = SLOTS.index("leaves")
    middle = Vector(CROWN[0]) - Vector((0, 0, 0.4))
    normals = [Vector(corner.vector) for corner in mesh.corner_normals]
    for polygon in mesh.polygons:
        if polygon.material_index != slot:
            continue
        for index in polygon.loop_indices:
            out = mesh.vertices[mesh.loops[index].vertex_index].co - middle
            normals[index] = Vector((out.x, out.y * 0.5 - 0.8, out.z + 0.3)).normalized()
    mesh.normals_split_custom_set([tuple(normal) for normal in normals])


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
    rng = numpy.random.default_rng(32)
    body = blobs.to_mesh(trunk(), SLOTS.index("bark"), [], 0.35)
    bark_material = bark(body)
    lid_material = bark_material.copy()
    lid_material.name = "lid"
    slots = [bark_material, leaves_material(), roots_material(), fruit_material(),
             creature.wet_eye_material(FOLDER, "tree_eye"), lid_material]
    body.data.materials.clear()
    for material in slots:
        body.data.materials.append(material)
    bm = bmesh.new()
    crown(bm, rng)
    epiphytes(bm, rng)
    ropes(bm, rng)
    fruits(bm, rng)
    face(bm, body)
    rest = kit.mesh_object("Crown", bm, slots)
    rest.data.uv_layers[0].name = body.data.uv_layers[0].name   # one UV map once joined
    kit.surface_uvs(rest, SLOTS.index("lid"), body)
    figure = blobs.join([body, rest], "tree_jungle")
    leaf_normals(figure)
    corners = [figure.matrix_world @ Vector(corner) for corner in figure.bound_box]
    low, high = [min(c[i] for c in corners) for i in range(3)], [max(c[i] for c in corners) for i in range(3)]
    print("tree_jungle bounds", [round(v, 2) for v in low], [round(v, 2) for v in high],
          "triangles", sum(len(p.vertices) - 2 for p in figure.data.polygons), "uv", [uv.name for uv in figure.data.uv_layers])
    armature = ugh_rig.build("tree_jungle", BONES, figure, falloff=0.1,
                             slot_bones={"lid": "lids", "eye": "trunk"},
                             slot_allowed={"leaves": CROWN_BONES, "fruit": CROWN_BONES,
                                           "roots": [name for name in BONES if name != "lids"],
                                           "bark": [name for name in BONES if name != "lids"]})
    ugh_rig.make(armature, BONES, ACTIONS)
    kit.export(os.path.join(FOLDER, "tree_jungle.glb"), [armature, figure])


build()
