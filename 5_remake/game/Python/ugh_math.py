"""Vectors, quaternions and a skeleton posed from its rest pose (metahuman_actions.py), in plain Python.

Vectors are (x, y, z) tuples, quaternions (x, y, z, w) as Unreal's FQuat: qmul(a, b) turns by b, then by a; a bone's
component space rotation is its parent's times its local one.
"""
import math


def add(a, b):
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def scale(a, s):
    return (a[0] * s, a[1] * s, a[2] * s)


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def length(a):
    return math.sqrt(dot(a, a))


def unit(a):
    size = length(a)
    return scale(a, 1 / size) if size > 1e-9 else (0.0, 0.0, 1.0)


def across(a, d):
    """`a` without its part along the unit vector `d`, as a unit vector."""
    return unit(sub(a, scale(d, dot(a, d))))


def qmul(a, b):
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return (aw * bx + ax * bw + ay * bz - az * by, aw * by - ax * bz + ay * bw + az * bx,
            aw * bz + ax * by - ay * bx + az * bw, aw * bw - ax * bx - ay * by - az * bz)


def qconj(q):
    return (-q[0], -q[1], -q[2], q[3])


def qrot(q, v):
    t = scale(cross(q[:3], v), 2)
    return add(add(v, scale(t, q[3])), cross(q[:3], t))


def axis_angle(axis, angle):
    s = math.sin(angle / 2)
    a = unit(axis)
    return (a[0] * s, a[1] * s, a[2] * s, math.cos(angle / 2))


def pitch(angle):
    """Leaning forward (+Z towards +Y) by `angle`."""
    return axis_angle((1, 0, 0), -angle)


def yaw(angle):
    """Turning to the figure's left (+Y towards +X) by `angle`."""
    return axis_angle((0, 0, 1), -angle)


def roll(angle):
    """Tilting to the figure's left (+Z towards +X) by `angle`."""
    return axis_angle((0, 1, 0), angle)


def from_basis(x, y, z):
    """The rotation that turns the axes X, Y, Z into the orthonormal vectors x, y, z."""
    m00, m10, m20 = x
    m01, m11, m21 = y
    m02, m12, m22 = z
    trace = m00 + m11 + m22
    if trace > 0:
        s = 0.5 / math.sqrt(trace + 1)
        return ((m21 - m12) * s, (m02 - m20) * s, (m10 - m01) * s, 0.25 / s)
    if m00 > m11 and m00 > m22:
        s = 2 * math.sqrt(1 + m00 - m11 - m22)
        return (0.25 * s, (m01 + m10) / s, (m02 + m20) / s, (m21 - m12) / s)
    if m11 > m22:
        s = 2 * math.sqrt(1 + m11 - m00 - m22)
        return ((m01 + m10) / s, 0.25 * s, (m12 + m21) / s, (m02 - m20) / s)
    s = 2 * math.sqrt(1 + m22 - m00 - m11)
    return ((m02 + m20) / s, (m12 + m21) / s, 0.25 * s, (m10 - m01) / s)


def frame(direction, toward):
    """The rotation of the frame (`direction`, `toward` across it, their cross product)."""
    d = unit(direction)
    p = across(toward, d)
    return from_basis(d, p, cross(d, p))


def turning(from_dir, from_toward, to_dir, to_toward):
    """The rotation that turns the frame of (from_dir, from_toward) into that of (to_dir, to_toward)."""
    return qmul(frame(to_dir, to_toward), qconj(frame(from_dir, from_toward)))


