"""How a MetaHuman does the actions the game has no clips for (metahuman_actions.py): a posing per moment t (0 .. 2 pi,
one loop) in the body's own proportions (its rest pose), component space centimetres, the figure facing +Y, its left
+X; where each action's origin is: see metahuman_actions.py.
"""
import math
import os
import sys

from ugh_math import add, pitch, qmul, roll, scale, sub, yaw

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "Blender"))

import copter_layout as layout  # noqa: E402


def copter_cm(at):
    """A point of copter_layout.py (metres, front -Y) in the copter's centimetres and Unreal's axes (front +Y)."""
    return (at[0] * 100, -at[1] * 100, at[2] * 100)


# the people's height (cm, UghFigurePlace::PersonHeight): the game shows a MetaHuman in the copter as high
PERSON = layout.PERSON_HEIGHT * 100
# from the seat the crank's axle, its pedals' radius and how far each is from the middle of the axle, the left handle
PEDAL_AXLE = copter_cm(layout.PEDAL_AXLE)
PEDAL_RADIUS, PEDAL_SPREAD = layout.PEDAL_RADIUS * 100, layout.PEDAL_SPREAD * 100
GRIP = copter_cm(layout.GRIP)
SIDES = ((1, "l"), (-1, "r"))


class Posing:
    """Where the pelvis is, turns of bones, limbs reaching a point (target, pole), hands and feet pointing (forward,
    the thumb's or the foot's top side), fingers curling (radians) by side; see ugh_math.Pose."""

    def __init__(self):
        self.pelvis = None
        self.turn, self.reach, self.aim, self.curl = {}, {}, {}, {}

    def spine(self, lean, twist=0.0, side=0.0):
        """The back bent forward by `lean`, turned by `twist`, tilted by `side` (radians), spread over its bones."""
        bones = ("spine_01", "spine_02", "spine_03", "spine_04", "spine_05")
        for bone in bones:
            n = len(bones)
            self.turn[bone] = qmul(yaw(twist / n), qmul(pitch(lean / n), roll(side / n)))

    def head(self, look_down=0.0, look_left=0.0):
        self.turn["neck_01"] = qmul(yaw(look_left / 2), pitch(look_down / 2))
        self.turn["head"] = qmul(yaw(look_left / 2), pitch(look_down / 2))


class Body:
    """The measures of a body's rest pose the posings use."""

    def __init__(self, rest, height):
        self.rest, self.at = rest, rest.world_pos
        self.height = height
        self.copter = height / PERSON   # copter centimetres to the body's
        self.leg = rest.bone_length("thigh_l", "calf_l") + rest.bone_length("calf_l", "foot_l")
        self.arm = rest.bone_length("upperarm_l", "lowerarm_l") + rest.bone_length("lowerarm_l", "hand_l")
        self.hip_drop = self.at["pelvis"][2] - self.at["thigh_l"][2]   # the hip joints below the pelvis
        self.head_rise = self.at["head"][2] - self.at["pelvis"][2]
        self.shoulder_rise = self.at["upperarm_l"][2] - self.at["pelvis"][2]

    def hip(self, side):
        return self.at[f"thigh_{side}"]

    def shoulder(self, side):
        return self.at[f"upperarm_{side}"]


def seated(posing, body, lean):
    """The hips on the seat (the origin; the hip joints a hand's breadth above it), leaning forward."""
    posing.pelvis = (0, -0.03 * body.height, 0.058 * body.height + body.hip_drop)
    posing.turn["pelvis"] = pitch(lean)


def standing(posing, body, bounce=0.0):
    pelvis = body.at["pelvis"]
    posing.pelvis = (pelvis[0], pelvis[1], pelvis[2] + bounce)


def in_water(posing, body, lean, bob):
    """Upright in the water: the head's joint (at the chin) just above the surface (the origin)."""
    posing.pelvis = (0, 0, 0.03 * body.height - body.head_rise + bob)
    posing.turn["pelvis"] = pitch(lean)


def wave(body):
    """Both arms over the head, waving to and fro against each other; a little bounce, looking up."""
    def posing(t):
        p = Posing()
        standing(p, body, -0.008 * body.height * abs(math.sin(2 * t)))
        p.spine(-0.08)
        p.head(-0.25, 0.15 * math.sin(t))
        for side, s in SIDES:
            to_and_fro = math.sin(2 * t + (0 if side > 0 else math.pi))
            shoulder = body.shoulder(s)
            hand = add(shoulder, (side * (0.08 + 0.05 * to_and_fro) * body.height, 0.02 * body.height,
                                  0.86 * body.arm))
            p.reach[f"upperarm_{s}"] = (hand, (side, -0.4, 0))
            p.aim[f"hand_{s}"] = ((side * 0.35 * to_and_fro, 0.1, 1), (-side, 0, 0))
            p.curl[s] = 0.15
        return p
    return posing


