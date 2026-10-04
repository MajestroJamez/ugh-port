"""The stone passenger of the remake (the original's standing passenger, a boulder with eyes that the copter carries
in a sling) as stone_passenger.glb: a lumpy boulder of HANGING_SEMI (copter_layout) with two big eyes looking
forward (-Y). Origin: the bottom middle (where it stands).

    blender -b --factory-startup --python-exit-code 1 --python stone_passenger.py -- <folder> <rock_face_03 folder>
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import bmesh  # noqa: E402
from mathutils import Vector  # noqa: E402

import copter_layout as layout  # noqa: E402
import ugh_kit as kit  # noqa: E402

FOLDER, STONE = kit.arguments()[:2]


def build():
    kit.clear_scene()
    materials = [kit.poly_haven_material("stone", STONE), kit.material("eye", (0.95, 0.93, 0.88), roughness=0.15),
                 kit.material("pupil", (0.04, 0.03, 0.02), roughness=0.1)]
    semi = Vector(layout.HANGING_SEMI)
    middle = Vector((0, 0, semi.z))
    bm = bmesh.new()
    kit.blob(bm, middle, (semi.x, semi.y, semi.z), seed=7, bumps=0.12, segments=32, rings=20)
    for side in (1, -1):
        eye = middle + Vector((side * 0.2, -semi.y * 0.82, 0.16))
        kit.blob(bm, eye, (0.11, 0.07, 0.12), material_index=1)
        kit.blob(bm, eye + Vector((-side * 0.015, -0.06, -0.01)), (0.045, 0.02, 0.05), material_index=2,
                 segments=12, rings=8)
    kit.export(os.path.join(FOLDER, "stone_passenger.glb"), [kit.mesh_object("StonePassenger", bm, materials)])


build()
