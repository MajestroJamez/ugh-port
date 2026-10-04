"""The caveman's actions (caveman.py), each a loop (its last frame is its first), at FPS frames a second:

- idle: standing (the origin on the ground between the feet), breathing, looking about,
- sit: sitting (the origin is the seat), hands on the knees, the feet dangling,
- pedal: sitting and pedalling the copter's crank (copter_layout: the axle and the pedals from the seat), holding its
  handles; one turn of the crank per loop, the left pedal on top at the start, its top going forward,
- hang: hanging from a rope by both hands (the origin is where they hold it), the legs swinging.

A pose says where bones point in the armature's space (aims, turns, two-bone IK for arms and legs); the bones' local
rotations are keyed from it.
"""
import math

import bpy
from mathutils import Matrix, Quaternion, Vector

import caveman_rig as rig
import copter_layout as layout

FPS = 24


class Pose:
    """What a frame wants: `move` a bone's head somewhere, `turn` a bone (a rotation in the armature's space about
    its head), `aim` a bone along a direction, `reach` a two-bone chain (upper bone -> (target of the lower one's
    tail, the direction its joint bends to))."""

    def __init__(self):
        self.move, self.turn, self.aim, self.reach = {}, {}, {}, {}


def solve(armature, pose):
    """The bones' matrices in the armature's space for `pose`."""
    bones = armature.data.bones
    posed = {}
    for name, (_, _, parent) in rig.BONES.items():
        rest = bones[name].matrix_local
        matrix = rest.copy() if parent is None else posed[parent] @ bones[parent].matrix_local.inverted() @ rest
        head = matrix.to_translation()
        if name in pose.move:
            head = Vector(pose.move[name])
        rotation = matrix.to_quaternion()
        if name in pose.turn:
            rotation = pose.turn[name] @ rotation
        if name in pose.reach:
            target, bend = pose.reach[name]
            child = next(c for c, (_, _, p) in rig.BONES.items() if p == name)
            joint = two_bone(head, Vector(target), bones[name].length, bones[child].length, Vector(bend))
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


def key(armature, posed, frame, last):
    """Keys the local rotations (and locations) that make `posed` at `frame`; `last` keeps quaternions continuous."""
    bones = armature.data.bones
    for name, (_, _, parent) in rig.BONES.items():
        rest = bones[name].matrix_local
        if parent is None:
            basis = rest.inverted() @ posed[name]
        else:
            basis = (bones[parent].matrix_local.inverted() @ rest).inverted() @ posed[parent].inverted() @ posed[name]
        bone = armature.pose.bones[name]
        bone.rotation_mode = "QUATERNION"
        rotation = basis.to_quaternion()
        if name in last and last[name].dot(rotation) < 0:
            rotation.negate()
        last[name] = rotation
        bone.rotation_quaternion = rotation
        bone.location = basis.to_translation()
        bone.keyframe_insert("rotation_quaternion", frame=frame)
        bone.keyframe_insert("location", frame=frame)


def turn(x=0.0, y=0.0, z=0.0):
    """A rotation about the armature's axes (radians), X first."""
    return Quaternion((0, 0, 1), z) @ Quaternion((0, 1, 0), y) @ Quaternion((1, 0, 0), x)


def head_of(name):
    return Vector(rig.BONES[name][0])


def idle(t):
    pose = Pose()
    pose.move["pelvis"] = head_of("pelvis") + Vector((0.006 * math.sin(t), 0, -0.004 * (1 - math.cos(2 * t))))
    pose.turn["pelvis"] = turn(y=0.03 * math.sin(t))
    pose.turn["chest"] = turn(x=-0.03 * math.sin(2 * t), y=-0.04 * math.sin(t))
    pose.turn["head"] = turn(z=0.25 * math.sin(t), x=0.05 * math.sin(2 * t))
    for side in (1, -1):
        pose.reach[f"upperarm.{'L' if side > 0 else 'R'}"] = (
            (side * (0.33 + 0.01 * math.sin(t)), -0.03, 0.43 + 0.01 * math.sin(2 * t)), (side * 0.5, 0.6, 0))
        pose.reach[f"thigh.{'L' if side > 0 else 'R'}"] = ((side * 0.12, 0.0, 0.075), (0, -1, 0))
        pose.aim[f"hand.{'L' if side > 0 else 'R'}"] = Vector((side * 0.1, -0.2, -1))
        pose.aim[f"foot.{'L' if side > 0 else 'R'}"] = Vector((side * 0.15, -1, -0.3))
    return pose


