"""What the enemies of the remake (pterodactyl.py, triceratops.py, blower.py, fruit_tree.py) share: a scaly hide,
a horn or claw, cartoon eyes (a white ball with a pupil), stars over a dizzy head, and their materials.
"""
import math

import bmesh
import numpy
from mathutils import Vector

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
