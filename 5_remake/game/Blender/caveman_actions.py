"""The caveman's actions (caveman.py, ugh_rig.py), each a loop (its last frame is its first). He faces -Y; where his
origin is:

- idle: standing (the origin on the ground between the feet), breathing, looking about,
- sit: sitting (the origin is the seat), hands on the knees, the feet dangling,
- pedal: sitting and pedalling the copter's crank (copter_layout: the axle and the pedals from the seat), holding its
  handles; one turn of the crank per loop, the left pedal on top at the start, its top going forward,
- hang: hanging from a rope by both hands (the origin is where they hold it), the legs swinging,
- walk, wave: on the ground as idle; walk is two steps forward (the left foot forward at the start), wave calls a
  copter with both arms over the head,
- tread, swim, fall: in the water (the origin at its surface, his chin just above it): treading water, swimming
  forward (his head up), falling or sinking with the arms up.
"""
import math

from mathutils import Vector

import caveman_rig as rig
import copter_layout as layout
import ugh_rig
from ugh_rig import Pose, sides, turn

# the water's surface (the origin of tread, swim, fall) is this far above the pelvis of the upright figure
UNDER_WATER = 0.42


def head_of(name):
    return Vector(rig.BONES[name][0])


def idle(t):
    pose = Pose()
    pose.move["pelvis"] = head_of("pelvis") + Vector((0.006 * math.sin(t), 0, -0.004 * (1 - math.cos(2 * t))))
    pose.turn["pelvis"] = turn(y=0.03 * math.sin(t))
    pose.turn["chest"] = turn(x=-0.03 * math.sin(2 * t), y=-0.04 * math.sin(t))
    pose.turn["head"] = turn(z=0.25 * math.sin(t), x=0.05 * math.sin(2 * t))
    for side, name in sides():
        pose.reach[f"upperarm.{name}"] = (
            (side * (0.33 + 0.01 * math.sin(t)), -0.03, 0.43 + 0.01 * math.sin(2 * t)), (side * 0.5, 0.6, 0))
        stand(pose, side, name)
        pose.aim[f"hand.{name}"] = Vector((side * 0.1, -0.2, -1))
    return pose


def stand(pose, side, name):
    pose.reach[f"thigh.{name}"] = ((side * 0.12, 0.0, 0.075), (0, -1, 0))
    pose.aim[f"foot.{name}"] = Vector((side * 0.15, -1, -0.3))


def seated(pose, lean):
    """The hips on the seat (the origin), the body leaning forward by `lean` radians."""
    pose.move["pelvis"] = Vector((0, 0.03, 0.08))
    pose.turn["pelvis"] = turn(x=lean)


def sit(t):
    pose = Pose()
    seated(pose, 0.05)
    pose.turn["chest"] = turn(x=0.03 * math.sin(t))
    pose.turn["head"] = turn(z=0.3 * math.sin(t), x=-0.05)
    for side, name in sides():
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
    for side, name in sides():
        angle = t + (0 if side > 0 else math.pi)
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
    pose.move["pelvis"] = Vector((0, 0.02 * math.sin(t), -0.84))
    pose.turn["pelvis"] = turn(x=0.08 * math.sin(t))
    pose.turn["head"] = turn(x=0.25, z=0.2 * math.sin(t))
    for side, name in sides():
        phase = 0 if side > 0 else math.pi
        pose.reach[f"upperarm.{name}"] = ((side * 0.08, 0, -0.085), (side * 1, 0.3, 0))
        pose.aim[f"hand.{name}"] = Vector((-side * 0.2, 0, 1))
        kick = 0.06 * math.sin(2 * t + phase)
        pose.reach[f"thigh.{name}"] = ((side * 0.14, -0.06 + kick, -1.13), (0, -1, 0))
        pose.aim[f"foot.{name}"] = Vector((side * 0.2, -1, -1))
    return pose


def walk(t):
    """Two steps: each foot swings forward (-Y) lifted, then pushes back on the ground; the arms swing against it."""
    pose = Pose()
    stride, lift = 0.11, 0.07
    pose.move["pelvis"] = head_of("pelvis") + Vector((0.012 * math.sin(t), 0, 0.012 * math.cos(2 * t) - 0.012))
    pose.turn["pelvis"] = turn(x=0.08, z=0.12 * math.cos(t))
    pose.turn["chest"] = turn(x=0.06, z=-0.18 * math.cos(t))
    pose.turn["head"] = turn(x=-0.06 + 0.03 * math.cos(2 * t))
    for side, name in sides():
        phase = t if side > 0 else t + math.pi
        forward = -stride * math.cos(phase)
        up = lift * max(0.0, -math.sin(phase))
        pose.reach[f"thigh.{name}"] = ((side * 0.12, forward, 0.075 + up), (0, -1, 0))
        pose.aim[f"foot.{name}"] = Vector((0, -1, -0.3 - 2.5 * up))
        swing = 0.14 * math.cos(phase)   # the arm goes forward with the other leg
        pose.reach[f"upperarm.{name}"] = ((side * 0.36, swing - 0.02, 0.45 + 0.04 * abs(math.cos(phase))),
                                          (side * 0.4, 0.8, 0))
        pose.aim[f"hand.{name}"] = Vector((side * 0.1, swing - 0.2, -1))
    return pose


