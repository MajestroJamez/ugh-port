"""What the stone age props of the remake (bones.py, totem.py) share: bleached bone, a bone with knobby ends, skulls
(a cave bear's and a horned dinosaur's) with dark sockets, and a painted wood.
"""
import math

import bmesh
import numpy
from mathutils import Vector

import creature_kit as creature
import ugh_kit as kit


def bone_material(folder):
    """Old bone: bleached ivory with brown dirt in its pores and fine cracks."""
    size = kit.TEXTURE_SIZE
    mottle = kit.noise(size, 2.2, seed=301)
    dirt = numpy.clip((kit.noise(size, 1.6, seed=302) - 0.55) * 3, 0, 1)
    cracks = numpy.clip(1 - numpy.abs(kit.noise(size, 1.4, seed=303) - 0.5) * 60, 0, 1)
    colour = kit.colour_ramp(mottle, (0.72, 0.66, 0.52), (0.93, 0.89, 0.78))
    colour = colour * (1 - 0.45 * dirt[..., None]) * (1 - 0.35 * cracks[..., None])
    return kit.material("bone", kit.save_image(folder, "prop_bone", colour),
                        kit.save_image(folder, "prop_bone_normal", kit.normals_from_height(mottle * 0.4 - cracks * 0.3, 4),
                                       colour=False), roughness=0.6)


def socket_material():
    return kit.material("socket", (0.04, 0.03, 0.025), roughness=0.9)


def bone(bm, a, b, radius, slot):
    """A long bone from `a` to `b`: a shaft and two knobs at each end, across it."""
    a, b = Vector(a), Vector(b)
    axis = (b - a).normalized()
    side = axis.cross(Vector((0, 0, 1)))
    side = side.normalized() if side.length > 1e-3 else Vector((1, 0, 0))
    shaft = [a.lerp(b, s) for s in numpy.linspace(0, 1, 7)]
    kit.tube(bm, shaft, [radius * (1 + 0.5 * (2 * s - 1) ** 4) for s in numpy.linspace(0, 1, 7)], sides=8,
             material_index=slot)
    for end in (a, b):
        for offset in (1, -1):
            knob = radius * 1.25
            kit.blob(bm, end + side * offset * radius * 0.8, (knob, knob, knob), segments=10, rings=6,
                     material_index=slot)


def skull_round(bm, at, size, bone_slot, socket_slot, look=(0, -1, 0)):
    """A cave bear's skull of length `size` lying at `at` (its bottom), its snout towards `look` (horizontal)."""
    at, look = Vector(at), Vector(look).normalized()
    side = look.cross(Vector((0, 0, 1))).normalized()
    s = size / 0.5
    kit.blob(bm, at + Vector((0, 0, 0.13 * s)) - look * 0.06 * s, (0.15 * s, 0.15 * s, 0.13 * s), seed=11, bumps=0.05,
             material_index=bone_slot)
    kit.blob(bm, at + Vector((0, 0, 0.1 * s)) + look * 0.14 * s, (0.08 * s, 0.08 * s, 0.07 * s), seed=12,
             material_index=bone_slot)
    for offset in (1, -1):
        eye = at + Vector((0, 0, 0.17 * s)) + look * 0.07 * s + side * offset * 0.075 * s
        kit.blob(bm, eye, (0.045 * s, 0.045 * s, 0.04 * s), material_index=socket_slot, segments=10, rings=6)
        for tooth in range(3):
            tip = at + Vector((0, 0, 0.04 * s)) + look * (0.2 - 0.03 * tooth) * s + side * offset * 0.04 * s
            creature.cone(bm, tip + Vector((0, 0, 0.03 * s)), tip - Vector((0, 0, 0.02 * s)), 0.012 * s, bone_slot,
                          sides=6)
    kit.blob(bm, at + Vector((0, 0, 0.11 * s)) + look * 0.215 * s, (0.025 * s, 0.02 * s, 0.025 * s),
             material_index=socket_slot, segments=8, rings=6)