def sit(body):
    """Sitting, the hands on the knees, the feet dangling, looking about."""
    def posing(t):
        p = Posing()
        seated(p, body, 0.08)
        p.spine(0.05 * math.sin(t) + 0.05)
        p.head(0.05, 0.35 * math.sin(t))
        thigh = body.rest.bone_length("thigh_l", "calf_l")
        shin = body.rest.bone_length("calf_l", "foot_l")
        for side, s in SIDES:
            hip = (side * abs(body.hip(s)[0]) * 1.1, p.pelvis[1], p.pelvis[2] - body.hip_drop)
            knee = add(hip, (0, 0.95 * thigh, -0.2 * thigh))
            swing = 0.12 * math.sin(t + (0 if side > 0 else math.pi))
            p.reach[f"thigh_{s}"] = (add(knee, (0, (0.15 + swing) * shin, -0.97 * shin)), (0, 1, 0.5))
            p.aim[f"foot_{s}"] = ((0, 1, -0.5), (0, 0.5, 1))
            p.reach[f"upperarm_{s}"] = (add(knee, (side * 0.02 * body.height, -0.07 * thigh, 0.07 * thigh)),
                                        (side, -0.6, 0))
            p.aim[f"hand_{s}"] = ((side * 0.1, 1, -0.6), (-side, 0.3, 0.3))
            p.curl[s] = 0.35
        return p
    return posing


def pedal(body):
    """Sitting, pedalling the crank (one turn a loop, the left pedal on top at the start, going forward), holding the
    handles."""
    k = body.copter

    def posing(t):
        p = Posing()
        seated(p, body, 0.18)
        p.spine(0.12, 0.05 * math.sin(t))
        p.head(-0.15 + 0.04 * math.sin(2 * t))
        for side, s in SIDES:
            angle = t + (0 if side > 0 else math.pi)
            pedal_at = add(scale(PEDAL_AXLE, k), (side * PEDAL_SPREAD * k, PEDAL_RADIUS * k * math.sin(angle),
                                                   PEDAL_RADIUS * k * math.cos(angle)))
            p.reach[f"thigh_{s}"] = (add(pedal_at, (0, -2.5 * k, 4 * k)), (side * 0.15, 1, 0.6))
            p.aim[f"foot_{s}"] = ((0, 1, -0.2 - 0.2 * math.sin(angle)), (0, 0.2, 1))
            grip = (side * GRIP[0] * k, GRIP[1] * k, GRIP[2] * k)
            p.reach[f"upperarm_{s}"] = (grip, (side * 0.8, -0.4, -0.5))
            p.aim[f"hand_{s}"] = ((side * 0.15, 1, -0.1), (0, 0, 1))
            p.curl[s] = 1.1
        return p
    return posing


def hang(body):
    """Hanging from a rope by both hands (the origin where they hold it), the legs swinging."""
    def posing(t):
        p = Posing()
        drop = 0.97 * body.arm + body.shoulder_rise + 0.04 * body.height
        p.pelvis = (0, 0.02 * body.height * math.sin(t), -drop)
        p.turn["pelvis"] = pitch(0.06 * math.sin(t))
        p.head(-0.25, 0.25 * math.sin(t))
        for side, s in SIDES:
            p.reach[f"upperarm_{s}"] = ((side * 0.02 * body.height, 0, -0.045 * body.height), (side, -0.3, 0))
            p.aim[f"hand_{s}"] = ((-side * 0.15, 0, 1), (0, 1, 0))
            p.curl[s] = 1.2
            kick = 0.06 * math.sin(2 * t + (0 if side > 0 else math.pi))
            hip = add(p.pelvis, (side * abs(body.hip(s)[0]), 0, -body.hip_drop))
            p.reach[f"thigh_{s}"] = (add(hip, (0, (kick + 0.08) * body.leg, -0.95 * body.leg)), (0, 1, 0))
            p.aim[f"foot_{s}"] = ((side * 0.2, 0.6, -1), (0, 1, 0.3))
        return p
    return posing


def legs_kicking(p, body, t, depth):
    """The legs under the water, kicking slowly in turn."""
    for side, s in SIDES:
        phase = 2 * t + (0 if side > 0 else math.pi)
        hip = add(p.pelvis, (side * abs(body.hip(s)[0]), 0, -body.hip_drop))
        p.reach[f"thigh_{s}"] = (add(hip, (side * 0.02 * body.height, 0.12 * body.leg * math.sin(phase),
                                           -depth * body.leg)), (0, 1, 0))
        p.aim[f"foot_{s}"] = ((0, 0.3, -1), (0, 1, 0.2))


def tread(body):
    """Upright in the water, the hands paddling in circles at the surface, the legs kicking slowly."""
    def posing(t):
        p = Posing()
        in_water(p, body, 0.1, 0.01 * body.height * math.sin(2 * t))
        p.head(0.1, 0.3 * math.sin(t))
        for side, s in SIDES:
            phase = 2 * t + (0 if side > 0 else math.pi)
            hand = (side * (0.2 + 0.03 * math.cos(phase)) * body.height,
                    (0.14 + 0.05 * math.sin(phase)) * body.height, -0.06 * body.height)
            p.reach[f"upperarm_{s}"] = (hand, (side, -0.5, 0))
            p.aim[f"hand_{s}"] = ((side * 0.3, 1, -0.2), (0, 0, 1))
            p.curl[s] = 0.2
        legs_kicking(p, body, t, 0.9)
        return p
    return posing


