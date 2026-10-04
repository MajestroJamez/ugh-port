"""Soft organic shapes for the remake's figures: blended metaballs (balls, ellipsoids, capsules) turned into meshes
with UVs, all of a mesh's faces in one material slot.
"""
import bmesh
import bpy
from mathutils import Vector

import ugh_kit as kit

SURFACE = 0.57   # a metaball's surface is this part of its radius (stiffness 2, threshold 0.6)


def metaballs(name, resolution):
    """A new metaball object and its data (`resolution` metres)."""
    data = bpy.data.metaballs.new(name)
    data.resolution = data.render_resolution = resolution
    holder = kit.new_object(name, data)
    return holder, data


def ellipsoid(data, at, semi, negative=False, rotation=None):
    """An ellipsoid of semi-axes `semi` (turned by quaternion `rotation`); `negative` carves it away."""
    element = data.elements.new(type="ELLIPSOID")
    element.co = at
    element.radius = 1 / SURFACE
    element.size_x, element.size_y, element.size_z = semi
    element.use_negative = negative
    if rotation is not None:
        element.rotation = rotation


def ball(data, at, radius, negative=False):
    ellipsoid(data, at, (radius, radius, radius), negative)


def capsule(data, a, b, radius):
    """A capsule from `a` to `b` of `radius`."""
    a, b = Vector(a), Vector(b)
    element = data.elements.new(type="CAPSULE")
    element.co = (a + b) / 2
    element.radius = radius / SURFACE
    element.size_x = max((b - a).length / 2, 1e-4)
    element.rotation = Vector((1, 0, 0)).rotation_difference(b - a)


def limb(data, points, radii):
    """Capsules along `points` with a radius at each (the thinner end of each capsule)."""
    for (a, b), radius in zip(zip(points, points[1:]), radii):
        capsule(data, a, b, radius)


def both_sides(points):
    """The left side's points (+X) and the right side's (mirrored)."""
    return [points, [(-x, y, z) for x, y, z in points]]


def to_mesh(holder, slot_index, materials, decimate=0.5, cut=None):
    """The metaballs of `holder` as a mesh object with UVs and `materials` in its slots, all its faces in slot
    `slot_index`; `cut(bmesh)` may delete faces first, `decimate` keeps that part of them."""
    evaluated = holder.evaluated_get(bpy.context.evaluated_depsgraph_get())
    mesh = bpy.data.meshes.new_from_object(evaluated)
    bpy.data.objects.remove(holder)
    made = kit.new_object(mesh.name, mesh)
    for each in materials:
        mesh.materials.append(each)
    if cut:
        bm = bmesh.new()
        bm.from_mesh(mesh)
        cut(bm)
        bm.to_mesh(mesh)
        bm.free()
    if decimate < 1:
        modifier = made.modifiers.new("decimate", "DECIMATE")
        modifier.ratio = decimate
        bpy.context.view_layer.objects.active = made
        bpy.ops.object.modifier_apply(modifier=modifier.name)
    for polygon in mesh.polygons:
        polygon.use_smooth = True
        polygon.material_index = slot_index
    unwrap(made)
    return made


def unwrap(made):
    bpy.ops.object.select_all(action="DESELECT")
    bpy.context.view_layer.objects.active = made
    made.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=1.1, island_margin=0.01)
    bpy.ops.object.mode_set(mode="OBJECT")


def join(parts, name):
    """`parts` joined into one object named `name` (its mesh too)."""
    bpy.ops.object.select_all(action="DESELECT")
    for part in parts:
        part.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    joined = bpy.context.view_layer.objects.active
    joined.name = joined.data.name = name
    return joined
