"""What the enemies of the remake (pterodactyl.py, triceratops.py, blower.py, fruit_tree.py, tree_hornbeam.py,
stone_slate.py) share: a scaly hide, a horn or claw, cartoon eyes (a white ball with a pupil) and wet ones (an
eyeball's picture), stars over a dizzy head, and their materials.
"""
import math

import bmesh
import bpy
import numpy
from mathutils import Matrix, Vector

import ugh_kit as kit


def hide_material(folder, name, dark, light, belly=None, scale_count=900, seed=40, roughness=0.65):
    """A scaly hide: rounded scales (light in their middles, dark between them) over mottled `dark` .. `light`; with
    `belly` the lower half of the texture (V < 0.5) fades to that colour (a smart UV map puts no order into it, so it
    only adds variety)."""
    size = kit.TEXTURE_SIZE
    distance, which = kit.spots(size, scale_count, seed)
    spacing = 0.5 / numpy.sqrt(scale_count)
    dome = numpy.clip(1 - distance / spacing, 0, 1) ** 0.6
    mottle = kit.noise(size, 2.4, seed=seed + 1)
    tone = 0.85 + 0.15 * ((which * 7919) % 13) / 12
    colour = kit.colour_ramp(mottle, dark, light) * (0.72 + 0.28 * dome[..., None]) * tone[..., None]
    if belly is not None:
        v = (numpy.arange(size)[:, None] + 0.5) / size * numpy.ones((1, size))
        fade = numpy.clip((0.5 - v) / 0.2, 0, 1) * kit.noise(size, 2, seed=seed + 2)
        colour = colour * (1 - fade[..., None]) + numpy.array(belly) * fade[..., None]
    return kit.material(name, kit.save_image(folder, name, colour),
                        kit.save_image(folder, name + "_normal", kit.normals_from_height(dome + mottle * 0.3, 3),
                                       colour=False),
                        roughness=roughness)


def plain_materials():
    """The materials every creature has: eye, pupil, horn (bone white), mouth (dark red)."""
    return {"eye": kit.material("eye", (0.95, 0.93, 0.88), roughness=0.15),
            "pupil": kit.material("pupil", (0.04, 0.03, 0.02), roughness=0.1),
            "horn": kit.material("horn", (0.86, 0.8, 0.66), roughness=0.4),
            "mouth": kit.material("mouth", (0.45, 0.12, 0.1), roughness=0.5)}


def wet_eye_material(folder, picture, iris=0.43, pupil=0.2):
    """The material eye: a wet eye on the UVs of a ball of kit.blob turned to look along its +Z (v 1 its front): an
    amber iris of fibres darker at its rim, a black pupil, a white a little yellowed with faint veins, darker towards
    the back (the picture <folder>/<picture>.png); `iris` and `pupil` how far they reach from its front (radians: a
    reptile's eye is most of it iris)."""
    size = 512
    v = (numpy.arange(size)[:, None] + 0.5) / size * numpy.ones((1, size))
    angle = (1 - v) * math.pi
    fibres = kit.noise(size, 1.5, seed=91, stretch=(1, 14))
    veins = numpy.exp(-((kit.noise(size, 1.3, seed=92, stretch=(10, 1)) - 0.5) / 0.02) ** 2) * (angle > 0.7)
    white = numpy.array((0.84, 0.8, 0.72)) * (1 - 0.3 * numpy.clip((angle - 1.2) / 1.4, 0, 1))[..., None]
    white = white * (1 - 0.15 * veins[..., None]) + numpy.array((0.05, 0, 0)) * veins[..., None]
    ring = kit.colour_ramp(fibres, (0.23, 0.12, 0.03), (0.62, 0.42, 0.12))
    ring = ring * (1 - 0.6 * numpy.clip((angle - iris * 0.8) / (iris * 0.2), 0, 1))[..., None]
    colour = numpy.where((angle < iris)[..., None], ring, white)
    colour = numpy.where((angle < pupil)[..., None], numpy.array((0.01, 0.01, 0.01)), colour)
    return kit.material("eye", kit.save_image(folder, picture, colour), roughness=0.05)