def wave(t):
    """Both arms over the head, waving to and fro against each other; a little bounce."""
    pose = Pose()
    bounce = 0.015 * abs(math.sin(2 * t))
    pose.move["pelvis"] = head_of("pelvis") + Vector((0, 0, bounce - 0.015))
    pose.turn["chest"] = turn(x=-0.08)
    pose.turn["head"] = turn(x=-0.15, z=0.15 * math.sin(t))
    for side, name in sides():
        stand(pose, side, name)
        to_and_fro = math.sin(2 * t + (0 if side > 0 else math.pi))
        hand = Vector((side * (0.3 + 0.1 * to_and_fro), -0.08, 1.12 + 0.04 * math.cos(2 * t)))
        pose.reach[f"upperarm.{name}"] = (hand, (side * 1, 0.2, -0.3))
        pose.aim[f"hand.{name}"] = Vector((side * 0.3 * to_and_fro, -0.3, 1))
    return pose


def in_water(pose, lean, bob):
    """The figure in the water: the pelvis UNDER_WATER below the origin (+ `bob`), leaning forward by `lean`."""
    pose.move["pelvis"] = Vector((0, 0, -UNDER_WATER + bob))
    pose.turn["pelvis"] = turn(x=lean)


def tread(t):
    """Upright in the water, the hands paddling in circles at the surface, the legs kicking slowly."""
    pose = Pose()
    in_water(pose, 0.1, 0.02 * math.sin(2 * t))
    pose.turn["head"] = turn(z=0.3 * math.sin(t))
    for side, name in sides():
        phase = 2 * t + (0 if side > 0 else math.pi)
        hand = Vector((side * (0.25 + 0.05 * math.cos(phase)), -0.24 + 0.08 * math.sin(phase), -0.06))
        pose.reach[f"upperarm.{name}"] = (hand, (side * 1, 0.5, 0))
        pose.aim[f"hand.{name}"] = Vector((side * 0.3, -1, -0.2))
        pose.reach[f"thigh.{name}"] = ((side * 0.13, 0.08 * math.sin(phase), -0.72), (0, -1, 0))
        pose.aim[f"foot.{name}"] = Vector((0, -0.3, -1))
    return pose


def swim(t):
    """Lying on the water, the head up: the arms pull in turn (forward over the water, back under it), the legs kick."""
    pose = Pose()
    pose.move["pelvis"] = Vector((0, 0.32, -0.1 + 0.01 * math.sin(2 * t)))
    pose.turn["pelvis"] = turn(x=1.35, y=0.08 * math.sin(t))
    pose.turn["head"] = turn(x=-1.1, z=0.2 * math.sin(t))   # up again from the lying body
    for side, name in sides():
        phase = t + (0 if side > 0 else math.pi)
        hand = Vector((side * 0.24, -0.06 - 0.3 * math.cos(phase), -0.08 - 0.18 * math.sin(phase)))
        pose.reach[f"upperarm.{name}"] = (hand, (side * 1, 0, 1))
        pose.aim[f"hand.{name}"] = Vector((0, -1, -0.5))
        kick = 0.08 * math.sin(2 * phase)
        pose.reach[f"thigh.{name}"] = ((side * 0.13, 0.62, -0.12 + kick), (0, 0, -1))
        pose.aim[f"foot.{name}"] = Vector((0, 1, -0.3))
    return pose


def fall(t):
    """Upright, the arms flailing over the head, the legs kicking, looking up."""
    pose = Pose()
    in_water(pose, -0.1, 0)
    pose.turn["head"] = turn(x=-0.35, z=0.15 * math.sin(2 * t))
    for side, name in sides():
        phase = 3 * t + (0 if side > 0 else math.pi)
        hand = Vector((side * (0.34 + 0.07 * math.sin(phase)), -0.06, 0.28 + 0.07 * math.cos(phase)))
        pose.reach[f"upperarm.{name}"] = (hand, (side * 1, 0.3, -0.2))
        pose.aim[f"hand.{name}"] = Vector((side * 0.4, 0, 1))
        pose.reach[f"thigh.{name}"] = ((side * 0.16, 0.1 * math.sin(phase), -0.7), (0, -1, 0))
        pose.aim[f"foot.{name}"] = Vector((side * 0.2, -0.5, -1))
    return pose


ACTIONS = {"idle": (idle, 48), "sit": (sit, 48), "pedal": (pedal, 24), "hang": (hang, 48), "walk": (walk, 24),
           "wave": (wave, 24), "tread": (tread, 48), "swim": (swim, 36), "fall": (fall, 24)}


def make(armature):
    """Every action of ACTIONS on the armature."""
    ugh_rig.make(armature, rig.BONES, ACTIONS)