def swim(body):
    """Lying on the water, the head up: the arms pull in turn (forward over the water, back under it), the legs
    kick."""
    def posing(t):
        p = Posing()
        p.pelvis = (0, -0.85 * body.head_rise, -0.08 * body.height + 0.006 * body.height * math.sin(2 * t))
        p.turn["pelvis"] = qmul(pitch(1.35), roll(0.08 * math.sin(t)))
        p.head(-0.6)
        p.turn["neck_01"] = pitch(-0.5)
        reach = 0.25 * body.height
        for side, s in SIDES:
            phase = t + (0 if side > 0 else math.pi)
            hand = (side * 0.16 * body.height, -0.3 * body.head_rise + reach * math.cos(phase),
                    -0.05 * body.height - 0.1 * body.height * max(0.0, math.sin(phase)))
            p.reach[f"upperarm_{s}"] = (hand, (side, 0, 1))
            p.aim[f"hand_{s}"] = ((0, 1, -0.5), (-side, 0, 0))
            kick = 0.08 * body.leg * math.sin(2 * phase)
            hip = add(p.pelvis, (side * abs(body.hip(s)[0]), 0, 0))
            p.reach[f"thigh_{s}"] = (add(hip, (0, -0.95 * body.leg, -0.1 * body.leg + kick)), (0, 0, -1))
            p.aim[f"foot_{s}"] = ((0, -1, -0.3), (0, 0, -1))
        return p
    return posing


def fall(body):
    """Upright, going down: the arms flailing over the head, the legs kicking, looking up."""
    def posing(t):
        p = Posing()
        in_water(p, body, -0.1, 0)
        p.head(-0.4, 0.15 * math.sin(2 * t))
        for side, s in SIDES:
            phase = 3 * t + (0 if side > 0 else math.pi)
            shoulder = sub(body.shoulder(s), (0, 0, body.at["pelvis"][2] - p.pelvis[2]))
            hand = add(shoulder, (side * (0.12 + 0.04 * math.sin(phase)) * body.height, 0.02 * body.height,
                                  (0.75 + 0.1 * math.cos(phase)) * body.arm))
            p.reach[f"upperarm_{s}"] = (hand, (side, -0.3, -0.2))
            p.aim[f"hand_{s}"] = ((side * 0.4, 0, 1), (-side, 0, 0))
            p.curl[s] = 0.2
        legs_kicking(p, body, 1.5 * t, 0.85)
        return p
    return posing


def flail(body):
    """Flung off a pad through the air into the sea: the arms windmilling wide in turn, the legs running on air, leaning
    back, looking down at the water (the origin as fall's: where the water's surface will be at the chin)."""
    def posing(t):
        p = Posing()
        in_water(p, body, -0.22, 0.01 * body.height * math.sin(2 * t))
        p.spine(-0.12, 0.1 * math.sin(2 * t), 0.06 * math.sin(2 * t))
        p.head(0.3, 0.2 * math.sin(2 * t))
        for side, s in SIDES:
            phase = 2 * t + (0 if side > 0 else math.pi)
            shoulder = sub(body.shoulder(s), (0, 0, body.at["pelvis"][2] - p.pelvis[2]))
            # a big circle beside the body in its front plane, a little in front of it (the hand never crosses it)
            middle = add(shoulder, (side * 0.45 * body.arm, 0.1 * body.arm, 0.3 * body.arm))
            radius = 0.48 * body.arm
            hand = add(middle, (side * radius * math.cos(phase), 0.1 * body.arm * math.sin(phase),
                                radius * math.sin(phase)))
            p.reach[f"upperarm_{s}"] = (hand, (side, -0.4, -0.3))
            p.aim[f"hand_{s}"] = ((side * math.cos(phase), 0.3, math.sin(phase)), (0, 1, 0))
            p.curl[s] = 0.5 + 0.3 * math.sin(phase)
            hip = add(p.pelvis, (side * abs(body.hip(s)[0]), 0, -body.hip_drop))
            stride = 0.28 * body.leg * math.sin(phase + 0.5 * math.pi)
            lift = 0.18 * body.leg * max(0.0, math.cos(phase + 0.5 * math.pi))
            p.reach[f"thigh_{s}"] = (add(hip, (side * 0.03 * body.height, stride, -0.85 * body.leg + lift)), (0, 1, 0))
            p.aim[f"foot_{s}"] = ((0, 0.4, -1), (0, 1, 0.3))
        return p
    return posing


def actions(rest, height):
    """The actions by name: (seconds a loop, the posing of a moment; none: sampled from a clip on the rest pose)."""
    body = Body(rest, height)
    return {"idle": (0, None), "sit": (3.2, sit(body)), "pedal": (1.0, pedal(body)), "hang": (3.2, hang(body)),
            "walk": (0, None), "wave": (1.2, wave(body)), "tread": (2.4, tread(body)), "swim": (2.4, swim(body)),
            "fall": (1.2, fall(body)), "flail": (1.4, flail(body))}

