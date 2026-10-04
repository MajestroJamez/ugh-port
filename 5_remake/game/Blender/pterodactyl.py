"""The flyer of the remake (the original's pterodactyl): a rigged mesh with its actions as pterodactyl.glb.

A cartoon pteranodon about 2 m from beak to tail and 3.2 m across the wings (the original's sprite is 32 x 23 px),
facing -Y, its origin the middle of its body: a body of blended metaballs with a scaly hide, a long beak and a crest
(beak), a lower beak that opens (jaw), big eyes, and leathery wings (membrane) on long arms with a finger to the tip.
Actions (each a loop):

- fly: two beats of the wings and a glide (the original's flight shows 12 sprites a loop),
- fall: tumbling head over tail, the wings limp, the beak open (a passenger hit it).

    blender -b --factory-startup --python-exit-code 1 --python pterodactyl.py -- <folder>
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import bmesh  # noqa: E402
import numpy  # noqa: E402
from mathutils import Vector  # noqa: E402

import creature_kit as creature  # noqa: E402
import ugh_blobs as blobs  # noqa: E402
import ugh_kit as kit  # noqa: E402
import ugh_rig  # noqa: E402
from ugh_rig import Pose, sides, turn  # noqa: E402

FOLDER = kit.arguments()[0]
RESOLUTION = 0.02
SLOTS = ("skin", "beak", "jaw", "membrane", "eye", "pupil")
# the arm along the wing's leading edge: shoulder, elbow, wrist, the finger's tip (the left one, +X)
ARM = ((0.14, -0.15, 0.08), (0.62, -0.08, 0.14), (1.0, -0.02, 0.12), (1.6, 0.3, 0.06))
HIP = (0.15, 0.58, -0.02)
CENTRE = Vector((0, 0.05, 0))
BONES = {
    "root": ((0, 0, 0), (0, 0, 0.1), None),
    "body": ((0, 0.4, 0), (0, -0.3, 0.05), "root"),
    "neck": ((0, -0.3, 0.05), (0, -0.58, 0.22), "body"),
    "head": ((0, -0.58, 0.22), (0, -1.2, 0.2), "neck"),
    "jaw": ((0, -0.78, 0.19), (0, -1.18, 0.15), "head"),
    "tail": ((0, 0.4, 0), (0, 0.85, 0.06), "body"),
    **ugh_rig.mirrored({
        "upperarm.L": (ARM[0], ARM[1], "body"),
        "forearm.L": (ARM[1], ARM[2], "upperarm.L"),
        "finger.L": (ARM[2], ARM[3], "forearm.L"),
        "leg.L": ((0.09, 0.32, -0.08), (0.11, 0.66, -0.16), "body"),
    }),
}


def materials():
    plain = creature.plain_materials()
    size = kit.TEXTURE_SIZE
    veins = kit.noise(size, 1.6, seed=51, stretch=(8, 1))
    lines = numpy.exp(-((veins - 0.5) / 0.035) ** 2)
    mottle = kit.noise(size, 2.2, seed=52)
    membrane = kit.colour_ramp(mottle, (0.55, 0.28, 0.13), (0.72, 0.42, 0.22)) * (1 - 0.35 * lines[..., None])
    made = {
        "skin": creature.hide_material(FOLDER, "pterodactyl_skin", (0.36, 0.22, 0.12), (0.56, 0.38, 0.22),
                                       belly=(0.74, 0.6, 0.42), seed=41),
        "beak": kit.material("beak", (0.86, 0.6, 0.28), roughness=0.45),
        "jaw": kit.material("jaw", (0.82, 0.56, 0.26), roughness=0.45),
        "membrane": kit.material(
            "membrane", kit.save_image(FOLDER, "pterodactyl_membrane", membrane),
            kit.save_image(FOLDER, "pterodactyl_membrane_normal", kit.normals_from_height(mottle * 0.4 - lines, 2),
                           colour=False), roughness=0.6, double_sided=True),
        "eye": plain["eye"], "pupil": plain["pupil"]}
    return [made[name] for name in SLOTS]


def metaballs(name):
    return blobs.metaballs(name, RESOLUTION)


def body():
    holder, data = metaballs("Body")
    blobs.ellipsoid(data, (0, 0.05, 0), (0.2, 0.42, 0.2))
    blobs.ellipsoid(data, (0, -0.2, 0.04), (0.22, 0.22, 0.21))
    blobs.capsule(data, (0, -0.3, 0.08), (0, -0.55, 0.2), 0.11)
    blobs.ellipsoid(data, (0, -0.66, 0.27), (0.13, 0.19, 0.14))
    blobs.capsule(data, (0, 0.4, 0.0), (0, 0.85, 0.06), 0.04)
    for side in blobs.both_sides([ARM[0], ARM[1], ARM[2], ARM[3], (0.09, 0.32, -0.08), (0.11, 0.62, -0.16)]):
        blobs.limb(data, side[:4], (0.065, 0.05, 0.035))
        blobs.ball(data, side[1], 0.07)
        blobs.ball(data, side[2], 0.055)
        blobs.capsule(data, side[4], side[5], 0.04)
        blobs.ellipsoid(data, (side[5][0], 0.69, -0.17), (0.04, 0.06, 0.02))
    return holder


def beak():
    holder, data = metaballs("Beak")
    blobs.capsule(data, (0, -0.76, 0.27), (0, -1.0, 0.24), 0.06)
    blobs.capsule(data, (0, -1.0, 0.24), (0, -1.24, 0.2), 0.03)
    for s in numpy.linspace(0, 1, 6):   # the crest: a flat fin backwards and up
        blobs.ellipsoid(data, (0, -0.6 + 0.34 * s, 0.36 + 0.2 * s), (0.03, 0.1, 0.08 - 0.03 * s))
    return holder


def jaw():
    holder, data = metaballs("Jaw")
    blobs.capsule(data, (0, -0.76, 0.19), (0, -1.0, 0.17), 0.042)
    blobs.capsule(data, (0, -1.0, 0.17), (0, -1.17, 0.15), 0.024)
    return holder


def along(points, s):
    """The point at `s` (0 .. 1) of the length of polyline `points`."""
    points = [Vector(p) for p in points]
    lengths = [(b - a).length for a, b in zip(points, points[1:])]
    left = s * sum(lengths)
    for (a, b), length in zip(zip(points, points[1:]), lengths):
        if left <= length:
            return a.lerp(b, left / length)
        left -= length
    return points[-1]


def membrane(bm, slot):
    """The wings: from the arm back to the hips, the trailing edge curving in (a grid, U along the wing)."""
    uv = bm.loops.layers.uv.verify()
    columns, rows = 14, 5
    for sign in (1, -1):
        def mirror(p):
            return Vector((sign * p[0], p[1], p[2]))
        grid = []
        for i in range(columns + 1):
            s = i / columns
            lead = mirror(along(ARM, s))
            trail = mirror(Vector(HIP).lerp(Vector(ARM[3]), s)) + Vector((0, -0.08 * math.sin(math.pi * s), -0.02))
            grid.append([bm.verts.new(lead.lerp(trail, j / rows) + Vector((0, 0, -0.03 * math.sin(math.pi * j / rows))))
                         for j in range(rows + 1)])
        for i in range(columns):
            for j in range(rows):
                quad = [grid[i][j], grid[i + 1][j], grid[i + 1][j + 1], grid[i][j + 1]]
                face = bm.faces.new(quad if sign > 0 else list(reversed(quad)))
                face.material_index, face.smooth = slot, True
                coords = [(i, j), (i + 1, j), (i + 1, j + 1), (i, j + 1)]
                for loop, (u, v) in zip(face.loops, coords if sign > 0 else list(reversed(coords))):
                    loop[uv].uv = (u / columns, v / rows)


def eyes(bm):
    for side in (1, -1):
        creature.eye(bm, (side * 0.095, -0.72, 0.32), 0.055, (side * 0.6, -1, 0.15), SLOTS.index("eye"),
                     SLOTS.index("pupil"))


def fly(t):
    """Two beats of the wings, then a glide; the body rises as they beat down."""
    u = t / (2 * math.pi)
    beating = min(1.0, max(0.0, (0.68 - u) / 0.12) + max(0.0, (u - 0.88) / 0.12))
    lift = beating * 0.7 * math.cos(6 * math.pi * u) + (1 - beating) * 0.06
    pose = Pose()
    pose.move["body"] = Vector(BONES["body"][0]) + Vector((0, 0, -0.07 * lift))
    pose.turn["head"] = turn(x=0.05 * lift)
    pose.turn["tail"] = turn(x=0.15 * lift)
    for side, name in sides():
        pose.turn[f"upperarm.{name}"] = turn(y=-side * lift, z=side * 0.12 * lift)
        pose.turn[f"forearm.{name}"] = turn(y=-side * 0.25 * lift)
        pose.turn[f"finger.{name}"] = turn(y=-side * (0.3 * lift + 0.05), z=side * 0.1)
        pose.aim[f"leg.{name}"] = Vector((side * 0.05, 1, -0.25))
    return pose


def fall(t):
    """Head over tail about its middle, the wings limp and half up, the beak open."""
    pose = Pose()
    spin = turn(x=-t)
    pose.move["body"] = CENTRE + spin @ (Vector(BONES["body"][0]) - CENTRE)
    pose.turn["body"] = spin
    pose.turn["jaw"] = spun(spin, turn(x=-0.5))
    for side, name in sides():
        flop = 0.6 + 0.2 * math.sin(2 * t + side)
        pose.turn[f"upperarm.{name}"] = spun(spin, turn(y=-side * flop))
        pose.turn[f"forearm.{name}"] = spun(spin, turn(y=side * 0.5, z=side * 0.5))
        pose.turn[f"finger.{name}"] = spun(spin, turn(y=side * 0.4, z=side * 0.4))
    return pose


def spun(spin, rotation):
    """`rotation` about the axes of a body turned by `spin`."""
    return spin @ rotation @ spin.conjugated()


ACTIONS = {"fly": (fly, 48), "fall": (fall, 36)}


def build():
    kit.clear_scene()
    slots = materials()
    parts = [blobs.to_mesh(body(), SLOTS.index("skin"), slots, 0.5),
             blobs.to_mesh(beak(), SLOTS.index("beak"), slots, 0.6),
             blobs.to_mesh(jaw(), SLOTS.index("jaw"), slots, 0.6)]
    bm = bmesh.new()
    membrane(bm, SLOTS.index("membrane"))
    eyes(bm)
    parts.append(kit.mesh_object("Wings", bm, slots))
    figure = blobs.join(parts, "pterodactyl")
    armature = ugh_rig.build("pterodactyl", BONES, figure, falloff=0.06,
                             slot_bones={"beak": "head", "jaw": "jaw", "eye": "head", "pupil": "head"})
    ugh_rig.make(armature, BONES, ACTIONS)
    kit.export(os.path.join(FOLDER, "pterodactyl.glb"), [armature, figure])


build()
