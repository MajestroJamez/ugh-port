"""The walker of the remake from Jan's triceratops (a Unity FBX from Sketchfab, kept local: assets/3d/sketchfab/
triceratops; 2312 vertices, 36 bones, actions walk, run, attack1, die, eat, idle) as walker_triceratops.glb: the same
animal with its own animations, refined for a close look.

- The import: the FBX's root bone is scaled by 0.2, which Blender's import keeps on the mesh but not on the bones (the
  skeleton comes out 5 times larger about the root, so the skin tore the mesh apart): the mesh is scaled 5 times about
  the root too. The whole is then turned from its axes (feet towards +X, head towards +Z) to the remake's (up +Z,
  head towards -Y), scaled from 2 cm to LENGTH and set on the ground under its middle (its feet over the walk); the
  animations' moves are scaled with it.
- The mesh: subdivided twice (Catmull-Clark; its sharp edges creased, so the horns, the frill's rim and the claws stay
  sharp), the weights with it.
- The skin: baked from its texture (warmed into an earthy olive brown, its pattern kept, the belly lighter) and a hide
  of scales in the model's space (big rounded scutes among small pebbly scales, darker in their grooves): colour,
  normal and roughness maps.
- The actions (each a loop, named as the walker's): walk (its walk), watch (its idle), charge (its run), recover (its
  attack1: tossing its horns), stunned (slumped onto its belly as early in its die, the head swaying, dazed).

    blender -b --factory-startup --python-exit-code 1 --python walker_triceratops.py -- <folder> <triceratops folder>
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import bpy  # noqa: E402
from mathutils import Matrix, Quaternion, Vector  # noqa: E402

import ugh_bake as bake  # noqa: E402
import ugh_kit as kit  # noqa: E402

FOLDER, SOURCE = kit.arguments()[:2]
FBX = os.path.join(SOURCE, "source", "Triceratops", "triceratops.fbx")
TEXTURE = os.path.join(SOURCE, "textures", "triceratops_col4.png")
LENGTH = 3.1             # m, nose to tail (the walker's sprite is 32 x 22 px: 3.2 x 2.2 m)
TEXTURE_SIZE = 2048
CREASE_ANGLE = math.radians(60)   # between two faces' normals: an edge sharper than it stays sharp
ROOT_SCALE = 0.2         # the FBX's root bone's scale
# its axes (after the import's own turn): the feet towards +X, the head towards +Z -> up +Z, the head towards -Y
TURN = Matrix(((0, 1, 0, 0), (0, 0, -1, 0), (-1, 0, 0, 0), (0, 0, 0, 1)))
# the walker's actions from its own: name -> its action
OWN = {"walk": "walk", "watch": "idle", "charge": "run", "recover": "attack1"}
STUNNED_FRAMES = 48
SLUMP = 0.18             # of its die: down on its belly, before it rolls over
NECK = "joint27"         # the bone the head and the frill hang on
DAZE = math.radians(10)  # how far the stunned one's head sways


def channel_curves(action):
    """The F-curves of `action` (Blender 5 keeps them in the channel bags of its layers)."""
    return [curve for layer in action.layers for strip in layer.strips for bag in strip.channelbags
            for curve in bag.fcurves]


def imported():
    """The armature, the mesh and the armature's actions by their names in the FBX; the rig's empties gone."""
    kit.clear_scene()
    bpy.ops.import_scene.fbx(filepath=FBX, automatic_bone_orientation=False)
    armature = next(each for each in bpy.data.objects if each.type == "ARMATURE")
    figure = next(each for each in bpy.data.objects if each.type == "MESH")
    for each in [each for each in bpy.data.objects if each.type == "EMPTY"]:
        bpy.data.objects.remove(each)
    actions = {}
    for action in list(bpy.data.actions):
        parts = action.name.split("|")
        if len(parts) == 3 and parts[0] == armature.name:
            actions[parts[1]] = action
        else:
            bpy.data.actions.remove(action)
    figure.scale = Vector(figure.scale) / ROOT_SCALE
    armature.animation_data.action = None
    return armature, figure, actions


def apply(each):
    bpy.ops.object.select_all(action="DESELECT")
    each.select_set(True)
    bpy.context.view_layer.objects.active = each
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)


def posed_points(armature, figure, action, frames):
    """The deformed mesh's points over `frames` of `action`."""
    armature.animation_data.action = action
    points = []
    for frame in frames:
        bpy.context.scene.frame_set(frame)
        evaluated = figure.evaluated_get(bpy.context.evaluated_depsgraph_get())
        mesh = evaluated.to_mesh()
        points.extend(evaluated.matrix_world @ vertex.co for vertex in mesh.vertices)
        evaluated.to_mesh_clear()
    armature.animation_data.action = None
    return points


