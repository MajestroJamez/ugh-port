"""Totems of the stone age tribe for the remake's ledges as totem.glb, a mesh each, standing on the ground (origin at
their foot, facing -Y):

- totem_faces: a pole of three carved, painted faces stacked up, spread wings and a beak on top (3 m),
- totem_skull: a pole in bark (the texture set palm_bark) with a crossbar, a horned skull on top, bones and feathers
  hanging on strings (2.6 m).

    blender -b --factory-startup --python-exit-code 1 --python totem.py -- <folder> <palm_bark folder>
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
SLOTS = ("painted", "wings", "bark", "bone", "socket", "eye", "pupil", "feather", "string")


def slot(name):
    return SLOTS.index(name)


def materials():
    plain = creature.plain_materials()
    made = {"painted": props.painted_wood_material(FOLDER, "totem_painted", 401),
            "wings": props.painted_wood_material(FOLDER, "totem_wings", 405),
            "bark": kit.poly_haven_material("bark", BARK),
            "bone": props.bone_material(FOLDER), "socket": props.socket_material(),
            "feather": kit.material("feather", (0.72, 0.16, 0.08), roughness=0.6, double_sided=True),
            "string": kit.material("string", (0.42, 0.33, 0.2), roughness=0.9), **plain}
    return [made[name] for name in SLOTS]


def face(bm, z, radius, seed):
    """A carved head around the pole at height `z`: a round head, a heavy brow, big eyes, a nose, a grinning mouth."""
    kit.blob(bm, Vector((0, 0, z)), (radius, radius * 0.9, radius * 1.1), seed=seed, bumps=0.06,
             material_index=slot("painted"))
    kit.blob(bm, Vector((0, -radius * 0.78, z + radius * 0.35)), (radius * 0.75, radius * 0.25, radius * 0.16),
             material_index=slot("painted"))
    for side in (1, -1):
        creature.eye(bm, (side * radius * 0.38, -radius * 0.78, z + radius * 0.12), radius * 0.2, (0, -1, 0),
                     slot("eye"), slot("pupil"))
    kit.blob(bm, Vector((0, -radius * 0.95, z - radius * 0.15)), (radius * 0.16, radius * 0.22, radius * 0.25),
             material_index=slot("painted"))
    kit.blob(bm, Vector((0, -radius * 0.8, z - radius * 0.6)), (radius * 0.5, radius * 0.2, radius * 0.14),
             material_index=slot("socket"))


def wing(bm, side):
    """A flat carved wing spreading from the top of the pole to one side, its feathers in steps."""
    points = [(0.1, 2.45), (0.75, 2.75), (1.1, 2.95), (1.05, 2.7), (0.85, 2.55), (0.9, 2.4), (0.6, 2.3), (0.55, 2.15),
              (0.1, 2.15)]
    verts = [bm.verts.new((side * x, 0.02, z)) for x, z in points]
    face_ = bm.faces.new(verts if side > 0 else list(reversed(verts)))
    face_.material_index = slot("wings")
    uv = bm.loops.layers.uv.verify()
    for loop in face_.loops:
        loop[uv].uv = (abs(loop.vert.co.x) / 1.1, (loop.vert.co.z - 2.15) / 0.8)
    bmesh.ops.solidify(bm, geom=[face_], thickness=0.08)


def totem_faces():
    bm = bmesh.new()
    kit.tube(bm, [Vector((0, 0, 0)), Vector((0, 0, 2.6))], [0.2, 0.18], sides=12, material_index=slot("painted"))
    for index, z in enumerate((0.55, 1.25, 1.9)):
        face(bm, z, 0.36 - 0.04 * index, 410 + index)
    for side in (1, -1):
        wing(bm, side)
    kit.blob(bm, Vector((0, 0, 2.62)), (0.22, 0.2, 0.2), material_index=slot("painted"))
    creature.cone(bm, (0, -0.15, 2.6), (0, -0.55, 2.5), 0.09, slot("wings"), bend=(0, 0, -0.1))
    for side in (1, -1):
        creature.eye(bm, (side * 0.12, -0.14, 2.7), 0.06, (0, -1, 0), slot("eye"), slot("pupil"))
    return bm


def hanging(bm, top, length, rng):
    """A string from `top` with a bone or a feather at its end."""
    end = Vector(top) - Vector((0, 0, length))
    kit.tube(bm, [Vector(top), end], [0.01, 0.01], sides=5, material_index=slot("string"))
    if rng.random() < 0.5:
        props.bone(bm, end + Vector((0, 0, -0.02)), end + Vector((0.02, 0, -0.32)), 0.025, slot("bone"))
    else:
        tip = end - Vector((0, 0, 0.35))
        side = Vector((0.06, 0, 0))
        verts = [bm.verts.new(p) for p in (end, end - Vector((0, 0, 0.12)) + side, tip,
                                          end - Vector((0, 0, 0.12)) - side)]
        bm.faces.new(verts).material_index = slot("feather")


def totem_skull():
    bm = bmesh.new()
    rng = numpy.random.default_rng(420)
    pole = [Vector((0.03 * math.sin(z * 2), 0, z)) for z in numpy.linspace(0, 2.2, 6)]
    kit.tube(bm, pole, [0.12 - 0.02 * z / 2.2 for z in numpy.linspace(0, 2.2, 6)], sides=10,
             material_index=slot("bark"))
    kit.tube(bm, [Vector((-0.7, -0.05, 1.75)), Vector((0.7, -0.05, 1.8))], [0.06, 0.05], sides=8,
             material_index=slot("bark"))
    for x in (-0.6, -0.3, 0.25, 0.55):
        hanging(bm, (x, -0.05, 1.72), rng.uniform(0.25, 0.5), rng)
    for z in (1.79, 1.76):   # the cord binding the crossbar
        kit.tube(bm, [Vector((0, -0.13, z)), Vector((0, 0.13, z - 0.04))], [0.02, 0.02], sides=5,
                 material_index=slot("string"))
    props.skull_horned(bm, (0, 0, 2.05), 0.95, slot("bone"), slot("socket"))
    return bm


def build():
    kit.clear_scene()
    made = materials()
    objects = [kit.mesh_object(name, make(), made) for name, make in
               (("totem_faces", totem_faces), ("totem_skull", totem_skull))]
    kit.export(os.path.join(FOLDER, "totem.glb"), objects)


build()
