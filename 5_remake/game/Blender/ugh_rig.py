"""Rigs of the remake's animated models (caveman, pterodactyl, triceratops, blower, tree): an armature from a table of
bones, skin weights by the distance to the bones, and actions keyed from poses.

A table of bones maps a name to (head, tail, parent) in rest pose (metres, the model's axes), parents before their
children. A pose says where bones point in the armature's space (`move` a head, `turn` about the head, `aim` along a
direction, `reach` a two-bone chain for a target, `scale` a bone); the bones' local transforms are keyed from it. Every
action is a loop (its last frame is its first) on an NLA track of its own, so the glTF exporter takes them all.
"""
import math

import bpy
import numpy
from mathutils import Matrix, Quaternion, Vector

FPS = 24


def build(name, bones, figure, slot_bones=None, slot_allowed=None, falloff=0.025, most=3):
    """The armature `name` of `bones` with `figure` skinned to it: a vertex follows the bones nearest to it (weights
    falling off by `falloff` metres per metre further away, at most `most` bones); the vertices of a material slot in
    `slot_bones` (slot name -> bone) follow only that bone, those of a slot in `slot_allowed` (slot name -> bones) only
    those bones. A bone named root moves the whole and holds no vertex."""
    data = bpy.data.armatures.new(name)
    armature = bpy.data.objects.new(name + "_rig", data)
    bpy.context.scene.collection.objects.link(armature)
    bpy.context.view_layer.objects.active = armature
    bpy.ops.object.mode_set(mode="EDIT")
    for bone_name, (head, tail, parent) in bones.items():
        bone = data.edit_bones.new(bone_name)
        bone.head, bone.tail = head, tail
        if parent:
            bone.parent = data.edit_bones[parent]
            bone.use_connect = (Vector(head) - Vector(bones[parent][1])).length < 1e-6
    bpy.ops.object.mode_set(mode="OBJECT")
    skin(figure, armature, bones, slot_bones or {}, slot_allowed or {}, falloff, most)
    return armature


def skin(figure, armature, bones, slot_bones, slot_allowed, falloff, most):
    names = [name for name in bones if name != "root"]
    heads = numpy.array([bones[name][0] for name in names])
    tails = numpy.array([bones[name][1] for name in names])
    along = tails - heads
    groups = {name: figure.vertex_groups.new(name=name) for name in names}
    slots = [slot.material.name if slot.material else "" for slot in figure.material_slots]
    slot_of = numpy.zeros(len(figure.data.vertices), dtype=numpy.int32)
    for polygon in figure.data.polygons:
        slot_of[list(polygon.vertices)] = polygon.material_index
    for vertex in figure.data.vertices:
        slot = slots[slot_of[vertex.index]] if slots else ""
        if slot in slot_bones:
            groups[slot_bones[slot]].add([vertex.index], 1.0, "REPLACE")
            continue
        point = numpy.array(vertex.co)
        t = numpy.clip(((point - heads) * along).sum(axis=1) / (along * along).sum(axis=1), 0, 1)
        distance = numpy.linalg.norm(heads + along * t[:, None] - point, axis=1)
        if slot in slot_allowed:
            allowed = numpy.full(len(names), numpy.inf)
            indices = [names.index(name) for name in slot_allowed[slot] if name in names]
            allowed[indices] = distance[indices]
            distance = allowed
        nearest = numpy.argsort(distance)[:most]
        weights = numpy.exp(-(distance[nearest] - distance[nearest[0]]) / falloff)
        for index, weight in zip(nearest, weights / weights.sum()):
            if weight > 0.01:
                groups[names[index]].add([vertex.index], float(weight), "REPLACE")
    figure.parent = armature
    modifier = figure.modifiers.new("rig", "ARMATURE")
    modifier.object = armature


class Pose:
    """What a frame wants: `move` a bone's head somewhere, `turn` a bone (a rotation in the armature's space about its
    head), `aim` a bone along a direction, `reach` a two-bone chain (upper bone -> (target of the lower one's tail, the
    direction its joint bends to)), `scale` a bone (its own axes)."""

    def __init__(self):
        self.move, self.turn, self.aim, self.reach, self.scale = {}, {}, {}, {}, {}


