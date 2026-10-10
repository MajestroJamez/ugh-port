"""The actions of the game's MetaHumans (metahumans.py, EUghCaveAction): for each character an animation per action
on its body (/Game/External/MetaHumans/<name>/Actions/AS_<action>), each a loop, made for that body's proportions:

- idle and walk: MetaHuman Creator's standing idle (looking about over it) and walk loop (its optional content), of
  the walk one stride in place; cheer that stride with both arms up (walking off delivered),
- the others posed here (no clips for them): wave (calling a copter with both arms over the head), sit (in the
  cabin, hands on the knees), pedal (the copter's crank, holding its handles), hang (from a rope by both hands),
  tread, swim and fall (in the water), flail (flung through the air into the sea, windmilling the arms), duck
  (crouched, the forearms over the head: a copter flying low by him).

Where an action has its origin is as the caveman's (Blender/caveman_actions.py): on the ground between the feet
(idle, walk, wave, duck, cheer), the seat (sit, pedal), where the hands hold the rope (hang), the water's surface with the chin
just above it (tread, swim, fall, flail). The copter's crank and handles are copter_layout.py's in the copter's scale: the
game shows a MetaHuman as high as a walking passenger's sprite (metahuman_poses.PERSON). The figures face +Y, their
left is +X.

    UnrealEditor-Cmd UghGame.uproject -run=pythonscript -script=Python/metahuman_actions.py
"""
import math
import os
import sys
import traceback

import unreal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import metahuman_poses as poses  # noqa: E402
from ugh_math import Pose, scale, sub  # noqa: E402

ROOT = "/Game/External/MetaHumans"
CHARACTERS = ("Pilot", "Man", "Woman", "Grandpa")
# radians: the old man stooped (metahuman_poses.stooped)
STOOP = {"Grandpa": 0.45}
LOCOMOTION = "/MetaHumanCharacter/Optional/Animation/UEFNAnimPreset/Locomotion"
# the clips sampled: the action, the clip, whether only its first stride
SAMPLED = {"idle": (f"{LOCOMOTION}/AS_MH_Neutral_Stand_Idle_Loop", False),
           "walk": (f"{LOCOMOTION}/AS_MH_Neutral_Walk_Loop_F", True),
           "cheer": (f"{LOCOMOTION}/AS_MH_Neutral_Walk_Loop_F", True)}
FPS = 30
REPORT = os.path.join(unreal.Paths.project_saved_dir(), "Logs", "UghMetaHumanActions.txt")

Lib = unreal.AnimPoseExtensions
Spaces = unreal.AnimPoseSpaces


def vec(v):
    return (v.x, v.y, v.z)


def quat(q):
    return (q.x, q.y, q.z, q.w)


def read_pose(pose, names):
    """The local rotations and translations and the component space positions of `names` in `pose`."""
    local_rot, local_pos, world_pos = {}, {}, {}
    for name in names:
        local = Lib.get_bone_pose(pose, name, Spaces.LOCAL)
        local_rot[name], local_pos[name] = quat(local.rotation), vec(local.translation)
        world_pos[name] = vec(Lib.get_bone_pose(pose, name, Spaces.WORLD).translation)
    return local_rot, local_pos, world_pos


def new_sequence(folder, name, skeleton, mesh, frames):
    path = f"{folder}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorAssetLibrary.delete_asset(path)
    factory = unreal.AnimSequenceFactory()
    factory.set_editor_property("target_skeleton", skeleton)
    factory.set_editor_property("preview_skeletal_mesh", mesh)
    sequence = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.AnimSequence, factory)
    # made for this body: the engine retargets nothing (from the skeleton's proportions it would lift the pelvis)
    sequence.set_editor_property("retarget_source_asset", mesh)
    controller = sequence.controller
    controller.open_bracket("UGH action", False)
    controller.set_frame_rate(unreal.FrameRate(FPS, 1), False)
    controller.set_number_of_frames(unreal.FrameNumber(frames), False)
    controller.close_bracket(False)
    return sequence


def write_keys(sequence, names, frames_of_poses):
    """Keys every bone whose pose differs from the rest pose in a frame (always the pelvis)."""
    controller = sequence.controller
    controller.open_bracket("UGH keys", False)
    controller.remove_all_bone_tracks(False)
    for name in names:
        rotations = []
        for frame in frames_of_poses:   # each key the nearer of q and -q to the one before: no flips between them
            q = frame[0][name]
            if rotations and sum(a * b for a, b in zip(q, rotations[-1])) < 0:
                q = tuple(-x for x in q)
            rotations.append(q)
        positions = [frame[1][name] for frame in frames_of_poses]
        if name != "pelvis" and all(frame[2].get(name) is None for frame in frames_of_poses):
            continue
        controller.add_bone_curve(name, False)
        controller.set_bone_track_keys(name, [unreal.Vector(*p) for p in positions],
                                       [unreal.Quat(*q) for q in rotations], [unreal.Vector(1, 1, 1)] * len(positions),
                                       False)
    controller.close_bracket(False)


