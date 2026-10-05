"""The flyer of the remake (the original's pterodactyl): a pteranodon as pterodactyl.glb.

About 1.9 m from beak to tail and 3.2 m across its wings (the original's sprite is 32 x 23 px), facing -Y, its origin
the middle of its body: a slim body with a deep chest of flight muscles, a long neck, a long toothless beak whose
lower half opens (jaw), a crest sweeping back, wet eyes; arms of three bones, the long wing finger to the tip;
hind legs; the wings thin leathery membranes from the wing finger's tip to the ankles and from the wrist to the
shoulder (membrane). Its hide and wings are baked into one set of maps (ugh_bake): a dark brown back and a pale belly
over fine pebbly skin and a short fuzz, the crest red and ochre in bands, the beak horn darker to its tip; the wings
warm brown, lighter where they are thin (the light shows through them: brighter in their middles and between the
fibres that stiffen them from the finger to the trailing edge), with dark branching veins. Actions (each a loop):

- fly: two beats of the wings (the hand trails the arm on the downstroke, the wing half folds on the upstroke) and a
  glide (the original's flight shows 12 sprites a loop),
- fall: tumbling head over tail, the wings limp, the beak open (a passenger hit it).

    blender -b --factory-startup --python-exit-code 1 --python pterodactyl.py -- <folder>
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import bmesh  # noqa: E402
import bpy  # noqa: E402
from mathutils import Vector  # noqa: E402

import creature_kit as creature  # noqa: E402
import ugh_bake as bake  # noqa: E402
import ugh_blobs as blobs  # noqa: E402
import ugh_kit as kit  # noqa: E402
import ugh_rig  # noqa: E402
from ugh_rig import Pose, sides, turn  # noqa: E402

FOLDER = kit.arguments()[0]
RESOLUTION = 0.016
MAP_SIZE = 2048
SLOTS = ("skin", "membrane", "eye")
PARTS = {"skin": 0, "beak": 1, "jaw": 1.01, "crest": 2, "membrane": 3}   # the face attribute part: what the bake
# paints (the jaw is horn as the beak; the rig tells it by its own value)
# the arm along the wing's leading edge: shoulder, elbow, wrist, the wing finger's tip (the left one, +X)
ARM = ((0.13, -0.18, 0.1), (0.55, -0.12, 0.16), (0.95, -0.1, 0.14), (1.6, 0.28, 0.06))
LEG = ((0.09, 0.3, -0.04), (0.14, 0.52, -0.1), (0.17, 0.72, -0.12))   # hip, knee, ankle
EYE = (0.07, -0.73, 0.33)
CENTRE = Vector((0, 0.05, 0))
BONES = {
    "root": ((0, 0, 0), (0, 0, 0.1), None),
    "body": ((0, 0.35, 0), (0, -0.3, 0.06), "root"),
    "neck": ((0, -0.3, 0.06), (0, -0.6, 0.25), "body"),
    "head": ((0, -0.6, 0.25), (0, -1.25, 0.2), "neck"),
    "jaw": ((0, -0.8, 0.22), (0, -1.22, 0.17), "head"),
    "tail": ((0, 0.35, 0), (0, 0.62, 0.02), "body"),
    **ugh_rig.mirrored({
        "upperarm.L": (ARM[0], ARM[1], "body"),
        "forearm.L": (ARM[1], ARM[2], "upperarm.L"),
        "finger.L": (ARM[2], ARM[3], "forearm.L"),
        "thigh.L": (LEG[0], LEG[1], "body"),
        "shin.L": (LEG[1], LEG[2], "thigh.L"),
    }),
}


def metaballs(name):
    return blobs.metaballs(name, RESOLUTION)


def body():
    """The body, neck and head, the arms and legs (metaballs)."""
    holder, data = metaballs("Body")
    blobs.ellipsoid(data, (0, 0.08, -0.01), (0.13, 0.32, 0.13))         # the belly
    blobs.ellipsoid(data, (0, -0.16, 0.03), (0.17, 0.2, 0.16))          # the chest's flight muscles
    blobs.capsule(data, (0, -0.28, 0.08), (0, -0.56, 0.24), 0.075)      # the neck
    blobs.ellipsoid(data, (0, -0.68, 0.29), (0.085, 0.15, 0.09))        # the skull
    blobs.capsule(data, (0, 0.35, 0.0), (0, 0.62, 0.03), 0.03)          # the tail
    for side in blobs.both_sides([*ARM, *LEG]):
        shoulder, elbow, wrist, tip, hip, knee, ankle = side
        blobs.capsule(data, shoulder, elbow, 0.055)
        blobs.capsule(data, elbow, wrist, 0.04)
        blobs.capsule(data, wrist, tip, 0.018)
        blobs.ball(data, wrist, 0.045)
        blobs.capsule(data, hip, knee, 0.045)
        blobs.capsule(data, knee, ankle, 0.025)
        blobs.ellipsoid(data, Vector(ankle) + Vector((0, 0.05, -0.02)), (0.03, 0.06, 0.015))   # the foot
    for eye in blobs.both_sides([EYE]):
        blobs.ball(data, eye[0], 0.032, negative=True)   # its socket
    return holder


def beak(name, root, tip, height, width):
    """A half of the beak from `root` to its pointed `tip`, tapering, flat sideways (taller than wide)."""
    holder, data = metaballs(name)
    steps = 14
    for step in range(steps + 1):
        s = step / steps
        taper = (1 - s) ** 0.8
        blobs.ellipsoid(data, Vector(root).lerp(Vector(tip), s),
                        (width * taper + 0.003, 0.035, height * taper + 0.003))
    return holder


def crest():
    holder, data = metaballs("Crest")
    steps = 8
    for step in range(steps + 1):
        s = step / steps
        blobs.ellipsoid(data, (0, -0.62 + 0.5 * s, 0.35 + 0.05 * s), (0.014 - 0.006 * s, 0.07, 0.05 - 0.03 * s))
    return holder


def along(points, s):
    """The point at `s` (0 .. 1) of the length of polyline `points`."""
    points = [Vector(p) for p in points]
    lengths = [(b - a).length for a, b in zip(points, points[1:])]
    left = s * sum(lengths)
    for (a, b), length in zip(zip(points, points[1:]), lengths):
        if left <= length:
            return a.lerp(b, left / length)
        left -= length
    return points[-1]


def wing_grid(bm, lead, trail, columns, rows, sag):
    """A membrane between the polylines `lead` and `trail` (both sides, mirrored), its trailing edge curving in."""
    for sign in (1, -1):
        def mirror(p):
            return Vector((sign * p.x, p.y, p.z))
        grid = []
        for i in range(columns + 1):
            s = i / columns
            front, back = along(lead, s), along(trail, s)
            back = back + (front - back) * (sag * math.sin(math.pi * s))   # the scalloped trailing edge
            grid.append([bm.verts.new(mirror(front.lerp(back, j / rows) +
                                             Vector((0, 0, -0.025 * math.sin(math.pi * j / rows)))))
                         for j in range(rows + 1)])
        for i in range(columns):
            for j in range(rows):
                quad = [grid[i][j], grid[i + 1][j], grid[i + 1][j + 1], grid[i][j + 1]]
                face = bm.faces.new(quad if sign > 0 else list(reversed(quad)))
                face.smooth = True


def membranes(bm):
    """The wing from the arm and finger back to the ankle, and the little one before the arm."""
    wing_grid(bm, ARM, [Vector(LEG[0]), Vector(LEG[2]), Vector(ARM[3])], 28, 10, 0.18)
    wing_grid(bm, [Vector(ARM[0]) + Vector((0, -0.02, 0.01)), Vector(ARM[2])],
              [Vector(ARM[0]) + Vector((0, -0.12, 0.02)), Vector(ARM[2]) + Vector((0, -0.05, 0))], 8, 3, 0.0)


def hide_graph(nodes, links):
    """The bake: what part each face is (its attribute part), where it is (the model's space) and which way it faces."""
    coordinates = nodes.new("ShaderNodeTexCoord")
    geometry = nodes.new("ShaderNodeNewGeometry")
    part = nodes.new("ShaderNodeAttribute")
    part.attribute_name = "part"
    position = coordinates.outputs["Object"]

    def is_part(name):
        return bake.math(nodes, links, "COMPARE", part.outputs["Fac"], float(round(PARTS[name])))

    def mix(factor, a, b):
        made = nodes.new("ShaderNodeMix")
        made.data_type = "RGBA"
        links.new(factor, made.inputs["Factor"])
        for socket, value in ((made.inputs[6], a), (made.inputs[7], b)):
            if isinstance(value, bpy.types.NodeSocket):
                links.new(value, socket)
            else:
                socket.default_value = (*kit.srgb_to_linear(value), 1)
        return made.outputs[2]

    def noise(scale, detail=4.0):
        made = nodes.new("ShaderNodeTexNoise")
        made.inputs["Scale"].default_value = scale
        made.inputs["Detail"].default_value = detail
        links.new(position, made.inputs["Vector"])
        return made.outputs["Fac"]

    def cells(scale):
        made = nodes.new("ShaderNodeTexVoronoi")
        made.feature = "DISTANCE_TO_EDGE"
        made.inputs["Scale"].default_value = scale
        links.new(position, made.inputs["Vector"])
        return made.outputs["Distance"]

    def ridges(scale, width):
        """Thin branching lines (veins): where a distorted noise crosses its middle."""
        made = nodes.new("ShaderNodeTexNoise")
        made.inputs["Scale"].default_value = scale
        made.inputs["Detail"].default_value = 3.0
        made.inputs["Distortion"].default_value = 0.6
        links.new(position, made.inputs["Vector"])
        away = bake.math(nodes, links, "ABSOLUTE", bake.math(nodes, links, "SUBTRACT", made.outputs["Fac"], 0.5))
        return bake.math(nodes, links, "SUBTRACT", 1.0, bake.math(nodes, links, "DIVIDE", away, width, clamp=True))

    # the hide: countershaded (dark where it faces up), mottled, fine pebbly scales under a short fuzz
    up = nodes.new("ShaderNodeSeparateXYZ")
    links.new(geometry.outputs["Normal"], up.inputs["Vector"])
    belly = bake.math(nodes, links, "MULTIPLY", bake.math(nodes, links, "SUBTRACT", 0.15, up.outputs["Z"]), 2.5,
                      clamp=True)
    mottle = noise(9.0)
    pebbles = bake.math(nodes, links, "MULTIPLY", cells(160.0), 7.0, clamp=True)
    fuzz = noise(420.0, 2.0)
    back = mix(mottle, (0.11, 0.075, 0.05), (0.24, 0.16, 0.1))
    skin = mix(belly, back, (0.6, 0.5, 0.38))
    skin = mix(bake.math(nodes, links, "MULTIPLY", fuzz, 0.35), skin, (0.05, 0.035, 0.025))
    # the crest in red and ochre bands along it, the beak horn darker towards its tip
    bands = bake.math(nodes, links, "SINE", bake.math(nodes, links, "MULTIPLY", nodes_y(nodes, links, position), 38.0))
    crest_colour = mix(bake.math(nodes, links, "MULTIPLY", bake.math(nodes, links, "ADD", bands, 1.0), 0.5),
                       (0.45, 0.08, 0.03), (0.62, 0.36, 0.1))
    tip = bake.math(nodes, links, "MULTIPLY", bake.math(nodes, links, "SUBTRACT", -0.95,
                                                        nodes_y(nodes, links, position)), 3.5, clamp=True)
    horn = mix(tip, (0.58, 0.5, 0.36), (0.16, 0.13, 0.1))
    # the wing: warm brown, thin (lighter) between the fibres and away from the arm, dark branching veins
    fibres = bake.math(nodes, links, "POWER", bake.math(nodes, links, "ABSOLUTE", bake.math(
        nodes, links, "SINE", bake.math(nodes, links, "MULTIPLY", fan(nodes, links, position), 260.0))), 6.0)
    veins = ridges(3.0, 0.012)
    small_veins = ridges(9.0, 0.008)
    thin = bake.math(nodes, links, "MULTIPLY", noise(4.0), 0.8)
    wing = mix(thin, (0.3, 0.17, 0.09), (0.66, 0.42, 0.22))
    wing = mix(bake.math(nodes, links, "MULTIPLY", fibres, 0.25), wing, (0.22, 0.12, 0.07))
    wing = mix(bake.math(nodes, links, "MAXIMUM", bake.math(nodes, links, "MULTIPLY", veins, 0.55),
                         bake.math(nodes, links, "MULTIPLY", small_veins, 0.3)), wing, (0.2, 0.08, 0.05))
    colour = mix(is_part("crest"), skin, crest_colour)
    colour = mix(is_part("beak"), colour, horn)
    colour = mix(is_part("membrane"), colour, wing)
    # relief: pebbles and fuzz on the hide, veins and fibres on the wing; the horn smooth and glossier
    membrane = is_part("membrane")
    hide_height = bake.math(nodes, links, "ADD", bake.math(nodes, links, "MULTIPLY", pebbles, 0.6),
                            bake.math(nodes, links, "MULTIPLY", fuzz, 0.3))
    wing_height = bake.math(nodes, links, "ADD", bake.math(nodes, links, "MULTIPLY", veins, 0.8),
                            bake.math(nodes, links, "MULTIPLY", fibres, 0.25))
    height = bake.math(nodes, links, "ADD", bake.math(nodes, links, "MULTIPLY", membrane, wing_height),
                       bake.math(nodes, links, "MULTIPLY", bake.math(nodes, links, "SUBTRACT", 1.0, membrane),
                                 hide_height))
    horny = bake.math(nodes, links, "ADD", is_part("beak"), is_part("crest"))
    roughness = bake.math(nodes, links, "SUBTRACT", 0.82, bake.math(nodes, links, "MULTIPLY", horny, 0.35))
    return colour, roughness, height


def nodes_y(nodes, links, position):
    """The Y of a position (along the body: the head towards -Y)."""
    split = nodes.new("ShaderNodeSeparateXYZ")
    links.new(position, split.inputs["Vector"])
    return split.outputs["Y"]


def fan(nodes, links, position):
    """The angle about the shoulder (radians, in the wing's plane): the fibres fan out from the finger along it."""
    split = nodes.new("ShaderNodeSeparateXYZ")
    links.new(position, split.inputs["Vector"])
    across = bake.math(nodes, links, "SUBTRACT", bake.math(nodes, links, "ABSOLUTE", split.outputs["X"]), ARM[0][0])
    along = bake.math(nodes, links, "SUBTRACT", split.outputs["Y"], ARM[0][1])
    return bake.math(nodes, links, "ARCTAN2", along, across)


def marked(made, name):
    """`made` with its faces' attribute part set to what `name` is (the bake paints by it)."""
    attribute = made.data.attributes.new("part", "FLOAT", "FACE")
    for each in attribute.data:
        each.value = PARTS[name]
    return made


def fly(t):
    """Two beats of the wings (the hand trails the arm going down, the wing half folds coming up), then a glide; the
    body rises as they beat down."""
    u = t / (2 * math.pi)
    beating = min(1.0, max(0.0, (0.68 - u) / 0.12) + max(0.0, (u - 0.88) / 0.12))
    stroke = math.cos(6 * math.pi * u)            # 1 up .. -1 down
    lag = math.cos(6 * math.pi * u - 0.7)         # the hand a little later
    rising = max(0.0, math.sin(6 * math.pi * u))  # coming up: the wing folds a little
    lift = beating * 0.7 * stroke + (1 - beating) * 0.06
    pose = Pose()
    pose.move["body"] = Vector(BONES["body"][0]) + Vector((0, 0, -0.06 * beating * stroke))
    pose.turn["head"] = turn(x=0.05 * lift)
    pose.turn["tail"] = turn(x=0.15 * lift)
    for side, name in sides():
        pose.turn[f"upperarm.{name}"] = turn(y=-side * lift, z=side * (0.12 * lift - 0.25 * beating * rising))
        pose.turn[f"forearm.{name}"] = turn(y=-side * 0.3 * beating * lag, z=side * 0.35 * beating * rising)
        pose.turn[f"finger.{name}"] = turn(y=-side * (0.35 * beating * lag + 0.05),
                                           z=side * (0.1 - 0.4 * beating * rising))
        pose.aim[f"thigh.{name}"] = Vector((side * 0.15, 1, -0.3))
        pose.aim[f"shin.{name}"] = Vector((side * 0.1, 1, -0.1))
    return pose


def fall(t):
    """Head over tail about its middle, the wings limp and half up, the beak open."""
    pose = Pose()
    spin = turn(x=-t)
    pose.move["body"] = CENTRE + spin @ (Vector(BONES["body"][0]) - CENTRE)
    pose.turn["body"] = spin
    pose.turn["jaw"] = spun(spin, turn(x=-0.5))
    for side, name in sides():
        flop = 0.6 + 0.2 * math.sin(2 * t + side)
        pose.turn[f"upperarm.{name}"] = spun(spin, turn(y=-side * flop))
        pose.turn[f"forearm.{name}"] = spun(spin, turn(y=side * 0.5, z=side * 0.5))
        pose.turn[f"finger.{name}"] = spun(spin, turn(y=side * 0.4, z=side * 0.4))
    return pose


def spun(spin, rotation):
    """`rotation` about the axes of a body turned by `spin`."""
    return spin @ rotation @ spin.conjugated()


ACTIONS = {"fly": (fly, 48), "fall": (fall, 36)}


def build():
    kit.clear_scene()
    parts = [("skin", body()), ("beak", beak("Beak", (0, -0.74, 0.275), (0, -1.28, 0.205), 0.04, 0.03)),
             ("jaw", beak("Jaw", (0, -0.78, 0.23), (0, -1.22, 0.195), 0.022, 0.022)), ("crest", crest())]
    meshes = [marked(blobs.to_mesh(holder, 0, [], 0.6), name) for name, holder in parts]
    bm = bmesh.new()
    membranes(bm)
    meshes.append(marked(kit.mesh_object("Wings", bm, []), "membrane"))
    figure = blobs.join(meshes, "pterodactyl")
    blobs.unwrap(figure)
    colour, normal, roughness = bake.bake(figure, FOLDER, "pterodactyl_hide", MAP_SIZE, hide_graph,
                                          bump_distance=0.002)
    slots = [kit.material("skin", colour, normal, roughness=roughness),
             kit.material("membrane", colour, normal, roughness=roughness, double_sided=True),
             creature.wet_eye_material(FOLDER, "pterodactyl_eye", iris=1.0, pupil=0.4)]
    figure.data.materials.clear()
    for material in slots:
        figure.data.materials.append(material)
    part = figure.data.attributes["part"]
    for polygon in figure.data.polygons:
        membrane = part.data[polygon.index].value == PARTS["membrane"]
        polygon.material_index = SLOTS.index("membrane" if membrane else "skin")
    eyes = bmesh.new()
    for side, at in zip((1, -1), blobs.both_sides([EYE])):
        creature.wet_eye(eyes, Vector(at[0]) + Vector((side * 0.006, 0, 0)), 0.02, (side * 0.9, -0.6, 0.15),
                         SLOTS.index("eye"))
    eye_object = marked(kit.mesh_object("Eyes", eyes, slots), "skin")
    eye_object.data.uv_layers[0].name = figure.data.uv_layers[0].name   # one UV map once joined
    figure = blobs.join([figure, eye_object], "pterodactyl")
    armature = ugh_rig.build("pterodactyl", BONES, figure, falloff=0.05, slot_bones={"eye": "head"},
                             slot_allowed={"skin": [name for name in BONES if name != "jaw"]})
    jaw_only(figure)
    ugh_rig.make(armature, BONES, ACTIONS)
    kit.export(os.path.join(FOLDER, "pterodactyl.glb"), [armature, figure])


def jaw_only(figure):
    """The lower beak follows the jaw alone."""
    groups = figure.vertex_groups
    part = figure.data.attributes["part"]
    lower = set()
    for polygon in figure.data.polygons:
        if abs(part.data[polygon.index].value - PARTS["jaw"]) < 1e-3:
            lower.update(polygon.vertices)
    for index in lower:
        for entry in list(figure.data.vertices[index].groups):
            groups[entry.group].remove([index])
        groups["jaw"].add([index], 1.0, "REPLACE")


build()