def solve(armature, bones, pose):
    """The bones' matrices in the armature's space for `pose` (without scale)."""
    rest_bones = armature.data.bones
    posed = {}
    for name, (_, _, parent) in bones.items():
        rest = rest_bones[name].matrix_local
        matrix = rest.copy() if parent is None else posed[parent] @ rest_bones[parent].matrix_local.inverted() @ rest
        head = matrix.to_translation()
        if name in pose.move:
            head = Vector(pose.move[name])
        rotation = matrix.to_quaternion()
        if name in pose.turn:
            rotation = pose.turn[name] @ rotation
        if name in pose.reach:
            target, bend = pose.reach[name]
            child = next(c for c, (_, _, p) in bones.items() if p == name)
            joint = two_bone(head, Vector(target), rest_bones[name].length, rest_bones[child].length, Vector(bend))
            pose.aim[name] = joint - head
            pose.aim[child] = Vector(target) - joint
        if name in pose.aim:
            rotation = (rotation @ Vector((0, 1, 0))).rotation_difference(pose.aim[name]) @ rotation
        posed[name] = Matrix.Translation(head) @ rotation.to_matrix().to_4x4()
    return posed


def two_bone(start, target, upper, lower, bend):
    """Where the joint of a two-bone chain from `start` reaching for `target` is, bent towards `bend`."""
    way = target - start
    reach = min(max(way.length, abs(upper - lower) + 1e-4), upper + lower - 1e-4)
    axis = way.normalized()
    along = (upper * upper - lower * lower + reach * reach) / (2 * reach)
    out = (bend - axis * bend.dot(axis)).normalized()
    return start + axis * along + out * math.sqrt(max(upper * upper - along * along, 0))


def key(armature, bones, posed, scales, frame, last):
    """Keys the local transforms that make `posed` (and `scales`) at `frame`; `last` keeps quaternions continuous."""
    rest_bones = armature.data.bones
    for name, (_, _, parent) in bones.items():
        rest = rest_bones[name].matrix_local
        if parent is None:
            basis = rest.inverted() @ posed[name]
        else:
            local_rest = rest_bones[parent].matrix_local.inverted() @ rest
            basis = local_rest.inverted() @ posed[parent].inverted() @ posed[name]
        bone = armature.pose.bones[name]
        bone.rotation_mode = "QUATERNION"
        rotation = basis.to_quaternion()
        if name in last and last[name].dot(rotation) < 0:
            rotation.negate()
        last[name] = rotation
        bone.rotation_quaternion = rotation
        bone.location = basis.to_translation()
        bone.scale = Vector(scales.get(name, (1, 1, 1)))
        for path in ("rotation_quaternion", "location", "scale"):
            bone.keyframe_insert(path, frame=frame)


def turn(x=0.0, y=0.0, z=0.0):
    """A rotation about the armature's axes (radians), X first."""
    return Quaternion((0, 0, 1), z) @ Quaternion((0, 1, 0), y) @ Quaternion((1, 0, 0), x)


def make(armature, bones, actions):
    """Every action of `actions` (name -> (posing(t) -> Pose for t 0 .. 2 pi, frames)) on the armature."""
    bpy.context.scene.render.fps = FPS
    armature.animation_data_create()
    for name, (posing, frames) in actions.items():
        action = bpy.data.actions.new(name)
        armature.animation_data.action = action
        last = {}
        for frame in range(frames + 1):
            pose = posing(2 * math.pi * frame / frames)
            key(armature, bones, solve(armature, bones, pose), pose.scale, frame, last)
        track = armature.animation_data.nla_tracks.new()
        track.name = name
        track.strips.new(name, 0, action)
        armature.animation_data.action = None
    for bone in armature.pose.bones:
        bone.rotation_quaternion, bone.location, bone.scale = (1, 0, 0, 0), (0, 0, 0), (1, 1, 1)


def mirrored(left_bones):
    """The bones of `left_bones` (names ending in .L, +X) and their mirror images (.R)."""
    both = {}
    for name, (head, tail, parent) in left_bones.items():
        both[name] = (head, tail, parent)
    for name, (head, tail, parent) in left_bones.items():
        right_parent = parent.replace(".L", ".R") if parent else None
        both[name.replace(".L", ".R")] = ((-head[0], *head[1:]), (-tail[0], *tail[1:]), right_parent)
    return both


def sides():
    """(+1, "L"), (-1, "R"): the sign of X and the suffix of the bones of each side."""
    return ((1, "L"), (-1, "R"))
