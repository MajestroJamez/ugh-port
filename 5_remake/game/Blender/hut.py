"""Huts of the stone age tribe for the remake's ledges as hut.glb, a mesh each, standing on the ground (origin at
the middle of their floor, the door towards -Y):

- hut_hide: a tent of poles (bark, the texture set palm_bark) tied at the top, covered with stitched hides painted
  with a band of signs, its door flap thrown open (2.9 m high, 2.6 m across),
- hut_thatch: a dome of straw over a ring of stones, a dark doorway framed by two mammoth tusks (2 m high, 3.2 m
  across).

    blender -b --factory-startup --python-exit-code 1 --python hut.py -- <folder> <palm_bark folder>
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import bmesh  # noqa: E402
import numpy  # noqa: E402
from mathutils import Vector  # noqa: E402

import creature_kit as creature  # noqa: E402
import prop_shapes as props  # noqa: E402
import ugh_kit as kit  # noqa: E402

FOLDER, BARK = kit.arguments()[:2]
SLOTS = ("hide", "thatch", "bark", "door", "bone", "stone")
DOOR = (-math.pi / 2 - 0.35, -math.pi / 2 + 0.35)   # the door's sides, angles about Z (the front is -Y)


def slot(name):
    return SLOTS.index(name)


def materials():
    size = kit.TEXTURE_SIZE
    u = (numpy.arange(size)[None, :] + 0.5) / size * numpy.ones((size, 1))
    v = (numpy.arange(size)[:, None] + 0.5) / size * numpy.ones((1, size))
    mottle = kit.noise(size, 2.3, seed=501)
    seams = (numpy.abs(((u * 6) % 1) - 0.5) > 0.485) & (((v * 40) % 1) < 0.6)
    signs = (numpy.abs(v - 0.55) < 0.05) & (numpy.abs(((u * 12) % 1) - 0.5) < 0.25 - numpy.abs(v - 0.55) * 4)
    hide = kit.colour_ramp(mottle, (0.5, 0.36, 0.22), (0.72, 0.56, 0.38))
    hide = numpy.where(seams[..., None], hide * 0.45, hide)
    hide = numpy.where(signs[..., None], numpy.array((0.5, 0.1, 0.05)), hide)
    straw = kit.noise(size, 2.0, seed=502, stretch=(1.0, 14.0))
    thatch = kit.colour_ramp(straw, (0.36, 0.26, 0.1), (0.78, 0.62, 0.3)) * (0.75 + 0.25 * mottle[..., None])
    stone = kit.colour_ramp(kit.noise(size, 2.2, seed=503), (0.3, 0.29, 0.27), (0.55, 0.53, 0.5))
    made = {"hide": kit.material("hide", kit.save_image(FOLDER, "hut_hide", hide),
                                 kit.save_image(FOLDER, "hut_hide_normal",
                                                kit.normals_from_height(mottle - seams * 0.3, 3), colour=False),
                                 roughness=0.7, double_sided=True),
            "thatch": kit.material("thatch", kit.save_image(FOLDER, "hut_thatch", thatch),
                                   kit.save_image(FOLDER, "hut_thatch_normal", kit.normals_from_height(straw, 8),
                                                  colour=False), roughness=0.9),
            "bark": kit.poly_haven_material("bark", BARK),
            "door": kit.material("door", (0.02, 0.015, 0.01), roughness=1.0),
            "bone": props.bone_material(FOLDER),
            "stone": kit.material("stone", kit.save_image(FOLDER, "hut_stone", stone), roughness=0.8)}
    return [made[name] for name in SLOTS]


def surface(bm, rings, material):
    """Quads between `rings` (lists of (point, u, v)), each ring a closed loop or not (same count)."""
    uv = bm.loops.layers.uv.verify()
    verts = [[bm.verts.new(point) for point, _, _ in ring] for ring in rings]
    for r in range(len(rings) - 1):
        for i in range(len(rings[r]) - 1):
            quad = (verts[r][i], verts[r][i + 1], verts[r + 1][i + 1], verts[r + 1][i])
            face = bm.faces.new(quad)
            face.material_index, face.smooth = material, True
            coords = (rings[r][i][1:], rings[r][i + 1][1:], rings[r + 1][i + 1][1:], rings[r + 1][i][1:])
            for loop, coord in zip(face.loops, coords):
                loop[uv].uv = coord


def in_door(angle):
    return DOOR[0] < angle < DOOR[1]


def hut_hide():
    bm = bmesh.new()
    apex, radius, height = Vector((0, 0, 2.5)), 1.3, 2.4
    poles = 9
    for index in range(poles):
        angle = 2 * math.pi * index / poles + 0.2
        foot = Vector((radius * 1.05 * math.cos(angle), radius * 1.05 * math.sin(angle), 0))
        top = apex + (apex - foot).normalized() * 0.45
        kit.tube(bm, [foot, top], [0.05, 0.035], sides=6, material_index=slot("bark"))
    # the cover: a cone from the ground to just below the apex, open at the door
    steps = 40
    angles = [DOOR[1] + (2 * math.pi - (DOOR[1] - DOOR[0])) * i / steps for i in range(steps + 1)]
    rings = []
    for z in numpy.linspace(0.02, height - 0.2, 8):
        r = radius * (1 - z / apex.z)
        rings.append([(Vector((r * math.cos(a), r * math.sin(a), z)), i / steps * 3, z / height)
                      for i, a in enumerate(angles)])
    surface(bm, rings, slot("hide"))
    # the dark inside seen through the door, and the flap thrown back beside it
    inside = [Vector((x, 0.3, z)) for x, z in ((-0.7, 0.02), (0.7, 0.02), (0.05, 2.0), (-0.05, 2.0))]
    rings = [[(inside[0], 0, 0), (inside[1], 1, 0)], [(inside[3], 0, 1), (inside[2], 1, 1)]]
    surface(bm, rings, slot("door"))
    def edge(z):
        r = radius * (1 - z / apex.z)
        return Vector((r * math.cos(DOOR[0]), r * math.sin(DOOR[0]), z))
    rings = [[(edge(0.05), 0, 0), (edge(0.05) + Vector((-0.5, -0.3, 0)), 0.4, 0)],
             [(edge(1.5), 0, 0.6), (edge(1.5) + Vector((-0.25, -0.15, 0)), 0.2, 0.6)]]
    surface(bm, rings, slot("hide"))
    return bm


def hut_thatch():
    bm = bmesh.new()
    radius, height, steps = 1.5, 1.85, 48
    rings = []
    for k, theta in enumerate(numpy.linspace(math.pi / 2, 0.05, 12)):   # up from the ground: faces outwards
        ring = []
        for i in range(steps + 1):
            a = 2 * math.pi * i / steps
            shag = 1 + 0.04 * math.sin(a * 7 + theta * 5)
            r = radius * math.sin(theta) * shag * (1.08 if k == 0 else 1)
            ring.append((Vector((r * math.cos(a), r * math.sin(a), height * math.cos(theta) + 0.05)), i / steps * 4,
                         theta / (math.pi / 2)))
        rings.append(ring)
    surface(bm, rings, slot("thatch"))
    kit.blob(bm, Vector((0, 0, height + 0.05)), (0.22, 0.22, 0.2), seed=51, bumps=0.2, material_index=slot("thatch"))
    # the doorway: a dark arch on the front, two tusks curving over it
    arch = [Vector((0.42 * math.cos(t), -radius * 1.12, 0.02 + 1.05 * math.sin(t))) for t in numpy.linspace(0, math.pi, 9)]
    centre = bm.verts.new(Vector((0, -radius * 1.12, 0.02)))
    ring = [bm.verts.new(p) for p in arch]
    for a, b in zip(ring, ring[1:]):
        bm.faces.new((centre, a, b)).material_index = slot("door")
    for side in (1, -1):
        creature.cone(bm, (side * 0.55, -radius * 1.08, 0.0), (side * 0.12, -radius * 1.25, 1.35), 0.09,
                      slot("bone"), bend=(side * 0.35, -0.2, 0.2))
    rng = numpy.random.default_rng(52)
    for i in range(22):   # the ring of stones holding the straw down
        a = 2 * math.pi * (i + 0.5) / 22
        if in_door(a - 2 * math.pi if a > math.pi else a):
            continue
        size = rng.uniform(0.14, 0.22)
        kit.blob(bm, Vector((radius * 1.05 * math.cos(a), radius * 1.05 * math.sin(a), size * 0.6)),
                 (size, size * 0.9, size * 0.75), seed=int(rng.integers(1000)), bumps=0.25, segments=10, rings=6,
                 material_index=slot("stone"))
    return bm


def build():
    kit.clear_scene()
    made = materials()
    objects = [kit.mesh_object(name, make(), made) for name, make in
               (("hut_hide", hut_hide), ("hut_thatch", hut_thatch))]
    kit.export(os.path.join(FOLDER, "hut.glb"), objects)


build()
