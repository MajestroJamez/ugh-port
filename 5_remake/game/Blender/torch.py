"""The torches on the cave's walls for the remake as torch.glb: a crooked shaft of a branch, its head a bundle of bark
and fibres soaked in pitch, bound with cords. The game burns its head (the slot "char" glows: embers in the pitch)
and puts a flame on it.

0.75 m long, the head 0.2 m long and about 0.1 m thick at its top; along +Z, its origin at the shaft's lower end (the
game wedges it into a crack of the rock, leaning out).

    blender -b --factory-startup --python-exit-code 1 --python torch.py -- <folder> <rough_wood folder>
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import bmesh  # noqa: E402
import numpy  # noqa: E402
from mathutils import Vector  # noqa: E402

import ugh_kit as kit  # noqa: E402

FOLDER, WOOD = kit.arguments()[:2]
SLOTS = ("shaft", "char", "cord")

LENGTH, SHAFT_RADIUS, HEAD_LENGTH, HEAD_RADIUS = 0.75, 0.022, 0.2, 0.05
# the head: this many strands of bark around the shaft's top, lumps of pitch among them; two cords around it
STRANDS, LUMPS, CORDS, CORD_RADIUS = 9, 7, 2, 0.006
TEXTURE_METRES = 0.4


def materials():
    """The shaft's weathered wood (rough_wood, darker and warmer), the charred head (black, a little glossy: pitch),
    the cords of plant fibre."""
    shaft = kit.poly_haven_material("torch_shaft", WOOD)
    return [shaft, kit.material("torch_char", (0.05, 0.04, 0.035), roughness=0.55),
            kit.material("torch_cord", (0.36, 0.28, 0.18), roughness=0.9)]


def shaft(bm, rng):
    """The branch: a little crooked, thinner towards its top, its lower end cut."""
    points = [Vector((rng.uniform(-0.012, 0.012), rng.uniform(-0.012, 0.012), z))
              for z in numpy.linspace(0, LENGTH, 9)]
    radii = [SHAFT_RADIUS * (1.1 - 0.25 * t) for t in numpy.linspace(0, 1, 9)]
    kit.tube(bm, points, radii, sides=12, uv_length=TEXTURE_METRES, material_index=SLOTS.index("shaft"))
    return points[-1]


def head(bm, top, rng):
    """Strands of bark bent round the shaft's top, swelling to the top, lumps of pitch between them, cords below."""
    bottom = top.z - HEAD_LENGTH
    for strand in range(STRANDS):
        angle = 2 * math.pi * (strand + rng.uniform(-0.2, 0.2)) / STRANDS
        out = Vector((math.cos(angle), math.sin(angle), 0))
        points, radii = [], []
        for t in numpy.linspace(0, 1, 7):
            reach = SHAFT_RADIUS + (HEAD_RADIUS - SHAFT_RADIUS) * math.sin(min(t * 1.25, 1) * math.pi / 2)
            points.append(Vector((top.x, top.y, bottom + t * (HEAD_LENGTH + 0.02))) + out * reach * (0.75 + 0.1 * t))
            radii.append(0.014 * (0.7 + 0.5 * t) * rng.uniform(0.85, 1.15))
        kit.tube(bm, points, radii, sides=7, material_index=SLOTS.index("char"))
    for lump in range(LUMPS):
        angle = rng.uniform(0, 2 * math.pi)
        z = bottom + rng.uniform(0.35, 1.0) * HEAD_LENGTH
        at = Vector((top.x + math.cos(angle) * HEAD_RADIUS * 0.7, top.y + math.sin(angle) * HEAD_RADIUS * 0.7, z))
        size = rng.uniform(0.018, 0.03)
        kit.blob(bm, at, (size, size, size * 1.3), seed=int(rng.integers(1000)), bumps=0.4, segments=10, rings=6,
                 material_index=SLOTS.index("char"))
    kit.blob(bm, Vector((top.x, top.y, top.z + 0.01)), (HEAD_RADIUS * 0.8, HEAD_RADIUS * 0.8, 0.035), seed=7,
             bumps=0.5, segments=12, rings=8, material_index=SLOTS.index("char"))
    for cord in range(CORDS):
        z = bottom + 0.015 + cord * 0.035
        ring = [Vector((top.x + math.cos(a) * (SHAFT_RADIUS + 0.012 + cord * 0.006),
                        top.y + math.sin(a) * (SHAFT_RADIUS + 0.012 + cord * 0.006), z + 0.004 * math.sin(3 * a)))
                for a in numpy.linspace(0, 2 * math.pi, 17)]
        kit.tube(bm, ring, [CORD_RADIUS] * len(ring), sides=6, twist=6, material_index=SLOTS.index("cord"))


def build():
    kit.clear_scene()
    rng = numpy.random.default_rng(41)
    bm = bmesh.new()
    head(bm, shaft(bm, rng), rng)
    kit.export(os.path.join(FOLDER, "torch.glb"), [kit.mesh_object("torch", bm, materials())])


build()