def normalised(armature, figure, actions):
    """Turned, scaled to LENGTH, its middle at the origin, its feet on the ground; transforms applied, the moves of
    the actions scaled (the armature's scale goes into its bones, not into the bones' moves)."""
    walk = actions["walk"]
    frames = range(int(walk.frame_range[0]), int(walk.frame_range[1]) + 1, 4)
    points = [TURN @ point for point in posed_points(armature, figure, walk, frames)]
    low = Vector([min(p[i] for p in points) for i in range(3)])
    high = Vector([max(p[i] for p in points) for i in range(3)])
    scale = LENGTH / (high.y - low.y)
    middle = Vector(((low.x + high.x) / 2, (low.y + high.y) / 2, low.z))
    world = Matrix.Scale(scale, 4) @ Matrix.Translation(-middle) @ TURN
    moves = scale * armature.matrix_world.to_scale().x
    figure.parent = None
    figure.matrix_world = world @ figure.matrix_world
    armature.matrix_world = world @ armature.matrix_world
    apply(figure)
    apply(armature)
    figure.parent = armature
    figure.matrix_parent_inverse = Matrix()
    for action in actions.values():
        for curve in channel_curves(action):
            if curve.data_path.endswith(".location"):
                for point in curve.keyframe_points:
                    for each in (point.co, point.handle_left, point.handle_right):
                        each.y *= moves


def refined(figure):
    """Subdivided twice before the armature deforms it (the weights subdivided too), sharp edges creased."""
    mesh = figure.data
    mesh.update()
    crease = mesh.attributes.get("crease_edge") or mesh.attributes.new("crease_edge", "FLOAT", "EDGE")
    faces_of = {}
    for polygon in mesh.polygons:
        for key in polygon.edge_keys:
            faces_of.setdefault(key, []).append(polygon.normal)
    for edge in mesh.edges:
        normals = faces_of.get(edge.key, [])
        sharp = len(normals) == 2 and normals[0].angle(normals[1], 0) > CREASE_ANGLE
        crease.data[edge.index].value = 0.8 if sharp else 0.0
    subdivision = figure.modifiers.new("refine", "SUBSURF")
    subdivision.levels = subdivision.render_levels = 2
    subdivision.use_creases = True
    bpy.ops.object.select_all(action="DESELECT")
    figure.select_set(True)
    bpy.context.view_layer.objects.active = figure
    bpy.ops.object.modifier_move_to_index(modifier=subdivision.name, index=0)
    bpy.ops.object.modifier_apply(modifier=subdivision.name)
    for polygon in figure.data.polygons:
        polygon.use_smooth = True


def skin_graph(nodes, links):
    """The hide: its texture warmed into olive brown, scales in the model's space (metres)."""
    coordinates = nodes.new("ShaderNodeTexCoord")
    texture = nodes.new("ShaderNodeTexImage")
    texture.image = kit.load_image(TEXTURE)
    links.new(coordinates.outputs["UV"], texture.inputs["Vector"])
    scales = []
    for size in (0.035, 0.12):   # pebbly scales, big scutes
        cells = nodes.new("ShaderNodeTexVoronoi")
        cells.feature = "DISTANCE_TO_EDGE"
        cells.inputs["Scale"].default_value = 1 / size
        cells.inputs["Randomness"].default_value = 0.85
        links.new(coordinates.outputs["Object"], cells.inputs["Vector"])
        tint = nodes.new("ShaderNodeTexVoronoi")
        tint.inputs["Scale"].default_value = 1 / size
        tint.inputs["Randomness"].default_value = 0.85
        links.new(coordinates.outputs["Object"], tint.inputs["Vector"])
        scales.append((cells.outputs["Distance"], tint.outputs["Color"]))
    # domes: a scale rises from its groove (distance to its edge) and flattens on top
    small = bake.math(nodes, links, "MULTIPLY", scales[0][0], 9.0, clamp=True)
    big = bake.math(nodes, links, "MULTIPLY", scales[1][0], 5.0, clamp=True)
    height = bake.math(nodes, links, "ADD", bake.math(nodes, links, "MULTIPLY", small, 0.55),
                       bake.math(nodes, links, "MULTIPLY", big, 0.45))
    groove = bake.math(nodes, links, "POWER", height, 0.6)
    # its colour: the texture's light and dark (a dark back, pale flanks and belly) as an earthy olive brown, a little
    # of its own hue kept; each scale a little different
    grey = nodes.new("ShaderNodeRGBToBW")
    links.new(texture.outputs["Color"], grey.inputs["Color"])
    ramp = nodes.new("ShaderNodeValToRGB")
    ramp.color_ramp.elements[0].color = (*kit.srgb_to_linear((0.12, 0.1, 0.07)), 1)
    ramp.color_ramp.elements[1].position = 0.6
    ramp.color_ramp.elements[1].color = (*kit.srgb_to_linear((0.6, 0.52, 0.38)), 1)
    links.new(grey.outputs["Val"], ramp.inputs["Fac"])
    warmed = nodes.new("ShaderNodeMix")
    warmed.data_type = "RGBA"
    warmed.inputs["Factor"].default_value = 0.2
    links.new(ramp.outputs["Color"], warmed.inputs[6])
    links.new(texture.outputs["Color"], warmed.inputs[7])
    shade = nodes.new("ShaderNodeMix")
    shade.data_type = "RGBA"
    shade.blend_type = "MULTIPLY"
    links.new(bake.math(nodes, links, "SUBTRACT", 1.0, groove), shade.inputs["Factor"])
    links.new(warmed.outputs[2], shade.inputs[6])
    shade.inputs[7].default_value = (*kit.srgb_to_linear((0.45, 0.42, 0.38)), 1)
    varied = nodes.new("ShaderNodeMix")
    varied.data_type = "RGBA"
    varied.blend_type = "OVERLAY"
    varied.inputs["Factor"].default_value = 0.12
    links.new(shade.outputs[2], varied.inputs[6])
    links.new(scales[0][1], varied.inputs[7])
    roughness = bake.math(nodes, links, "SUBTRACT", 0.85, bake.math(nodes, links, "MULTIPLY", groove, 0.3))
    return varied.outputs[2], roughness, height


