"""The blower of the remake (in the original a big brown beast that blows the copters away): a rigged mesh with its
actions as blower.glb.

A round, warty cartoon beast about 2.9 m long and 2 m high (the original's sprite is 32 x 22 px), facing -Y (it blows
forward), its origin on the ground under its middle: a big body and head of blended metaballs, cheeks that puff up,
pursed lips at the end of a short trunk, little eyes on top, short legs; stars circle its head only while it is
stunned (star). Actions (each a loop):

- blow: it breathes in (its cheeks and belly swell, the lips draw back) and blows (its cheeks empty, the trunk thrusts
  forward): the original's 10 sprites a loop, the first 3 breathing in (the copters are drawn in), the rest blowing,
- stunned: slumped, the head swaying, the stars circling.

    blender -b --factory-startup --python-exit-code 1 --python blower.py -- <folder>
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
RESOLUTION = 0.03
SLOTS = ("skin", "mouth", "eye", "pupil", "star")
BREATH_IN = 0.3   # of the loop: the original's first 3 of 10 sprites
BONES = {
    "root": ((0, 0, 0), (0, 0, 0.2), None),
    "body": ((0, 0.8, 0.9), (0, -0.2, 1.0), "root"),
    "belly": ((0, 0.35, 0.4), (0, 0.35, 1.5), "body"),
    "head": ((0, -0.2, 1.0), (0, -0.9, 1.1), "body"),
    "snout": ((0, -0.9, 1.0), (0, -1.45, 1.0), "head"),
    "stars": ((0, -0.5, 2.05), (0, -0.5, 2.2), "head"),
    **ugh_rig.mirrored({
        "cheek.L": ((0.25, -0.55, 1.0), (0.62, -0.78, 1.0), "head"),
    }),
}


def materials():
    plain = creature.plain_materials()
    made = {"skin": creature.hide_material(FOLDER, "blower_skin", (0.3, 0.19, 0.1), (0.5, 0.33, 0.19),
                                           belly=(0.66, 0.5, 0.33), scale_count=260, seed=70, roughness=0.75),
            "star": creature.star_material(), **plain}
    return [made[name] for name in SLOTS]


def body():
    holder, data = blobs.metaballs("Body", RESOLUTION)
    blobs.ellipsoid(data, (0, 0.35, 0.95), (0.85, 0.95, 0.88))
    blobs.ellipsoid(data, (0, -0.5, 1.15), (0.68, 0.55, 0.6))
    blobs.capsule(data, (0, -0.9, 1.0), (0, -1.3, 1.0), 0.17)
    blobs.ball(data, (0, -1.36, 1.0), 0.2)   # the lips
    blobs.ball(data, (0, -1.5, 1.0), 0.11, negative=True)
    for side in blobs.both_sides([(0.45, -0.72, 1.0), (0.27, -0.82, 1.62), (0.55, -0.3, 0.32), (0.6, 0.85, 0.35)]):
        blobs.ellipsoid(data, side[0], (0.3, 0.28, 0.3))   # cheek
        blobs.ellipsoid(data, Vector(side[1]) + Vector((0, 0.02, 0.06)), (0.15, 0.12, 0.1))   # the lid over the eye
        for foot in side[2:]:
            blobs.ellipsoid(data, foot, (0.24, 0.3, 0.34))
            blobs.ellipsoid(data, (foot[0], foot[1] - 0.12, 0.07), (0.22, 0.2, 0.08))
    rng = numpy.random.default_rng(71)
    for _ in range(14):   # warts on the back
        angle, height = rng.uniform(0, 2 * math.pi), rng.uniform(0.2, 0.9)
        at = Vector((0, 0.35, 0.95)) + Vector((0.85 * math.cos(angle) * math.sqrt(1 - height ** 2),
                                               0.95 * math.sin(angle) * math.sqrt(1 - height ** 2) + 0.15,
                                               0.88 * height))
        blobs.ball(data, at, rng.uniform(0.07, 0.12))
    return holder


def details(bm):
    for side in (1, -1):
        creature.eye(bm, (side * 0.27, -0.84, 1.6), 0.11, (side * 0.3, -1, 0.1), SLOTS.index("eye"),
                     SLOTS.index("pupil"))
    kit.blob(bm, Vector((0, -1.43, 1.0)), (0.1, 0.04, 0.1), material_index=SLOTS.index("mouth"))
    creature.stars(bm, BONES["stars"][0], SLOTS.index("star"), radius=0.55)


def blow(t):
    u = t / (2 * math.pi)
    swell = math.sin(0.5 * math.pi * u / BREATH_IN) if u < BREATH_IN else max(0.0, 1 - (u - BREATH_IN) / 0.15)
    blowing = 0.0 if u < BREATH_IN else math.sin(math.pi * min(1.0, (u - BREATH_IN) / (1 - BREATH_IN)))
    pose = Pose()
    pose.scale["belly"] = (1 + 0.07 * swell, 1 + 0.05 * swell, 1 + 0.05 * swell)
    pose.turn["head"] = turn(x=-0.12 * swell + 0.1 * blowing)
    pose.move["snout"] = Vector(BONES["snout"][0]) + Vector((0, 0.1 * swell - 0.18 * blowing, 0.05 * swell))
    for _, name in sides():
        grow = 1 + 0.35 * swell
        pose.scale[f"cheek.{name}"] = (grow, grow, grow)
    creature.shrink(pose, "stars")
    return pose


def stunned(t):
    pose = Pose()
    pose.move["body"] = Vector(BONES["body"][0]) + Vector((0, 0, -0.08))
    pose.turn["body"] = turn(x=0.06, y=0.06 * math.sin(t))
    pose.turn["head"] = turn(x=0.3, z=0.2 * math.sin(t))
    pose.scale["belly"] = (1.04, 1.02, 0.94)
    pose.turn["stars"] = turn(z=t)
    return pose


ACTIONS = {"blow": (blow, 60), "stunned": (stunned, 48)}


def build():
    kit.clear_scene()
    slots = materials()
    bm = bmesh.new()
    details(bm)
    figure = blobs.join([blobs.to_mesh(body(), SLOTS.index("skin"), slots, 0.35),
                         kit.mesh_object("Details", bm, slots)], "blower")
    armature = ugh_rig.build("blower", BONES, figure, falloff=0.12,
                             slot_bones={"mouth": "snout", "eye": "head", "pupil": "head", "star": "stars"},
                             slot_allowed={"blower_skin": [name for name in BONES if name != "stars"]})
    ugh_rig.make(armature, BONES, ACTIONS)
    kit.export(os.path.join(FOLDER, "blower.glb"), [armature, figure])


build()