class Pose:
    """
    A skeleton in its rest pose (local rotations and translations, component space positions of every bone, parents
    before children) and how to pose it: solve(posing, base) -> the local rotations and translations of every bone
    and which of them differ from the rest pose.

    A posing (metahuman_poses.Posing) says where the pelvis is (component space), the turns of bones (in component
    space, about their joints, after their parents'), limbs reaching a point (two bones: the end joint at the target,
    the middle joint bending towards a pole), how hands and feet point and fingers curl. A base (rotations by bone and
    the pelvis's position, a frame of a sampled animation) replaces the rest pose's rotations.
    """

    LIMBS = {"thigh_l": ("calf_l", "foot_l"), "thigh_r": ("calf_r", "foot_r"),
             "upperarm_l": ("lowerarm_l", "hand_l"), "upperarm_r": ("lowerarm_r", "hand_r")}
    FINGERS = ("index", "middle", "ring", "pinky")

    def __init__(self, names, parents, local_rot, local_pos, world_pos):
        self.names, self.parents = names, parents
        self.local_rot, self.local_pos, self.world_pos = local_rot, local_pos, world_pos
        self.world_rot = {}
        for name in names:
            parent = parents.get(name)
            assert parent in self.world_rot or parent not in names, f"{name} before its parent {parent}"
            self.world_rot[name] = qmul(self.world_rot[parent], local_rot[name]) if parent in names else local_rot[name]
        for upper, (middle, end) in self.LIMBS.items():
            assert parents[middle] == upper and parents[end] == middle, f"the limb {upper} is not a chain"
        self.finger_bones = {f"{finger}_{i:02d}_{side}": side for finger in self.FINGERS for i in (1, 2, 3)
                             for side in "lr"}

    def bone_length(self, a, b):
        return length(sub(self.world_pos[b], self.world_pos[a]))

    def limb_ends(self, upper):
        middle, end = self.LIMBS[upper]
        return self.world_pos[upper], self.world_pos[middle], self.world_pos[end]

    def hand_axes(self, side):
        """The hand's forward (to the middle finger) and its thumb's side in the rest pose."""
        hand = self.world_pos[f"hand_{side}"]
        forward = unit(sub(self.world_pos[f"middle_01_{side}"], hand))
        return forward, across(sub(self.world_pos[f"thumb_01_{side}"], hand), forward)

    def foot_axes(self, side):
        forward = unit(sub(self.world_pos[f"ball_{side}"], self.world_pos[f"foot_{side}"]))
        return forward, across((0, 0, 1), forward)

    def curl_axis(self, side):
        """The axis about which the fingers of a hand curl towards its palm (rest pose)."""
        forward, _ = self.hand_axes(side)
        lateral = across(sub(self.world_pos[f"index_01_{side}"], self.world_pos[f"pinky_01_{side}"]), forward)
        tip = sub(self.world_pos[f"middle_03_{side}"], self.world_pos[f"middle_01_{side}"])
        palm = across(tip, forward)   # the rest pose's fingers bend a little to the palm already
        axis = cross(forward, palm)
        return axis if dot(qrot(axis_angle(axis, 0.3), forward), palm) > 0 else scale(axis, -1)

    def reach(self, upper, target, pole, rot, pos):
        """The component space rotations of a limb's upper and middle bones that put its end at `target`."""
        middle, end = self.LIMBS[upper]
        a, b = self.bone_length(upper, middle), self.bone_length(middle, end)
        start = pos[upper]
        to_target = sub(target, start)
        d = min(max(length(to_target), abs(a - b) + 1e-3), a + b - 1e-3)
        u = unit(to_target)
        v = across(pole, u)
        cos_a = (a * a + d * d - b * b) / (2 * a * d)
        knee = add(start, add(scale(u, a * cos_a), scale(v, a * math.sqrt(max(0.0, 1 - cos_a * cos_a)))))
        end_at = add(start, scale(u, d))
        rest_start, rest_middle, rest_end = self.limb_ends(upper)
        rest_pole = across(pole, unit(sub(rest_end, rest_start)))
        upper_turn = turning(sub(rest_middle, rest_start), rest_pole, sub(knee, start), v)
        middle_turn = turning(sub(rest_end, rest_middle), rest_pole, sub(end_at, knee), v)
        return qmul(upper_turn, self.world_rot[upper]), qmul(middle_turn, self.world_rot[middle])

    def solve(self, posing, base=None):
        rot, pos, solved = {}, {}, {}
        base_rot, base_pelvis = base if base else (None, None)
        for name in self.names:
            parent = self.parents.get(name)
            local = base_rot[name] if base_rot else self.local_rot[name]
            if parent in rot:
                r = qmul(rot[parent], local)
                p = add(pos[parent], qrot(rot[parent], self.local_pos[name]))
            else:
                r, p = local, self.local_pos[name]
            if name == "pelvis":
                p = posing.pelvis or base_pelvis or self.world_pos["pelvis"]
            if name in solved:
                r = solved.pop(name)
            if name in posing.turn:
                r = qmul(posing.turn[name], r)
            if name in posing.aim:
                forward, toward = posing.aim[name]
                side = name[-1]
                rest_forward, rest_toward = self.hand_axes(side) if name.startswith("hand") else self.foot_axes(side)
                r = qmul(turning(rest_forward, rest_toward, forward, toward), self.world_rot[name])
            side = self.finger_bones.get(name)
            if side and posing.curl.get(side):
                hand_turn = qmul(rot[f"hand_{side}"], qconj(self.world_rot[f"hand_{side}"]))
                r = qmul(axis_angle(qrot(hand_turn, self.curl_axis(side)), posing.curl[side]), r)
            rot[name], pos[name] = r, p
            if name in posing.reach:
                target, pole = posing.reach[name]
                upper, middle = self.reach(name, target, pole, rot, pos)
                rot[name] = upper
                solved[self.LIMBS[name][0]] = middle
        local_rot, local_pos, changed = {}, {}, {}
        for name in self.names:
            parent = self.parents.get(name)
            if parent in rot:
                inverse = qconj(rot[parent])
                local_rot[name] = qmul(inverse, rot[name])
                local_pos[name] = qrot(inverse, sub(pos[name], pos[parent]))
            else:
                local_rot[name], local_pos[name] = rot[name], pos[name]
            rest = self.local_rot[name]
            if abs(sum(x * y for x, y in zip(local_rot[name], rest))) < 0.99999 or \
                    length(sub(local_pos[name], self.local_pos[name])) > 0.01:
                changed[name] = True
        return local_rot, local_pos, changed
