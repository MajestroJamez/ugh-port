"""The caveman's skeleton and skin weights (caveman.py).

Bones in rest pose (an A pose, metres, facing -Y, the left side +X): root at the ground between the feet, pelvis,
spine, chest, neck, head, and per side clavicle, upper arm, forearm, hand, thigh, shin, foot. A vertex of the body
or the hide follows the bones nearest to it (weights falling off with the distance to each bone); hair, beard and
eyes follow the head.
"""
import bpy
import numpy
from mathutils import Vector

LEFT_BONES = {
    "clavicle.L": ((0.04, 0.0, 0.78), (0.19, 0.01, 0.79), "chest"),
    "upperarm.L": ((0.2, 0.01, 0.77), (0.35, 0.02, 0.61), "clavicle.L"),
    "forearm.L": ((0.35, 0.02, 0.61), (0.45, -0.02, 0.46), "upperarm.L"),
    "hand.L": ((0.45, -0.02, 0.46), (0.49, -0.04, 0.37), "forearm.L"),
    "thigh.L": ((0.11, 0.0, 0.4), (0.12, -0.02, 0.23), "pelvis"),
    "shin.L": ((0.12, -0.02, 0.23), (0.12, 0.0, 0.075), "thigh.L"),
    "foot.L": ((0.12, 0.0, 0.075), (0.12, -0.12, 0.03), "shin.L"),
}
BONES = {
    "root": ((0, 0, 0), (0, 0, 0.1), None),
    "pelvis": ((0, 0, 0.4), (0, 0, 0.52), "root"),
    "spine": ((0, 0, 0.52), (0, 0, 0.67), "pelvis"),
    "chest": ((0, 0, 0.67), (0, 0, 0.81), "spine"),
    "neck": ((0, 0, 0.81), (0, -0.01, 0.88), "chest"),
    "head": ((0, -0.01, 0.88), (0, -0.01, 1.12), "neck"),
}
for _name, (_head, _tail, _parent) in LEFT_BONES.items():
    BONES[_name] = (_head, _tail, _parent)
for _name, (_head, _tail, _parent) in LEFT_BONES.items():
    BONES[_name.replace(".L", ".R")] = ((-_head[0], *_head[1:]), (-_tail[0], *_tail[1:]), _parent.replace(".L", ".R"))

FALLOFF = 0.025   # metres: a bone this much further away than the nearest one weighs 1/e as much
MOST_BONES = 3
# what the hide may follow (not the arms: its strap sits on the shoulder)
HIDE_BONES = ("pelvis", "spine", "chest", "clavicle.L", "thigh.L", "thigh.R")
RIGID_SLOTS = ("eye", "pupil", "hair_short", "hair_long", "beard")


def build(figure, slots):
    """The armature of BONES with `figure` skinned to it; `slots` names figure's material slots."""
    data = bpy.data.armatures.new("caveman")
    armature = bpy.data.objects.new("caveman_rig", data)
    bpy.context.scene.collection.objects.link(armature)
    bpy.context.view_layer.objects.active = armature
    bpy.ops.object.mode_set(mode="EDIT")
    for name, (head, tail, parent) in BONES.items():
        bone = data.edit_bones.new(name)
        bone.head, bone.tail = head, tail
        if parent:
            bone.parent = data.edit_bones[parent]
            bone.use_connect = (Vector(head) - Vector(BONES[parent][1])).length < 1e-6
    bpy.ops.object.mode_set(mode="OBJECT")
    skin(figure, armature, slots)
    return armature


def skin(figure, armature, slots):
    names = [name for name in BONES if name != "root"]
    heads = numpy.array([BONES[name][0] for name in names])
    tails = numpy.array([BONES[name][1] for name in names])
    groups = {name: figure.vertex_groups.new(name=name) for name in names}
    slot_of = numpy.zeros(len(figure.data.vertices), dtype=numpy.int32)
    for polygon in figure.data.polygons:
        slot_of[list(polygon.vertices)] = polygon.material_index
    hide = [names.index(name) for name in HIDE_BONES]
    for vertex in figure.data.vertices:
        slot = slots[slot_of[vertex.index]]
        if slot in RIGID_SLOTS:
            groups["head"].add([vertex.index], 1.0, "REPLACE")
            continue
        point = numpy.array(vertex.co)
        along = tails - heads
        t = numpy.clip(((point - heads) * along).sum(axis=1) / (along * along).sum(axis=1), 0, 1)
        distance = numpy.linalg.norm(heads + along * t[:, None] - point, axis=1)
        if slot == "fur":
            allowed = numpy.full(len(names), numpy.inf)
            allowed[hide] = distance[hide]
            distance = allowed
        nearest = numpy.argsort(distance)[:MOST_BONES]
        weights = numpy.exp(-(distance[nearest] - distance[nearest[0]]) / FALLOFF)
        for index, weight in zip(nearest, weights / weights.sum()):
            if weight > 0.01:
                groups[names[index]].add([vertex.index], float(weight), "REPLACE")
    figure.parent = armature
    modifier = figure.modifiers.new("rig", "ARMATURE")
    modifier.object = armature