def skull_horned(bm, at, size, bone_slot, socket_slot, look=(0, -1, 0)):
    """A triceratops' skull of length `size` lying at `at` (its bottom): a beak towards `look`, three horns, a frill."""
    at, look = Vector(at), Vector(look).normalized()
    side = look.cross(Vector((0, 0, 1))).normalized()
    up = Vector((0, 0, 1))
    s = size / 1.2
    kit.blob(bm, at + up * 0.25 * s, (0.3 * s, 0.3 * s, 0.25 * s), seed=21, bumps=0.08, material_index=bone_slot)
    kit.blob(bm, at + up * 0.2 * s + look * 0.35 * s, (0.14 * s, 0.14 * s, 0.16 * s), seed=22, bumps=0.05,
             material_index=bone_slot)
    beak = at + up * 0.12 * s + look * 0.52 * s
    creature.cone(bm, beak + up * 0.08 * s - look * 0.05 * s, beak, 0.06 * s, bone_slot, bend=-up * 0.1 * s)
    creature.cone(bm, at + up * 0.36 * s + look * 0.38 * s, at + up * 0.55 * s + look * 0.48 * s, 0.05 * s, bone_slot)
    # the frill: a raised half disc behind the head, leaning back, with a scalloped rim
    frill = at + up * 0.35 * s - look * 0.25 * s
    rise = (up * 0.8 - look * 0.6).normalized()
    rim = [frill + (side * math.cos(math.pi * t) + rise * math.sin(math.pi * t)) * 0.5 * s
           for t in numpy.linspace(0, 1, 13)]
    centre = bm.verts.new(frill)
    ring = [bm.verts.new(point) for point in rim]
    faces = [bm.faces.new((centre, a, b)) for a, b in zip(ring, ring[1:])]
    for face in faces:
        face.material_index, face.smooth = bone_slot, True
    bmesh.ops.solidify(bm, geom=faces, thickness=0.06 * s)
    for point in rim[1:-1:2]:
        creature.cone(bm, point, point + (point - frill).normalized() * 0.12 * s, 0.045 * s, bone_slot, sides=6)
    for offset in (1, -1):
        brow = at + up * 0.42 * s + look * 0.12 * s + side * offset * 0.16 * s
        creature.cone(bm, brow, brow + (look * 0.55 + up * 0.25 + side * offset * 0.08) * s, 0.06 * s, bone_slot,
                      bend=up * 0.15 * s)
        eye = at + up * 0.33 * s + look * 0.15 * s + side * offset * 0.22 * s
        kit.blob(bm, eye, (0.06 * s, 0.05 * s, 0.05 * s), material_index=socket_slot, segments=10, rings=6)


def painted_wood_material(folder, name, seed):
    """Weathered wood with stripes of red ochre, yellow ochre and charcoal along V (the length of a pole)."""
    size = kit.TEXTURE_SIZE
    grain = kit.noise(size, 2.0, seed=seed, stretch=(1.0, 12.0))
    wood = kit.colour_ramp(grain, (0.24, 0.15, 0.08), (0.46, 0.32, 0.18))
    v = (numpy.arange(size)[:, None] + 0.5) / size * numpy.ones((1, size))
    paints = numpy.array([(0.55, 0.12, 0.06), (0.78, 0.55, 0.16), (0.06, 0.05, 0.04), (0.85, 0.82, 0.72)])
    band = (v * 10).astype(numpy.int32)
    painted = ((band % 3) != 0) & (kit.noise(size, 1.8, seed=seed + 1) > 0.3)
    colour = numpy.where(painted[..., None], paints[band % 4] * (0.8 + 0.3 * grain[..., None]), wood)
    return kit.material(name, kit.save_image(folder, name, colour),
                        kit.save_image(folder, name + "_normal", kit.normals_from_height(grain, 5), colour=False),
                        roughness=0.75)