def wet_eye(bm, middle, radius, look, slot):
    """A wet eye ball at `middle` looking along `look` (its UVs for wet_eye_material)."""
    made = bmesh.new()
    kit.blob(made, Vector(), (radius,) * 3, segments=24, rings=16, material_index=slot)
    _placed(bm, made, middle, look)


def lid(bm, middle, radius, look, low, slot):
    """An upper lid over an eye at `middle` looking along `look`: the top of a shell of `radius`, down to `low` (of
    the radius, above the eye's middle; below 0 it covers the eye more than half: shut)."""
    made = bmesh.new()
    kit.blob(made, Vector(), (radius,) * 3, segments=48, rings=32, material_index=slot)
    bmesh.ops.rotate(made, cent=Vector(), matrix=Matrix.Rotation(math.pi / 2, 3, "X"), verts=made.verts)   # poles aside
    bmesh.ops.delete(made, geom=[face for face in made.faces if face.calc_center_median().y < low * radius],
                     context="FACES")
    _placed(bm, made, middle, look)


def _placed(bm, made, middle, look):
    """The bmesh `made` (looking along +Z, up +Y) turned to look along `look`, at `middle`, into `bm`."""
    turned = Vector(look).normalized().to_track_quat("Z", "Y").to_matrix().to_4x4()
    bmesh.ops.transform(made, matrix=Matrix.Translation(middle) @ turned, verts=made.verts)
    mesh = bpy.data.meshes.new("part")
    made.to_mesh(mesh)
    made.free()
    if bm.loops.layers.uv.keys():   # its UVs in bm's UV map, not a new one
        mesh.uv_layers[0].name = bm.loops.layers.uv.keys()[0]
    bm.from_mesh(mesh)
    bpy.data.meshes.remove(mesh)


def eye(bm, centre, radius, look, eye_slot, pupil_slot):
    """A cartoon eye at `centre`: a white ball, its pupil on the side towards `look`."""
    centre, look = Vector(centre), Vector(look).normalized()
    kit.blob(bm, centre, (radius, radius, radius), material_index=eye_slot)
    pupil = radius * 0.45
    kit.blob(bm, centre + look * (radius * 0.82), (pupil, pupil, pupil), material_index=pupil_slot,
             segments=10, rings=6)


def cone(bm, base, tip, radius, slot, sides=10, bend=None):
    """A horn or a claw from `base` to `tip` (a tube narrowing to a point), curving towards `bend` (a vector)."""
    base, tip = Vector(base), Vector(tip)
    bend = Vector(bend) if bend is not None else Vector()
    points = [base.lerp(tip, s) + bend * (s * (1 - s)) for s in numpy.linspace(0, 1, 6)]
    radii = [radius * (1 - s) + 0.002 for s in numpy.linspace(0, 1, 6)]
    kit.tube(bm, points, radii, sides=sides, material_index=slot)


def stars(bm, centre, slot, radius=0.45):
    """Five little stars on a ring of `radius` around `centre` (a dizzy creature: a bone turns them about Z)."""
    centre = Vector(centre)
    for index in range(5):
        angle = 2 * math.pi * index / 5
        at = centre + Vector((radius * math.cos(angle), radius * math.sin(angle), 0.05 * math.sin(3 * angle)))
        points = []
        for corner in range(10):
            size = 0.1 if corner % 2 == 0 else 0.045
            a = 2 * math.pi * corner / 10
            points.append(bm.verts.new(at + Vector((size * math.cos(a), 0, size * math.sin(a)))))
        face = bm.faces.new(points)
        face.material_index = slot
        bmesh.ops.solidify(bm, geom=[face], thickness=0.03)


def star_material():
    return kit.material("star", (1.0, 0.85, 0.2), roughness=0.3)


def shrink(pose, bone):
    """`bone` scaled to nothing in `pose` (an ugh_rig.Pose): what only it holds is not seen (stars, eyelids)."""
    pose.scale[bone] = (0.001, 0.001, 0.001)
