"""The caveman of the remake (the copters' pilots, the passengers): a rigged mesh with its actions as caveman.glb.

A squat, broad cartoon caveman about 1.15 m tall (the original's are 10 px high and 16 px wide), facing -Y: a body of
blended metaballs (belly, big hands and feet, a heavy jaw, a brow ridge, eyes), a hide of leopard fur, and hair in
material slots of their own so the game can show the ones a look wants: hair_short, hair_long, beard. The colours of
hair, beard, fur and skin are glTF base colour factors the game may change (white-ish textures). The rig is in
caveman_rig.py, the actions in caveman_actions.py.

    blender -b --factory-startup --python-exit-code 1 --python caveman.py -- <folder>
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import bmesh  # noqa: E402
import numpy  # noqa: E402
from mathutils import Vector  # noqa: E402

import caveman_actions  # noqa: E402
import caveman_rig as rig  # noqa: E402
import ugh_blobs as blobs  # noqa: E402
import ugh_kit as kit  # noqa: E402

FOLDER = kit.arguments()[0]
RESOLUTION = 0.011
# material slots: (name, default colour of the factor); the order is the game's (UghCopterModel.h)
SLOTS = (("skin", None), ("eye", None), ("pupil", None), ("hair_short", (0.3, 0.17, 0.09)),
         ("hair_long", (0.45, 0.2, 0.08)), ("beard", (0.27, 0.15, 0.08)), ("fur", (1.0, 1.0, 1.0)))


def metaballs(name):
    return blobs.metaballs(name, RESOLUTION)


def body():
    holder, data = metaballs("Body")
    blobs.ellipsoid(data, (0, 0.01, 0.44), (0.17, 0.15, 0.11))   # hips
    blobs.ellipsoid(data, (0, -0.035, 0.55), (0.17, 0.155, 0.13))   # belly
    blobs.ellipsoid(data, (0, 0.0, 0.7), (0.21, 0.15, 0.11))   # chest
    blobs.capsule(data, (0, 0.01, 0.78), (0, -0.01, 0.9), 0.075)   # neck
    blobs.ellipsoid(data, (0, 0.0, 1.0), (0.13, 0.135, 0.14))   # skull
    blobs.ellipsoid(data, (0, -0.055, 0.925), (0.12, 0.1, 0.065))   # jaw
    blobs.capsule(data, (-0.075, -0.112, 1.035), (0.075, -0.112, 1.035), 0.033)   # brow ridge
    blobs.ball(data, (0, -0.15, 0.965), 0.04)   # nose
    for side in blobs.both_sides([(0.052, -0.13, 0.99), (0.135, 0.0, 0.99)]):
        blobs.ball(data, side[0], 0.032, negative=True)   # eye socket
        blobs.ball(data, side[1], 0.03)   # ear
    for shoulder, elbow, wrist, fist, hip, knee, ankle, toes in blobs.both_sides([
            (0.2, 0.01, 0.77), (0.35, 0.02, 0.61), (0.45, -0.02, 0.46), (0.48, -0.035, 0.405),
            (0.11, 0.0, 0.4), (0.12, -0.02, 0.23), (0.12, 0.0, 0.075), (0.12, -0.085, 0.035)]):
        blobs.ball(data, shoulder, 0.085)
        blobs.capsule(data, shoulder, elbow, 0.062)
        blobs.capsule(data, elbow, wrist, 0.056)
        blobs.ellipsoid(data, fist, (0.05, 0.055, 0.06))
        blobs.ball(data, (fist[0] - math.copysign(0.035, fist[0]), fist[1] - 0.035, fist[2] + 0.025), 0.022)   # thumb
        blobs.capsule(data, hip, knee, 0.075)
        blobs.capsule(data, knee, ankle, 0.058)
        blobs.ellipsoid(data, toes, (0.055, 0.085, 0.035))
    return holder


def hair(name, long_hair):
    holder, data = metaballs(name)
    rng = numpy.random.default_rng(3 if long_hair else 1)
    for _ in range(60):
        theta, phi = rng.uniform(0, 1.2), rng.uniform(0, 2 * math.pi)
        if math.cos(phi) < -0.6 and theta > 0.75:
            continue   # the face stays free
        direction = Vector((math.sin(theta) * math.sin(phi), math.sin(theta) * -math.cos(phi), math.cos(theta)))
        blobs.ball(data, Vector((0, 0.01, 1.0)) + direction * 0.135, rng.uniform(0.04, 0.06))
    if long_hair:
        for x in numpy.linspace(-0.14, 0.14, 7):
            blobs.capsule(data, (x, 0.08, 1.02), (x * 1.3, 0.12, 0.74), 0.045)
    return holder


def beard():
    holder, data = metaballs("Beard")
    rng = numpy.random.default_rng(5)
    for _ in range(36):
        angle = rng.uniform(-1.4, 1.4)
        z = rng.uniform(0.82, 0.92)
        reach = 0.125 + 0.03 * (0.92 - z) / 0.1
        blobs.ball(data, (math.sin(angle) * reach, -math.cos(angle) * reach - 0.03, z), rng.uniform(0.035, 0.05))
    blobs.ball(data, (0, -0.155, 0.81), 0.055)   # the beard's point on the chest
    return holder


def fur():
    """A hide around the hips and belly with a strap over the left shoulder: jagged hem, open top."""
    holder, data = metaballs("Fur")
    blobs.ellipsoid(data, (0, 0.01, 0.43), (0.195, 0.175, 0.13))
    blobs.ellipsoid(data, (0, -0.035, 0.56), (0.19, 0.175, 0.14))
    blobs.capsule(data, (0.11, -0.12, 0.66), (0.19, 0.0, 0.84), 0.04)
    blobs.capsule(data, (0.19, 0.0, 0.84), (0.11, 0.12, 0.66), 0.04)
    return holder


def cut_fur(bm):
    doomed = []
    for face in bm.faces:
        c = face.calc_center_median()
        angle = math.atan2(c.y, c.x)
        hem = 0.29 + 0.025 * math.sin(7 * angle) + 0.012 * math.sin(17 * angle + 1)
        top = 0.63 + 0.12 * c.x
        on_strap = c.x > 0.08 and c.z > 0.6
        if c.z < hem or (c.z > top and not on_strap):
            doomed.append(face)
    bmesh.ops.delete(bm, geom=doomed, context="FACES")


def eyes():
    bm = bmesh.new()
    for x in (0.052, -0.052):
        kit.blob(bm, Vector((x, -0.118, 0.99)), (0.031, 0.031, 0.031), material_index=1)
        kit.blob(bm, Vector((x * 1.08, -0.147, 0.988)), (0.014, 0.007, 0.015), material_index=2, segments=10, rings=6)
    return bm


def to_mesh(holder, slot, materials_of_slots, decimate=0.5):
    """The metaballs as a mesh object in the materials of SLOTS, all its faces in slot `slot`."""
    return blobs.to_mesh(holder, [name for name, _ in SLOTS].index(slot), materials_of_slots, decimate,
                         cut_fur if slot == "fur" else None)


def textures():
    size = kit.TEXTURE_SIZE
    mottle = kit.noise(size, 2.6, seed=11)
    pores = kit.noise(size, 1.6, seed=12)
    skin = kit.colour_ramp(mottle, (0.76, 0.54, 0.41), (0.84, 0.61, 0.48)) * (0.94 + 0.06 * pores[..., None])
    strands = kit.noise(size, 1.6, seed=13, stretch=(10, 1))
    hair = kit.colour_ramp(strands, (0.55, 0.55, 0.55), (1.0, 1.0, 1.0))
    distance, which = kit.spots(size, 220, seed=14)
    radius = 0.016 + 0.01 * (which % 5) / 4 + 0.008 * kit.noise(size, 2, seed=17)
    ring = (numpy.abs(distance - radius) < 0.006 + 0.005 * kit.noise(size, 2, seed=15)).astype(numpy.float32)
    centre = (distance < radius - 0.003).astype(numpy.float32)
    fibres = kit.noise(size, 1.4, seed=16, stretch=(1, 8))
    hide = kit.colour_ramp(fibres, (0.78, 0.6, 0.34), (0.92, 0.75, 0.47))
    hide = hide * (1 - 0.3 * centre[..., None]) * (1 - 0.72 * ring[..., None])
    return {
        "skin": (kit.save_image(FOLDER, "caveman_skin", skin),
                 kit.save_image(FOLDER, "caveman_skin_normal",
                                kit.normals_from_height(mottle * 0.5 + pores * 0.5, 0.35), colour=False)),
        "hair": (kit.save_image(FOLDER, "caveman_hair", hair),
                 kit.save_image(FOLDER, "caveman_hair_normal", kit.normals_from_height(strands, 6), colour=False)),
        "fur": (kit.save_image(FOLDER, "caveman_fur", hide),
                kit.save_image(FOLDER, "caveman_fur_normal", kit.normals_from_height(fibres + ring * 0.1, 4),
                               colour=False)),
    }


def materials():
    made = textures()
    skin, hair, hide = made["skin"], made["hair"], made["fur"]
    by_slot = {
        "skin": kit.material("skin", skin[0], skin[1], roughness=0.6),
        "eye": kit.material("eye", (0.95, 0.93, 0.88), roughness=0.15),
        "pupil": kit.material("pupil", (0.04, 0.03, 0.02), roughness=0.1),
        "fur": kit.material("fur", hide[0], hide[1], roughness=0.85, double_sided=True, tint=(1, 1, 1)),
    }
    for name, colour in SLOTS:
        if name not in by_slot:
            by_slot[name] = kit.material(name, hair[0], hair[1], roughness=0.75, tint=colour)
    return [by_slot[name] for name, _ in SLOTS]


def build():
    kit.clear_scene()
    slots = materials()
    parts = [to_mesh(body(), "skin", slots, 0.6), to_mesh(hair("HairShort", False), "hair_short", slots),
             to_mesh(hair("HairLong", True), "hair_long", slots), to_mesh(beard(), "beard", slots),
             to_mesh(fur(), "fur", slots, 0.7), kit.mesh_object("Eyes", eyes(), slots)]
    figure = blobs.join(parts, "caveman")
    armature = rig.build(figure)
    caveman_actions.make(armature)
    kit.export(os.path.join(FOLDER, "caveman.glb"), [armature, figure])


build()
