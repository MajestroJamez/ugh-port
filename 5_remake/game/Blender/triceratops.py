"""The walker of the remake (the original's triceratops): a rigged mesh with its actions as triceratops.glb.

A chubby cartoon triceratops about 3 m long and 1.9 m to the top of its frill (the original's sprite is 32 x 22 px),
facing -Y, its origin on the ground under its middle: a body of blended metaballs in a scaly hide, a frill with a
pattern (frill), three horns and a beak (horn), angry eyes, and stars that circle its head only while it is stunned
(star: scaled to nothing in the other actions). Actions (each a loop):

- walk: four legs, two at a time (the original's walk shows 6 sprites a loop),
- watch: standing, pawing the ground with a front foot, glaring about,
- charge: a gallop with the head down,
- recover: standing, shaking the head,
- stunned: lying on its belly, the head swaying, stars circling.

    blender -b --factory-startup --python-exit-code 1 --python triceratops.py -- <folder>
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import bmesh  # noqa: E402
import numpy  # noqa: E402
from mathutils import Quaternion, Vector  # noqa: E402

import creature_kit as creature  # noqa: E402
import ugh_blobs as blobs  # noqa: E402
import ugh_kit as kit  # noqa: E402
import ugh_rig  # noqa: E402
from ugh_rig import Pose, sides, turn  # noqa: E402

FOLDER = kit.arguments()[0]
RESOLUTION = 0.03
SLOTS = ("skin", "frill", "horn", "eye", "pupil", "star")
# per left leg (+X): hip or shoulder, knee, ankle
FRONT_LEG = ((0.4, -0.45, 0.8), (0.42, -0.5, 0.42), (0.42, -0.5, 0.12))
BACK_LEG = ((0.4, 0.55, 0.95), (0.43, 0.48, 0.47), (0.42, 0.55, 0.12))
BONES = {
    "root": ((0, 0, 0), (0, 0, 0.2), None),
    "hips": ((0, 0.55, 1.0), (0, 0.05, 1.0), "root"),
    "chest": ((0, 0.05, 1.0), (0, -0.55, 1.0), "hips"),
    "neck": ((0, -0.55, 1.0), (0, -0.9, 1.05), "chest"),
    "head": ((0, -0.9, 1.05), (0, -1.6, 0.85), "neck"),
    "stars": ((0, -1.0, 1.75), (0, -1.0, 1.9), "head"),
    "tail1": ((0, 0.9, 0.95), (0, 1.3, 0.75), "hips"),
    "tail2": ((0, 1.3, 0.75), (0, 1.85, 0.42), "tail1"),
    **ugh_rig.mirrored({
        "arm.L": (FRONT_LEG[0], FRONT_LEG[1], "chest"),
        "forearm.L": (FRONT_LEG[1], FRONT_LEG[2], "arm.L"),
        "thigh.L": (BACK_LEG[0], BACK_LEG[1], "hips"),
        "shin.L": (BACK_LEG[1], BACK_LEG[2], "thigh.L"),
    }),
}


def materials():
    plain = creature.plain_materials()
    size = kit.TEXTURE_SIZE
    distance, _ = kit.spots(size, 60, seed=61)
    mottle = kit.noise(size, 2.2, seed=62)
    spots = (distance < 0.045 + 0.02 * mottle).astype(numpy.float32)
    frill = kit.colour_ramp(mottle, (0.78, 0.42, 0.18), (0.92, 0.6, 0.28)) * (1 - 0.35 * spots[..., None])
    made = {
        "skin": creature.hide_material(FOLDER, "triceratops_skin", (0.28, 0.24, 0.13), (0.46, 0.38, 0.22),
                                       belly=(0.62, 0.54, 0.34), scale_count=1200, seed=60),
        "frill": kit.material("frill", kit.save_image(FOLDER, "triceratops_frill", frill),
                              kit.save_image(FOLDER, "triceratops_frill_normal",
                                             kit.normals_from_height(mottle * 0.3 + spots * 0.4, 3), colour=False),
                              roughness=0.6, double_sided=True),
        "star": creature.star_material(),
        **plain}
    return [made[name] for name in SLOTS]


def leg(data, points, radius):
    blobs.limb(data, points, (radius, radius * 0.85))
    ankle = points[2]
    blobs.ellipsoid(data, (ankle[0], ankle[1] - 0.05, 0.08), (radius * 1.05, radius * 1.2, 0.08))


def body():
    holder, data = blobs.metaballs("Body", RESOLUTION)
    blobs.ellipsoid(data, (0, 0.15, 0.98), (0.55, 0.85, 0.55))
    blobs.ellipsoid(data, (0, 0.1, 0.78), (0.5, 0.7, 0.42))
    blobs.ellipsoid(data, (0, 0.6, 1.0), (0.45, 0.4, 0.45))
    blobs.limb(data, [(0, 0.9, 0.95), (0, 1.3, 0.75), (0, 1.6, 0.55), (0, 1.85, 0.42)], (0.22, 0.12, 0.05))
    blobs.capsule(data, (0, -0.55, 1.0), (0, -0.85, 1.05), 0.36)
    blobs.ellipsoid(data, (0, -1.05, 1.02), (0.36, 0.4, 0.34))
    blobs.ellipsoid(data, (0, -1.38, 0.86), (0.23, 0.27, 0.23))
    blobs.capsule(data, (-0.3, -1.22, 1.22), (0.3, -1.22, 1.22), 0.07)   # the brow, frowning
    for side in blobs.both_sides([FRONT_LEG[0], FRONT_LEG[1], FRONT_LEG[2], BACK_LEG[0], BACK_LEG[1], BACK_LEG[2]]):
        leg(data, side[:3], 0.17)
        leg(data, side[3:], 0.2)
        blobs.ball(data, side[3], 0.3)
    return holder


def frill():
    """A thick disc behind the head leaning back, with bumps along its rim."""
    holder, data = blobs.metaballs("Frill", RESOLUTION * 0.8)
    lean = Quaternion((1, 0, 0), -0.95)
    centre = Vector((0, -0.68, 1.36))
    blobs.ellipsoid(data, centre, (0.78, 0.07, 0.56), rotation=lean)
    for angle in numpy.linspace(-0.2, math.pi + 0.2, 11):
        rim = lean @ Vector((0.8 * math.cos(angle), 0, 0.58 * math.sin(angle)))
        blobs.ball(data, centre + rim, 0.08)
    return holder


def horns(bm):
    slot = SLOTS.index("horn")
    for side in (1, -1):
        creature.cone(bm, (side * 0.2, -1.15, 1.25), (side * 0.3, -1.8, 1.55), 0.09, slot, bend=(0, 0.1, -0.25))
        creature.eye(bm, (side * 0.24, -1.26, 1.12), 0.09, (side * 0.5, -1, 0), SLOTS.index("eye"),
                     SLOTS.index("pupil"))
    creature.cone(bm, (0, -1.5, 1.0), (0, -1.68, 1.24), 0.07, slot, bend=(0, -0.05, 0))
    creature.cone(bm, (0, -1.5, 0.82), (0, -1.68, 0.7), 0.1, slot)   # the beak


def stand_leg(pose, chain, foot):
    """A leg (its upper bone `chain`) with its ankle at `foot`, the knee forward."""
    pose.reach[chain] = (foot, (0, -1, 0))


def feet(pose, offsets=None):
    """All four feet on the ground under their hips, moved by `offsets` (bone -> vector)."""
    offsets = offsets or {}
    for side, name in sides():
        for chain, ankle in ((f"arm.{name}", FRONT_LEG[2]), (f"thigh.{name}", BACK_LEG[2])):
            stand_leg(pose, chain, Vector((side * ankle[0], ankle[1], ankle[2])) + offsets.get(chain, Vector()))


def gait(t, stride, lift, bob):
    """Feet moving in diagonal pairs (front left with back right) by `stride`, lifted by `lift` going forward."""
    offsets = {}
    for side, name in sides():
        front, back = (0, math.pi) if side > 0 else (math.pi, 0)
        for chain, phase in ((f"arm.{name}", front), (f"thigh.{name}", back)):
            angle = t + phase
            offsets[chain] = Vector((0, -stride * math.cos(angle), lift * max(0.0, -math.sin(angle))))
    pose = Pose()
    feet(pose, offsets)
    pose.move["hips"] = Vector(BONES["hips"][0]) + Vector((0, 0, bob * math.cos(2 * t)))
    return pose


def walk(t):
    pose = gait(t, 0.17, 0.12, 0.025)
    pose.turn["hips"] = turn(y=0.03 * math.sin(t))
    pose.turn["head"] = turn(x=0.05 * math.cos(2 * t))
    pose.turn["tail1"] = turn(z=0.2 * math.sin(t))
    pose.turn["tail2"] = turn(z=0.35 * math.sin(t - 0.6))
    creature.shrink(pose, "stars")
    return pose


def watch(t):
    """Standing, the right front foot scraping the ground back and forth, the head turning to glare."""
    pose = Pose()
    feet(pose, {"arm.R": Vector((0, 0.18 * math.sin(2 * t), 0.06 * max(0.0, math.cos(2 * t))))})
    pose.move["hips"] = Vector(BONES["hips"][0]) + Vector((0, 0, -0.03))
    pose.turn["neck"] = turn(x=0.1)
    pose.turn["head"] = turn(x=0.15 + 0.05 * math.sin(2 * t), z=0.25 * math.sin(t))
    pose.turn["tail1"] = turn(z=0.3 * math.sin(t), x=-0.1)
    creature.shrink(pose, "stars")
    return pose


def charge(t):
    pose = gait(t, 0.3, 0.2, 0.05)
    pose.turn["hips"] = turn(x=0.08)
    pose.turn["neck"] = turn(x=0.3)
    pose.turn["head"] = turn(x=0.45 + 0.05 * math.sin(2 * t))
    pose.turn["tail1"] = turn(x=-0.25, z=0.15 * math.sin(t))
    creature.shrink(pose, "stars")
    return pose


def recover(t):
    pose = Pose()
    feet(pose)
    pose.move["hips"] = Vector(BONES["hips"][0]) + Vector((0.03 * math.sin(t), 0, -0.04))
    pose.turn["head"] = turn(z=0.45 * math.sin(3 * t), y=0.15 * math.sin(3 * t))
    pose.turn["tail1"] = turn(z=-0.2 * math.sin(t))
    creature.shrink(pose, "stars")
    return pose


def stunned(t):
    """Belly on the ground, the legs splayed, the head down and swaying, the stars going round."""
    pose = Pose()
    pose.move["hips"] = Vector((0, 0.55, 0.52))
    for side, name in sides():
        stand_leg(pose, f"arm.{name}", Vector((side * 0.75, -0.75, 0.1)))
        stand_leg(pose, f"thigh.{name}", Vector((side * 0.8, 0.85, 0.1)))
    pose.turn["neck"] = turn(x=0.25)
    pose.turn["head"] = turn(x=0.35, z=0.3 * math.sin(t), y=0.2 * math.sin(t))
    pose.turn["stars"] = turn(z=t)
    pose.turn["tail1"] = turn(x=-0.1)
    return pose


ACTIONS = {"walk": (walk, 24), "watch": (watch, 48), "charge": (charge, 16), "recover": (recover, 48),
           "stunned": (stunned, 48)}


def build():
    kit.clear_scene()
    slots = materials()
    bm = bmesh.new()
    horns(bm)
    creature.stars(bm, BONES["stars"][0], SLOTS.index("star"))
    figure = blobs.join([blobs.to_mesh(body(), SLOTS.index("skin"), slots, 0.35),
                         blobs.to_mesh(frill(), SLOTS.index("frill"), slots, 0.5),
                         kit.mesh_object("Horns", bm, slots)], "triceratops")
    armature = ugh_rig.build("triceratops", BONES, figure, falloff=0.08,
                             slot_bones={"frill": "head", "horn": "head", "eye": "head", "pupil": "head",
                                         "star": "stars"},
                             slot_allowed={"triceratops_skin": [name for name in BONES if name != "stars"]})
    ugh_rig.make(armature, BONES, ACTIONS)
    kit.export(os.path.join(FOLDER, "triceratops.glb"), [armature, figure])


build()
