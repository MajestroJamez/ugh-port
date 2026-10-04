"""The tree of the remake (the original's swaying tree with a face, which drops a fruit when a passenger bounces off
it): a rigged mesh with its actions as fruit_tree.glb.

A stout cartoon tree about 2.5 m high and 3 m across its crown (the original's sprite is 32 x 24 px), facing -Y, its
origin on the ground at the foot of its trunk: a gnarled trunk with roots in bark (the texture set palm_bark), a face
on it (eyes, a knot of a nose, a mouth; eyelids that only the bone lids shows), branches, a crown of leafy lumps
(foliage) covered with leaves (leaf, cut out) and a few red fruits. Actions (each a loop):

- sway: the crown sways, the leaves rustle; it blinks at the end of the loop (the original's 14 sprites a loop show
  the eyes shut in the last one),
- shaken: the eyes shut, the crown shaking (a passenger bounced off it; also when it has no fruit left).

    blender -b --factory-startup --python-exit-code 1 --python fruit_tree.py -- <folder> <palm_bark folder>
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import bmesh  # noqa: E402
import numpy  # noqa: E402
from mathutils import Quaternion, Vector  # noqa: E402

import creature_kit as creature  # noqa: E402
import ugh_blobs as blobs  # noqa: E402
import ugh_kit as kit  # noqa: E402
import ugh_rig  # noqa: E402
from ugh_rig import Pose, turn  # noqa: E402

FOLDER, BARK = kit.arguments()[:2]
RESOLUTION = 0.03
SLOTS = ("bark", "foliage", "leaf", "fruit", "lid", "eye", "pupil", "mouth")
BLINK = 13 / 14   # of the sway loop: the eyes are shut from here on
# the lumps of the crown: middle, radius
LUMPS = (((0, 0.05, 2.0), 0.6), ((0.72, 0.05, 1.82), 0.45), ((-0.72, 0.05, 1.82), 0.45), ((0.38, 0.12, 2.3), 0.42),
         ((-0.38, 0.12, 2.3), 0.42), ((1.1, 0.0, 1.62), 0.32), ((-1.1, 0.0, 1.62), 0.32))
EYES = ((0.11, -0.25, 0.97), (-0.11, -0.25, 0.97))
BONES = {
    "root": ((0, 0, 0), (0, 0, 0.1), None),
    "trunk": ((0, 0, 0.1), (0, 0, 0.9), "root"),
    "upper": ((0, 0, 0.9), (0, 0, 1.5), "trunk"),
    "crown": ((0, 0, 1.5), (0, 0, 2.5), "upper"),
    "lids": ((0, -0.3, 0.97), (0, -0.3, 1.07), "trunk"),
    **ugh_rig.mirrored({"branch.L": ((0.1, 0, 1.15), (1.05, 0, 1.7), "upper")}),
}
CROWN_BONES = ("upper", "crown", "branch.L", "branch.R")


def materials():
    plain = creature.plain_materials()
    size = kit.TEXTURE_SIZE
    u = (numpy.arange(size)[None, :] + 0.5) / size * numpy.ones((size, 1))
    v = (numpy.arange(size)[:, None] + 0.5) / size * numpy.ones((1, size))
    # a leaf: U across, V along; pointed at both ends, a midrib, cut out around it
    half = 0.48 * numpy.sin(numpy.pi * v) ** 0.8
    inside = (numpy.abs(u - 0.5) < half).astype(numpy.float32)
    rib = numpy.clip(1 - numpy.abs(u - 0.5) / 0.015, 0, 1)
    cells = kit.noise(size, 1.8, seed=81)
    leaf = kit.colour_ramp(cells, (0.2, 0.42, 0.1), (0.42, 0.66, 0.2)) + rib[..., None] * 0.15
    clumps = kit.noise(size, 1.2, seed=82)
    foliage = kit.colour_ramp(clumps, (0.1, 0.24, 0.05), (0.26, 0.46, 0.12))
    made = {
        "bark": kit.poly_haven_material("bark", BARK),
        "foliage": kit.material("foliage", kit.save_image(FOLDER, "tree_foliage", foliage),
                                kit.save_image(FOLDER, "tree_foliage_normal", kit.normals_from_height(clumps, 6),
                                               colour=False), roughness=0.8),
        "leaf": kit.material("leaf", kit.save_image(FOLDER, "tree_leaf", numpy.dstack((leaf, inside))),
                             roughness=0.6, alpha_cutoff=0.5, double_sided=True),
        "fruit": kit.material("fruit", (0.85, 0.12, 0.08), roughness=0.3),
        "lid": kit.material("lid", (0.36, 0.25, 0.16), roughness=0.8),
        **plain}
    return [made[name] for name in SLOTS]


def trunk():
    holder, data = blobs.metaballs("Trunk", RESOLUTION)
    blobs.limb(data, [(0, 0, 0.05), (0, 0.02, 0.6), (0.03, 0, 1.15), (0, 0, 1.75)], (0.34, 0.26, 0.15))
    for angle in (0.4, 1.9, 3.3, 4.8):   # roots
        blobs.limb(data, [(0, 0, 0.3), (0.42 * math.cos(angle), 0.3 * math.sin(angle), 0.05),
                          (0.62 * math.cos(angle), 0.45 * math.sin(angle), 0.0)], (0.14, 0.07))
    for side in (1, -1):
        blobs.limb(data, [(side * 0.1, 0, 1.12), (side * 0.65, 0.05, 1.5), (side * 1.05, 0, 1.68)], (0.12, 0.07))
    blobs.ball(data, (0, -0.28, 0.83), 0.06)   # the nose, a knot
    blobs.ellipsoid(data, (0, -0.26, 0.68), (0.12, 0.08, 0.05), negative=True)   # the mouth
    for eye in EYES:
        blobs.ball(data, Vector(eye) + Vector((0, 0.03, 0)), 0.075, negative=True)   # its socket
    return holder


def crown():
    holder, data = blobs.metaballs("Crown", RESOLUTION)
    for middle, radius in LUMPS:
        blobs.ball(data, middle, radius * 0.85)
    return holder


def leaves(bm):
    """Leaves on the lumps of the crown, facing out, and the fruits hanging among them."""
    rng = numpy.random.default_rng(83)
    uv = bm.loops.layers.uv.verify()
    slot = SLOTS.index("leaf")
    for middle, radius in LUMPS:
        for _ in range(int(160 * radius * radius)):
            out = Vector(rng.normal(size=3)).normalized()
            if out.z < -0.6:
                continue
            at = Vector(middle) + out * radius * rng.uniform(0.85, 1.05)
            along = out.orthogonal().normalized()
            along.rotate(Quaternion(out, rng.uniform(0, 2 * math.pi)))
            across = out.cross(along).normalized() * 0.09
            along = along * 0.15
            corners = [at - along - across, at - along + across, at + along + across, at + along - across]
            face = bm.faces.new([bm.verts.new(c) for c in corners])
            face.material_index, face.smooth = slot, True
            for loop, coord in zip(face.loops, ((0, 0), (1, 0), (1, 1), (0, 1))):
                loop[uv].uv = coord
    for at in ((0.5, -0.5, 1.68), (-0.62, -0.42, 1.86), (0.12, -0.6, 2.08), (-0.25, -0.55, 1.6), (0.95, -0.3, 1.5)):
        kit.blob(bm, Vector(at), (0.09, 0.09, 0.095), material_index=SLOTS.index("fruit"))


def face(bm):
    for eye in EYES:
        creature.eye(bm, eye, 0.068, (0, -1, 0), SLOTS.index("eye"), SLOTS.index("pupil"))
        kit.blob(bm, Vector(eye) + Vector((0, -0.012, 0.004)), (0.078, 0.06, 0.078), material_index=SLOTS.index("lid"))
    kit.blob(bm, Vector((0, -0.24, 0.68)), (0.1, 0.03, 0.04), material_index=SLOTS.index("mouth"))


def sway(t):
    pose = Pose()
    pose.turn["upper"] = turn(y=0.04 * math.sin(t))
    pose.turn["crown"] = turn(y=0.06 * math.sin(t - 0.5), x=0.02 * math.sin(2 * t))
    for side, name in ugh_rig.sides():
        pose.turn[f"branch.{name}"] = turn(y=0.03 * math.sin(t - 0.8) - side * 0.02 * math.sin(2 * t))
    if t / (2 * math.pi) < BLINK - 1e-6:
        creature.shrink(pose, "lids")
    return pose


def shaken(t):
    pose = Pose()
    pose.turn["trunk"] = turn(y=0.015 * math.sin(4 * t))
    pose.turn["upper"] = turn(y=0.05 * math.sin(4 * t + 0.5))
    pose.turn["crown"] = turn(y=0.08 * math.sin(4 * t + 1), x=0.03 * math.sin(6 * t))
    return pose


ACTIONS = {"sway": (sway, 56), "shaken": (shaken, 24)}


def build():
    kit.clear_scene()
    slots = materials()
    bm = bmesh.new()
    leaves(bm)
    face(bm)
    figure = blobs.join([blobs.to_mesh(trunk(), SLOTS.index("bark"), slots, 0.4),
                         blobs.to_mesh(crown(), SLOTS.index("foliage"), slots, 0.35),
                         kit.mesh_object("Leaves", bm, slots)], "fruit_tree")
    armature = ugh_rig.build("fruit_tree", BONES, figure, falloff=0.1,
                             slot_bones={"lid": "lids", "eye": "trunk", "pupil": "trunk", "mouth": "trunk"},
                             slot_allowed={"foliage": CROWN_BONES, "leaf": CROWN_BONES, "fruit": CROWN_BONES,
                                           "bark": [name for name in BONES if name != "lids"]})
    ugh_rig.make(armature, BONES, ACTIONS)
    kit.export(os.path.join(FOLDER, "fruit_tree.glb"), [armature, figure])


build()
