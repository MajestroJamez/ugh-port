"""The stone passenger of the remake from a scanned mossy rock (Megascans' MossyForestRock_02 of Epic's Electric Dreams
sample, exported by electric-dreams.ps1 to assets/3d/electricdreams; local only) as stone_boulder.glb: the rock,
lightened from a million triangles to FACES, fitted into the stone passenger's ellipsoid (copter_layout.HANGING_SEMI:
it hangs in the sling and sits on the seat as the old one did), its scanned colour, normal and roughness maps; two wet
eyes sunk into its mossy face (-Y: the camera's side), looking forward under heavy lids of the rock.
Origin: the bottom middle (where it stands).

    blender -b --factory-startup --python-exit-code 1 --python stone_boulder.py -- <folder> <electricdreams folder>
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import bmesh  # noqa: E402
import bpy  # noqa: E402
import numpy  # noqa: E402
from mathutils import Matrix, Vector  # noqa: E402

import copter_layout as layout  # noqa: E402
import creature_kit as creature  # noqa: E402
import ugh_kit as kit  # noqa: E402

FOLDER, SCANS = kit.arguments()[:2]
SCAN = "SM_MossyForestRock_02"
FACES = 40000
MAP_SIZE = 2048
EYE_RADIUS = 0.1
EYES = (0.19, 0.6)       # the eyes: how far from the middle aside, at what part of the height
SINK = 0.45              # how much of an eye's radius is in the rock
LID, LID_LOW = 1.07, 0.25  # the lids' shell (of the eye's radius) and how low they come down over it


def scan():
    """The scan, lightened, fitted into the ellipsoid, its bottom middle at the origin."""
    folder = os.path.join(SCANS, SCAN)
    bpy.ops.wm.obj_import(filepath=os.path.join(folder, SCAN + ".obj"), forward_axis="Y", up_axis="Z")
    rock = bpy.context.selected_objects[0]
    bpy.context.view_layer.objects.active = rock
    lighter = rock.modifiers.new("lighter", "DECIMATE")
    lighter.ratio = FACES / len(rock.data.polygons)
    bpy.ops.object.modifier_apply(modifier=lighter.name)
    if rock.data.has_custom_normals:
        bpy.ops.mesh.customdata_custom_splitnormals_clear()
    points = numpy.array([vertex.co for vertex in rock.data.vertices])
    low, high = points.min(axis=0), points.max(axis=0)
    size = Vector(layout.HANGING_SEMI) * 2
    rock.data.transform(Matrix.Diagonal((*(size[i] / (high[i] - low[i]) for i in range(3)), 1)) @
                        Matrix.Translation(Vector((-(low[0] + high[0]) / 2, -(low[1] + high[1]) / 2, -low[2]))))
    for polygon in rock.data.polygons:
        polygon.use_smooth = True
    rock.data.materials.clear()
    rock.data.materials.append(kit.material(
        "stone", kit.load_image(os.path.join(folder, "default_material_Albedo.png"), size=MAP_SIZE),
        kit.load_image(os.path.join(folder, "default_material_Normal.png"), colour=False, size=MAP_SIZE,
                       flip_green=True),
        roughness=kit.load_image(os.path.join(folder, "default_material_DR.png"), colour=False, size=MAP_SIZE)))
    rock.name = "stone_boulder"
    return rock


def eyes(rock):
    """Two eyes sunk into the front of the rock, looking forward, under heavy lids of the rock (a cap of a shell over
    the top of each eye, the rock's picture on it)."""
    bm = bmesh.new()
    height = layout.HANGING_SEMI[2] * 2
    for side in (1, -1):
        start = Vector((side * EYES[0], -10, EYES[1] * height))
        hit, point, normal, _ = rock.ray_cast(start, Vector((0, 1, 0)))
        if not hit:
            raise RuntimeError(f"no rock in front of the eye at {start}")
        look = Vector((side * 0.08, -1, 0.06))
        middle = point - normal * (SINK * EYE_RADIUS)
        creature.wet_eye(bm, middle, EYE_RADIUS, look, 0)
        creature.lid(bm, middle, LID * EYE_RADIUS, look, LID_LOW, 1)
    made = kit.mesh_object("eyes", bm, [creature.wet_eye_material(FOLDER, "stone_eye"), rock.data.materials[0]])
    made.data.uv_layers[0].name = rock.data.uv_layers[0].name   # one UV map once joined
    kit.surface_uvs(made, 1, rock)
    return made


def build():
    kit.clear_scene()
    rock = scan()
    made = eyes(rock)
    bpy.ops.object.select_all(action="DESELECT")
    made.select_set(True)
    rock.select_set(True)
    bpy.context.view_layer.objects.active = rock
    bpy.ops.object.join()
    kit.export(os.path.join(FOLDER, "stone_boulder.glb"), [rock])


build()
