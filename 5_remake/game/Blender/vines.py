"""Lianas of the jungle for the remake's ceilings and overhangs as vines.glb, a mesh each, hanging down from their
origin (where they grow out of the rock): a twisting stem, now and then a second thinner one, heart-shaped leaves
along them (more near the top), a tuft of moss where they hang from and a curled tip. vine_1 .. vine_4 are 1.2, 2,
2.8 and 3.6 m long.

    blender -b --factory-startup --python-exit-code 1 --python vines.py -- <folder>
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import bmesh  # noqa: E402
import numpy  # noqa: E402
from mathutils import Quaternion, Vector  # noqa: E402

import ugh_kit as kit  # noqa: E402

FOLDER = kit.arguments()[0]
LENGTHS = (1.2, 2.0, 2.8, 3.6)
STEM, LEAF, MOSS = 0, 1, 2
CURL = 5   # points of a stem's curled tip


def materials():
    size = kit.TEXTURE_SIZE
    u = (numpy.arange(size)[None, :] + 0.5) / size * numpy.ones((size, 1))
    v = (numpy.arange(size)[:, None] + 0.5) / size * numpy.ones((1, size))
    cells = kit.noise(size, 1.8, seed=601)
    # a leaf: U across, V from its stalk to its tip; a midrib and side veins, lighter towards the tip
    rib = numpy.clip(1 - numpy.abs(u - 0.5) / 0.012, 0, 1)
    veins = numpy.clip(1 - numpy.abs(((v - numpy.abs(u - 0.5) * 0.8) * 7) % 1 - 0.5) / 0.06, 0, 1) * 0.5
    leaf = kit.colour_ramp(cells * 0.6 + v * 0.4, (0.1, 0.3, 0.06), (0.3, 0.55, 0.14))
    leaf = leaf * (1 + 0.25 * (rib + veins)[..., None])
    stem = kit.colour_ramp(kit.noise(size, 2, seed=602, stretch=(1, 8)), (0.18, 0.2, 0.08), (0.34, 0.33, 0.14))
    moss = kit.colour_ramp(kit.noise(size, 2.6, seed=603), (0.12, 0.22, 0.05), (0.32, 0.45, 0.12))
    return [kit.material("stem", kit.save_image(FOLDER, "vine_stem", stem), roughness=0.8),
            kit.material("leaf", kit.save_image(FOLDER, "vine_leaf", leaf),
                         kit.save_image(FOLDER, "vine_leaf_normal", kit.normals_from_height(rib + veins, 2),
                                        colour=False), roughness=0.55, double_sided=True),
            kit.material("moss", kit.save_image(FOLDER, "vine_moss", moss),
                         kit.save_image(FOLDER, "vine_moss_normal",
                                        kit.normals_from_height(kit.noise(size, 2.6, seed=603), 6), colour=False),
                         roughness=0.9)]


def stem_path(length, rng, sway):
    """Points from the top down `length`, swaying sideways and in depth, the tip curling."""
    phase = rng.uniform(0, 6.28, size=2)
    points = []
    for s in numpy.linspace(0, 1, max(8, int(length * 10))):
        z = -length * s
        reach = sway * s ** 0.7
        points.append(Vector((reach * math.sin(5 * s + phase[0]), reach * 0.6 * math.sin(4 * s + phase[1]), z)))
    tip = points[-1]
    for t in numpy.linspace(0.3, 1.6, CURL) * math.pi:   # a curl at the end
        points.append(tip + Vector((0.08 * math.sin(t), 0, -0.04 - 0.06 * (1 - math.cos(t)) / 2)))
    return points


def leaf(bm, at, out, size, uv):
    """A heart-shaped leaf from its stalk at `at`, pointing along `out`, facing mostly the camera (-Y)."""
    out = out.normalized()
    across = out.cross(Vector((0, -1, 0)))
    across = across.normalized() if across.length > 1e-3 else Vector((1, 0, 0))
    outline = [(0.0, 0.0), (0.35, 0.12), (0.45, 0.35), (0.3, 0.7), (0.0, 1.0), (-0.3, 0.7), (-0.45, 0.35),
               (-0.35, 0.12)]
    centre = bm.verts.new(at + out * size * 0.45 + Vector((0, -0.01, 0)))
    ring = [bm.verts.new(at + across * x * size + out * y * size + Vector((0, 0.02 * y * size, 0)))
            for x, y in outline]
    for a, b in zip(ring, ring[1:] + ring[:1]):
        face = bm.faces.new((centre, a, b))
        face.material_index, face.smooth = LEAF, True
        for loop in face.loops:
            local = loop.vert.co - at
            loop[uv].uv = (0.5 + local.dot(across) / size, local.dot(out) / size)


def vine(length, seed):
    bm = bmesh.new()
    uv = bm.loops.layers.uv.verify()
    rng = numpy.random.default_rng(seed)
    stems = [(stem_path(length, rng, 0.12 + 0.05 * length), 0.022)]
    if length > 1.5:
        stems.append((stem_path(length * 0.7, rng, 0.1), 0.012))
    for path, radius in stems:
        kit.tube(bm, path, [radius * (1 - 0.6 * i / len(path)) for i in range(len(path))], sides=6, uv_length=0.5,
                 material_index=STEM)
        # leaves along it (not on its curl), denser near the top
        for index, point in enumerate(path[:-CURL]):
            for _ in range(int(rng.poisson(3.0 * (1.2 - index / len(path))))):
                out = Vector((rng.choice((-1, 1)) * rng.uniform(0.4, 1), rng.uniform(-0.4, 0.2), rng.uniform(-1, 0.1)))
                out.rotate(Quaternion(Vector((0, 0, 1)), rng.uniform(-0.4, 0.4)))
                leaf(bm, point, out, rng.uniform(0.08, 0.15), uv)
    for _ in range(5):   # the moss it hangs from
        size = rng.uniform(0.06, 0.12)
        kit.blob(bm, Vector((rng.uniform(-0.15, 0.15), rng.uniform(-0.06, 0.06), -rng.uniform(0, 0.08))),
                 (size * 1.4, size, size * 0.7), seed=int(rng.integers(1000)), bumps=0.3, segments=10, rings=6,
                 material_index=MOSS)
    return bm


def build():
    kit.clear_scene()
    made = materials()
    objects = [kit.mesh_object(f"vine_{index + 1}", vine(length, 610 + index), made)
               for index, length in enumerate(LENGTHS)]
    kit.export(os.path.join(FOLDER, "vines.glb"), objects)


build()