def seated(pose, lean):
    """The hips on the seat (the origin), the body leaning forward by `lean` radians."""
    pose.move["pelvis"] = Vector((0, 0.03, 0.08))
    pose.turn["pelvis"] = turn(x=lean)


def sit(t):
    pose = Pose()
    seated(pose, 0.05)
    pose.turn["chest"] = turn(x=0.03 * math.sin(t))
    pose.turn["head"] = turn(z=0.3 * math.sin(t), x=-0.05)
    for side, name in ((1, "L"), (-1, "R")):
        knee = Vector((side * 0.12, -0.15, 0.08))
        swing = 0.05 * math.sin(t + (0 if side > 0 else math.pi))
        pose.reach[f"thigh.{name}"] = (knee + Vector((0, -0.04 + swing, -0.15)), (0, -1, 0.6))
        pose.aim[f"foot.{name}"] = Vector((0, -1, -0.6))
        pose.reach[f"upperarm.{name}"] = (knee + Vector((side * 0.03, 0.03, 0.08)), (side * 0.6, 0.6, 0))
        pose.aim[f"hand.{name}"] = Vector((side * 0.2, -1, -0.8))
    return pose


def pedal(t):
    pose = Pose()
    seated(pose, 0.15)
    pose.turn["pelvis"] = turn(x=0.15, y=0.05 * math.sin(t))
    pose.turn["chest"] = turn(x=0.05, y=-0.06 * math.sin(t))
    pose.turn["head"] = turn(x=-0.2 + 0.04 * math.sin(2 * t))
    axle = Vector(layout.PEDAL_AXLE)
    for side, name, phase in ((1, "L", 0.0), (-1, "R", math.pi)):
        angle = t + phase
        pedal_at = axle + Vector((side * layout.PEDAL_SPREAD, -layout.PEDAL_RADIUS * math.sin(angle),
                                  layout.PEDAL_RADIUS * math.cos(angle)))
        ankle = pedal_at + Vector((0, 0.035, 0.045))
        pose.reach[f"thigh.{name}"] = (ankle, (0, -1, 0.5))
        pose.aim[f"foot.{name}"] = Vector((0, -1, -0.25 - 0.2 * math.sin(angle)))
        grip = Vector((side * layout.GRIP[0], layout.GRIP[1], layout.GRIP[2]))
        pose.reach[f"upperarm.{name}"] = (grip, (side * 0.7, 0.5, -0.3))
        pose.aim[f"hand.{name}"] = Vector((side * 0.1, -1, -0.2))
    return pose


def hang(t):
    pose = Pose()
    swing = 0.08 * math.sin(t)
    pose.move["pelvis"] = Vector((0, 0.02 * math.sin(t), -0.84))
    pose.turn["pelvis"] = turn(x=swing)
    pose.turn["head"] = turn(x=0.25, z=0.2 * math.sin(t))
    for side, name, phase in ((1, "L", 0.0), (-1, "R", math.pi)):
        pose.reach[f"upperarm.{name}"] = ((side * 0.08, 0, -0.085), (side * 1, 0.3, 0))
        pose.aim[f"hand.{name}"] = Vector((-side * 0.2, 0, 1))
        kick = 0.06 * math.sin(2 * t + phase)
        pose.reach[f"thigh.{name}"] = ((side * 0.14, -0.06 + kick, -1.13), (0, -1, 0))
        pose.aim[f"foot.{name}"] = Vector((side * 0.2, -1, -1))
    return pose


ACTIONS = {"idle": (idle, 48), "sit": (sit, 48), "pedal": (pedal, 24), "hang": (hang, 48)}


def make(armature):
    """Every action of ACTIONS on the armature, each on an NLA track of its own (the glTF exporter takes them all)."""
    bpy.context.scene.render.fps = FPS
    armature.animation_data_create()
    for name, (posing, frames) in ACTIONS.items():
        action = bpy.data.actions.new(name)
        armature.animation_data.action = action
        last = {}
        for frame in range(frames + 1):
            key(armature, solve(armature, posing(2 * math.pi * frame / frames)), frame, last)
        track = armature.animation_data.nla_tracks.new()
        track.name = name
        track.strips.new(name, 0, action)
        armature.animation_data.action = None
    for bone in armature.pose.bones:
        bone.rotation_quaternion, bone.location = (1, 0, 0, 0), (0, 0, 0)
