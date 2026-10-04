"""Bones of the stone age for the remake's ledges as bones.glb, a mesh each, lying on the ground (origin at their
bottom middle, facing -Y):

- bone_long: a big long bone (1 m),
- bone_pile: three crossed bones and a cave bear's skull,
- skull_bear: a cave bear's skull (0.5 m),
- skull_horned: a triceratops' skull with its frill and horns (1.2 m),
- ribcage: what is left of a big beast: an arched spine and its ribs rising from the ground (2.6 m long).

    blender -b --factory-startup --python-exit-code 1 --python bones.py -- <folder>
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import bmesh  # noqa: E402
import numpy  # noqa: E402
from mathutils import Vector  # noqa: E402

import prop_shapes as props  # noqa: E402
import ugh_kit as kit  # noqa: E402

FOLDER = kit.arguments()[0]
BONE, SOCKET = 0, 1


def bone_long():
    bm = bmesh.new()
    props.bone(bm, (-0.5, 0, 0.07), (0.5, 0.05, 0.07), 0.05, BONE)
    return bm


def bone_pile():
    bm = bmesh.new()
    props.bone(bm, (-0.55, -0.1, 0.06), (0.45, 0.15, 0.07), 0.05, BONE)
    props.bone(bm, (-0.3, 0.3, 0.12), (0.4, -0.25, 0.1), 0.04, BONE)
    props.bone(bm, (0.1, 0.35, 0.05), (0.6, 0.1, 0.05), 0.035, BONE)
    props.skull_round(bm, (-0.45, -0.25, 0), 0.4, BONE, SOCKET, look=(0.4, -1, 0))
    return bm


def skull_bear():
    bm = bmesh.new()
    props.skull_round(bm, (0, 0, 0), 0.5, BONE, SOCKET, look=(0.5, -1, 0))
    return bm


def skull_horned():
    bm = bmesh.new()
    props.skull_horned(bm, (0, 0, 0), 1.2, BONE, SOCKET, look=(0.6, -1, 0))
    return bm


def ribcage():
    """A spine arching along X over the ground, ribs from each vertebra down both sides to the ground."""
    bm = bmesh.new()
    rng = numpy.random.default_rng(31)
    spine = [Vector((x, 0, 0.25 + 0.75 * math.sin(math.pi * (x + 1.3) / 2.6))) for x in numpy.linspace(-1.3, 1.3, 14)]
    kit.tube(bm, spine, [0.07] * len(spine), sides=8, material_index=BONE)
    for point in spine:
        kit.blob(bm, point, (0.06, 0.11, 0.1), seed=int(rng.integers(1000)), bumps=0.1, segments=10, rings=6,
                 material_index=BONE)
    for index, point in enumerate(spine[2:-2]):
        reach = 0.55 + 0.25 * math.sin(math.pi * (index + 1) / (len(spine) - 3))
        for side in (1, -1):
            # out and down, bowing outwards; some ribs broken short
            length = 1.0 if rng.random() > 0.25 else rng.uniform(0.45, 0.7)
            curve = []
            for t in numpy.linspace(0, length, 8):
                angle = math.pi * 0.5 * t
                curve.append(point + Vector((0.08 * t, side * reach * math.sin(angle) * 1.1,
                                             -(point.z - 0.02) * (1 - math.cos(angle)))))
            kit.tube(bm, curve, [0.035 * (1 - 0.5 * t) for t in numpy.linspace(0, 1, 8)], sides=6,
                     material_index=BONE)
    props.skull_horned(bm, (1.7, -0.1, 0), 0.9, BONE, SOCKET, look=(1, -0.5, 0))
    return bm


def build():
    kit.clear_scene()
    materials = [props.bone_material(FOLDER), props.socket_material()]
    shapes = {"bone_long": bone_long, "bone_pile": bone_pile, "skull_bear": skull_bear, "skull_horned": skull_horned,
              "ribcage": ribcage}
    made = [kit.mesh_object(name, make(), materials) for name, make in shapes.items()]
    kit.export(os.path.join(FOLDER, "bones.glb"), made)


build()