def sampled(source, rest, stride):
    """
    The frames of the clip `source` at FPS: the local rotations and the pelvis in place (its drift across the loop
    taken out); with `stride` only its first stride (from the left foot's farthest forward to the next time).
    """
    options = unreal.AnimPoseEvaluationOptions()
    names = rest.names
    samples = []
    for frame in range(round(source.get_play_length() * FPS) + 1):
        pose = Lib.get_anim_pose_at_time(source, frame / FPS, options)
        rotations = {name: quat(Lib.get_bone_pose(pose, name, Spaces.LOCAL).rotation) for name in names}
        rotations["root"] = rest.local_rot["root"]
        pelvis = vec(Lib.get_bone_pose(pose, "pelvis", Spaces.WORLD).translation)
        forward = Lib.get_bone_pose(pose, "foot_l", Spaces.WORLD).translation.y - pelvis[1]
        samples.append((rotations, pelvis, forward))
    if stride:
        ahead = [i for i in range(1, len(samples) - 1)
                 if samples[i - 1][2] < samples[i][2] >= samples[i + 1][2]]
        if len(ahead) < 2:
            raise RuntimeError(f"no stride in {source.get_name()}")
        samples = samples[ahead[0]:ahead[1] + 1]
    first, last = samples[0][1], samples[-1][1]
    drift = sub(last, first)
    source_rest = Lib.get_reference_pose(source.get_skeleton())
    ratio = rest.world_pos["pelvis"][2] / Lib.get_bone_pose(source_rest, "pelvis", Spaces.WORLD).translation.z
    frames = len(samples) - 1
    result = []
    for frame, (rotations, pelvis, _) in enumerate(samples):
        in_place = sub(pelvis, scale(drift, frame / frames))
        in_place = (in_place[0] - first[0], in_place[1] - first[1], in_place[2])
        result.append((rotations, scale(in_place, ratio)))
    return result


def make(name, report):
    folder = f"{ROOT}/{name}/Actions"
    mesh = unreal.load_asset(f"{ROOT}/{name}/Body/SKM_{name}_BodyMesh")
    face = unreal.load_asset(f"{ROOT}/{name}/Face/SKM_{name}_FaceMesh")
    if mesh is None or face is None:
        raise RuntimeError("not built (metahumans.py)")
    bounds = face.get_imported_bounds()
    height = bounds.origin.z + bounds.box_extent.z
    skeleton = mesh.skeleton
    # the body's rest pose: a sequence without keys evaluated on the body
    probe = new_sequence(folder, "AS_rest", skeleton, mesh, 1)
    options = unreal.AnimPoseEvaluationOptions()
    options.optional_skeletal_mesh = mesh
    options.should_retarget = True
    rest_pose = Lib.get_anim_pose_at_time(probe, 0, options)
    names = [str(n) for n in Lib.get_bone_names(rest_pose)]
    parents = {n: str(mesh.get_bone_parent(n)) for n in names}
    rest = Pose(names, parents, *read_pose(rest_pose, names))
    unreal.EditorAssetLibrary.delete_asset(f"{folder}/AS_rest")
    report(f"{name}: {len(names)} bones, {height:.0f} cm tall")
    for action, (seconds, posing) in poses.actions(rest, height, STOOP.get(name, 0.0)).items():
        base = None
        if action in SAMPLED:
            clip, stride = SAMPLED[action]
            base = sampled(unreal.load_asset(clip), rest, stride)
            frames = len(base) - 1
        else:
            frames = round(seconds * FPS)
        keys = []
        for frame in range(frames + 1):
            t = 2 * math.pi * frame / frames
            keys.append(rest.solve(posing(t) if posing else poses.Posing(), base[frame] if base else None))
        sequence = new_sequence(folder, f"AS_{action}", skeleton, mesh, frames)
        write_keys(sequence, names, keys)
        report(f"  {action}: {frames} frames")
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)


def main():
    unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous([LOCOMOTION, ROOT], True)
    lines = []

    def report(text):
        lines.append(text)
        with open(REPORT, "w", encoding="ascii", errors="replace") as file:
            file.write("\n".join(lines) + "\n")

    made = 0
    for name in CHARACTERS:
        try:
            make(name, report)
            made += 1
        except Exception:
            report(f"{name}: FAILED {traceback.format_exc()}")
    report("OK" if made == len(CHARACTERS) else f"FAILED: {len(CHARACTERS) - made}")


main()
