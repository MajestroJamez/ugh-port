"""The caveman's skeleton and skin weights (caveman.py, ugh_rig.py).

Bones in rest pose (an A pose, metres, facing -Y, the left side +X): root at the ground between the feet, pelvis,
spine, chest, neck, head, and per side clavicle, upper arm, forearm, hand, thigh, shin, foot. A vertex of the body
or the hide follows the bones nearest to it; hair, beard and eyes follow the head.
"""
import ugh_rig

BONES = {
    "root": ((0, 0, 0), (0, 0, 0.1), None),
    "pelvis": ((0, 0, 0.4), (0, 0, 0.52), "root"),
    "spine": ((0, 0, 0.52), (0, 0, 0.67), "pelvis"),
    "chest": ((0, 0, 0.67), (0, 0, 0.81), "spine"),
    "neck": ((0, 0, 0.81), (0, -0.01, 0.88), "chest"),
    "head": ((0, -0.01, 0.88), (0, -0.01, 1.12), "neck"),
    **ugh_rig.mirrored({
        "clavicle.L": ((0.04, 0.0, 0.78), (0.19, 0.01, 0.79), "chest"),
        "upperarm.L": ((0.2, 0.01, 0.77), (0.35, 0.02, 0.61), "clavicle.L"),
        "forearm.L": ((0.35, 0.02, 0.61), (0.45, -0.02, 0.46), "upperarm.L"),
        "hand.L": ((0.45, -0.02, 0.46), (0.49, -0.04, 0.37), "forearm.L"),
        "thigh.L": ((0.11, 0.0, 0.4), (0.12, -0.02, 0.23), "pelvis"),
        "shin.L": ((0.12, -0.02, 0.23), (0.12, 0.0, 0.075), "thigh.L"),
        "foot.L": ((0.12, 0.0, 0.075), (0.12, -0.12, 0.03), "shin.L"),
    }),
}
# what the hide may follow (not the arms: its strap sits on the shoulder)
HIDE_BONES = ("pelvis", "spine", "chest", "clavicle.L", "thigh.L", "thigh.R")
RIGID_SLOTS = ("eye", "pupil", "hair_short", "hair_long", "beard")


def build(figure):
    """The armature of BONES with `figure` (its material slots named as caveman.SLOTS) skinned to it."""
    return ugh_rig.build("caveman", BONES, figure, slot_bones={slot: "head" for slot in RIGID_SLOTS},
                         slot_allowed={"fur": HIDE_BONES})