def skinned(figure):
    """The baked hide as the mesh's one material."""
    colour, normal, roughness = bake.bake(figure, FOLDER, "triceratops_skin", TEXTURE_SIZE, skin_graph)
    figure.data.materials.clear()
    figure.data.materials.append(kit.material("skin", colour, normal, roughness=roughness))


def stunned(armature, figure, die):
    """Slumped onto its belly as early in its die (it rolls over later), down on the ground, the head swaying to and
    fro and nodding, dazed."""
    armature.animation_data.action = die
    start, end = die.frame_range
    bpy.context.scene.frame_set(int(round(start + SLUMP * (end - start))))
    lying = {bone.name: bone.matrix_basis.copy() for bone in armature.pose.bones}
    armature.animation_data.action = None
    for bone in armature.pose.bones:
        bone.matrix_basis = lying[bone.name]
    bpy.context.view_layer.update()
    root = armature.pose.bones[0]
    evaluated = figure.evaluated_get(bpy.context.evaluated_depsgraph_get())
    lowest = min(vertex.co.z for vertex in evaluated.to_mesh().vertices)
    evaluated.to_mesh_clear()
    root.matrix = Matrix.Translation((0, 0, -lowest)) @ root.matrix
    bpy.context.view_layer.update()
    lying[root.name] = root.matrix_basis.copy()
    action = bpy.data.actions.new("stunned")
    armature.animation_data.action = action
    neck = armature.pose.bones[NECK]
    for frame in range(STUNNED_FRAMES + 1):
        t = 2 * math.pi * frame / STUNNED_FRAMES
        for bone in armature.pose.bones:
            bone.matrix_basis = lying[bone.name]
        bpy.context.view_layer.update()
        sway = Quaternion((0, 0, 1), DAZE * math.sin(t)) @ Quaternion((1, 0, 0), 0.4 * DAZE * math.sin(2 * t))
        head = neck.head.copy()
        neck.matrix = Matrix.Translation(head) @ sway.to_matrix().to_4x4() @ Matrix.Translation(-head) @ neck.matrix
        for bone in armature.pose.bones:
            bone.rotation_mode = "QUATERNION"
            for path in ("rotation_quaternion", "location", "scale"):
                bone.keyframe_insert(path, frame=frame)
    armature.animation_data.action = None
    return action


def walker_actions(armature, figure, actions):
    """The walker's actions, each on its own NLA track (the glTF exporter takes them all); the others gone."""
    made = {"stunned": stunned(armature, figure, actions["die"])}
    for name, own in OWN.items():
        actions[own].name = name
        made[name] = actions[own]
    for action in list(bpy.data.actions):
        if action not in made.values():
            bpy.data.actions.remove(action)
    for name, action in made.items():
        track = armature.animation_data.nla_tracks.new()
        track.name = name
        track.strips.new(name, int(action.frame_range[0]), action)
    for bone in armature.pose.bones:
        bone.matrix_basis = Matrix()


def build():
    armature, figure, actions = imported()
    normalised(armature, figure, actions)
    refined(figure)
    skinned(figure)
    walker_actions(armature, figure, actions)
    armature.name = "walker_triceratops_rig"
    figure.name = "walker_triceratops"
    kit.export(os.path.join(FOLDER, "walker_triceratops.glb"), [armature, figure])


build()
