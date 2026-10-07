"""The stone of the remake (the original's standing passenger: the stone the copter carries in its sling, drops onto the
enemies and has sitting on its seat) as stone_slate.glb, after Jan's reference: a boulder of dark blue grey slate in
beds of sharp fractured flakes, strong relief, thin white quartz veins along the beds and a few across them, matte;
two wet eyes sunk into its face (-Y: the camera's side) under heavy lids of the rock, as the stone of step 19h had.

The boulder fills the stone passenger's ellipsoid (copter_layout.HANGING_SEMI: 1.58 x 0.84 x 1.06 m, the sprite's
16 x 11 px; it hangs in the sling and sits on the seat as the older ones did), its origin the bottom middle (where it
stands). Made as a detailed surface (a cube sphere of about 400 thousand triangles: a broken, lumpy block with flat
fracture faces and a flat foot, its beds - dipping as the whole sea stack's, UghRockNoise::StrataDip - each out most at
its foot, a sharp lip shading the bed below, broken into flakes standing out each its own way with cracks between them,
thin laminae on them) baked onto a light one (about 5 thousand triangles; the engine's Nanite takes it further down far
away): its colour (the slate dark_rock_02 seen from three sides, its tone, darker in the seams and cracks, paler
weathered streaks, the veins), normal (the surface's flakes and laminae, the slate's relief) and roughness maps.

    blender -b --factory-startup --python-exit-code 1 --python stone_slate.py -- <folder> <dark_rock_02 folder>
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import bmesh  # noqa: E402
import bpy  # noqa: E402
import numpy  # noqa: E402
from mathutils import Vector  # noqa: E402

import copter_layout as layout  # noqa: E402
import creature_kit as creature  # noqa: E402
import ugh_bake as bake  # noqa: E402
import ugh_kit as kit  # noqa: E402

FOLDER, SLATE = kit.arguments()[:2]
DETAIL = 7               # the detailed surface: a cube subdivided this often (128 x 128 quads a side)
FACES = 2600             # the light one's faces (quads and triangles; about 5000 triangles)
MAP_SIZE = 2048
SEED = 25
DIP = 0.12               # the beds rise to the right as the stone's (UghRockNoise::StrataDip, about 0.14)
BED, LAMINA = 0.13, 0.028          # how thick the beds and the laminae on them are (m)
BED_OUT, LAMINA_OUT = 0.06, 0.006  # how far a bed's (a lamina's) foot stands out (m)
TONE = (0.032, 0.036, 0.045)       # the slate's albedo (linear), dark: the figures' lights (24c) brighten it
VEIN = (0.4, 0.4, 0.38)
BED_VEINS = (-0.2, -0.04, 0.11, 0.27)      # the veins along the beds: at these heights of them (m)
VEIN_WIDTHS = (0.0024, 0.0018, 0.003, 0.002, 0.0016, 0.0012)   # half widths: those, then the two across (m)
TILE = 0.9               # the slate's picture repeats every this many metres
EYE_RADIUS = 0.1
EYES = (0.19, 0.6)       # the eyes: how far from the middle aside, at what part of the height
SINK = 0.45              # how much of an eye's radius is in the rock
LID, LID_LOW = 1.07, 0.25  # the lids' shell (of the eye's radius) and how low they come down over it


# ------------------------------------------------------------------ noise (numpy, points n x 3)

def _hash(cells, seed):
    """A number 0 .. 1 for every integer cell (n x 3)."""
    c = cells.astype(numpy.int64).astype(numpy.uint64)
    h = (c[:, 0] * numpy.uint64(73856093)) ^ (c[:, 1] * numpy.uint64(19349663)) ^ \
        (c[:, 2] * numpy.uint64(83492791)) ^ numpy.uint64(seed * 2654435761 % 2 ** 32)
    h ^= h >> numpy.uint64(13)
    h *= numpy.uint64(1274126177)
    h ^= h >> numpy.uint64(16)
    return (h & numpy.uint64(0xFFFFFF)).astype(numpy.float64) / 0xFFFFFF


def value_noise(points, seed):
    """Smooth value noise -1 .. 1."""
    base = numpy.floor(points)
    f = points - base
    u = f * f * (3 - 2 * f)
    total = numpy.zeros(len(points))
    for corner in range(8):
        offset = numpy.array(((corner >> 0) & 1, (corner >> 1) & 1, (corner >> 2) & 1))
        weight = numpy.prod(numpy.where(offset == 1, u, 1 - u), axis=1)
        total += weight * _hash(base + offset, seed)
    return total * 2 - 1


def fbm(points, seed, octaves=4, gain=0.5):
    """Fractal value noise, about -1 .. 1."""
    total, amplitude, norm = numpy.zeros(len(points)), 1.0, 0.0
    for octave in range(octaves):
        total += amplitude * value_noise(points * 2.0 ** octave, seed + octave * 31)
        norm += amplitude
        amplitude *= gain
    return total / norm


def cells(points, seed):
    """Jittered cells: the distance to the nearest point and to the second nearest, and the nearest one's number."""
    base = numpy.floor(points)
    first = numpy.full(len(points), 9.0)
    second = numpy.full(len(points), 9.0)
    number = numpy.zeros(len(points))
    for dx in (-1, 0, 1):
        for dy in (-1, 0, 1):
            for dz in (-1, 0, 1):
                cell = base + numpy.array((dx, dy, dz))
                jitter = numpy.stack([_hash(cell, seed + axis) for axis in range(3)], axis=1) * 0.8 + 0.1
                distance = numpy.linalg.norm(cell + jitter - points, axis=1)
                nearer = distance < first
                second = numpy.where(nearer, first, numpy.minimum(second, distance))
                number = numpy.where(nearer, _hash(cell, seed + 7), number)
                first = numpy.minimum(first, distance)
    return first, second, number


def smoothstep(low, high, x):
    t = numpy.clip((x - low) / (high - low), 0, 1)
    return t * t * (3 - 2 * t)


def smooth_min(a, b, k):
    h = numpy.clip(0.5 + 0.5 * (b - a) / k, 0, 1)
    return b + (a - b) * h - k * h * (1 - h)


# ------------------------------------------------------------------ the shape

def cube_sphere():
    """A cube subdivided DETAIL times, its points on the unit sphere (spread evenly: the cube's own mapping)."""
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=2)
    mesh = bpy.data.meshes.new("slate_detail")
    bm.to_mesh(mesh)
    bm.free()
    made = kit.new_object("slate_detail", mesh)
    subdivision = made.modifiers.new("detail", "SUBSURF")
    subdivision.subdivision_type = "SIMPLE"
    subdivision.levels = subdivision.render_levels = DETAIL
    bpy.context.view_layer.objects.active = made
    bpy.ops.object.modifier_apply(modifier=subdivision.name)
    points = numpy.empty(len(made.data.vertices) * 3)
    made.data.vertices.foreach_get("co", points)
    p = points.reshape(-1, 3)
    x, y, z = p[:, 0], p[:, 1], p[:, 2]
    sphere = numpy.stack((x * numpy.sqrt(1 - y * y / 2 - z * z / 2 + y * y * z * z / 3),
                          y * numpy.sqrt(1 - z * z / 2 - x * x / 2 + z * z * x * x / 3),
                          z * numpy.sqrt(1 - x * x / 2 - y * y / 2 + x * x * y * y / 3)), axis=1)
    return made, sphere


def block(directions):
    """The boulder's body (metres, about its middle) along `directions`: a lumpy, squarish ellipsoid broken by flat
    fracture faces (the front one broad, towards the camera), higher on its left, its upper right standing out, its
    foot flat."""
    semi = numpy.array((0.8, 0.46, 0.56))
    d = directions
    radius = numpy.sum(numpy.abs(d / semi) ** 2.6, axis=1) ** (-1 / 2.6)
    radius *= 1 + 0.07 * fbm(d * 1.4 + 3.1, SEED, octaves=3) + 0.03 * fbm(d * 4.5, SEED + 50, octaves=2)
    for towards, amount, width in (((-0.35, 0.1, 0.93), 0.12, 0.35), ((0.82, -0.1, 0.5), 0.07, 0.15),
                                   ((-0.9, 0.2, -0.2), -0.05, 0.2)):
        towards = numpy.array(towards) / numpy.linalg.norm(towards)
        radius *= 1 + amount * numpy.exp(-numpy.sum((d - towards) ** 2, axis=1) / width)
    # fracture faces: a plane of normal n at h cuts the body where it is further (rounded a little)
    for normal, height in (((0.0, -1.0, 0.04), 0.38), ((0.25, -0.75, 0.6), 0.6), ((-0.6, -0.7, 0.3), 0.6),
                           ((0.9, -0.2, -0.15), 0.72), ((-0.15, 0.2, 1.0), 0.6), ((0.55, 0.4, 0.75), 0.62),
                           ((-0.5, 0.85, 0.2), 0.48), ((0.4, 0.9, -0.1), 0.45), ((-0.95, -0.1, 0.25), 0.74)):
        normal = numpy.array(normal) / numpy.linalg.norm(normal)
        facing = d @ normal
        cut = numpy.where(facing > 0.05, height / numpy.maximum(facing, 0.05), 9.0)
        radius = smooth_min(radius, cut, 0.05)
    points = d * radius[:, None]
    # the foot flat
    foot = -0.41
    points[:, 2] = -smooth_min(-points[:, 2], -foot, 0.06)
    return points


def normals_of(made, points):
    made.data.vertices.foreach_set("co", points.ravel())
    made.data.update()
    normals = numpy.empty(len(points) * 3)
    made.data.vertex_normals.foreach_get("vector", normals)
    normals = normals.reshape(-1, 3)
    # smoothed a little: along a crease between two fracture faces the beds go out the way of both, not of one
    edges = numpy.empty(len(made.data.edges) * 2, dtype=numpy.int64)
    made.data.edges.foreach_get("vertices", edges)
    edges = edges.reshape(-1, 2)
    for _ in range(6):
        summed = normals.copy()
        numpy.add.at(summed, edges[:, 0], normals[edges[:, 1]])
        numpy.add.at(summed, edges[:, 1], normals[edges[:, 0]])
        normals = summed / numpy.linalg.norm(summed, axis=1, keepdims=True)
    return normals


def beds(points, normals):
    """The slate on the body: how far each point goes out along its normal (the beds, their flakes and cracks, the
    laminae) and what it looks like there: its tone, its roughness and the fields of the quartz veins (VEINS: their
    signed distances, which the bake's material turns into thin lines sharper than the points, and where they are
    broken)."""
    x, y, z = points[:, 0], points[:, 1], points[:, 2]
    along = z - DIP * x
    # the beds wave a little and step up or down from one block of the stone to the next (fractured, not stacked)
    _, _, block_number = cells(points * numpy.array((3.0, 3.0, 4.0)), SEED + 13)
    s = along / BED + 0.5 * fbm(points * 1.5, SEED + 1) + 0.12 * fbm(points * 6, SEED + 11) + 0.7 * block_number
    bed = numpy.floor(s)
    up = s - bed                            # 0 at a bed's foot .. 1 at its top
    bed_number = _hash(numpy.stack((bed, bed * 0, bed * 0), axis=1), SEED + 2)
    # flakes: big irregular scales of a bed, each standing out its own way; a crack between some of them
    flake_at = numpy.stack((x * 2.6, y * 2.6, bed * 1.37 + 0.5), axis=1) + \
        numpy.stack([0.45 * fbm(points * 3.5, SEED + 3 + axis) for axis in range(3)], axis=1)
    first, second, flake_number = cells(flake_at, SEED + 4)
    crack = (1 - smoothstep(0.0, 0.05, second - first)) * smoothstep(0.0, 0.4, fbm(points * 2.5, SEED + 12))
    lip = 0.06
    profile = numpy.where(up < lip, up / lip, ((1 - up) / (1 - lip)) ** 0.6)
    out = BED_OUT * (0.35 + 0.9 * bed_number) * (0.2 + 1.3 * flake_number ** 2) * profile - 0.008 * crack
    s2 = along / LAMINA + 0.6 * fbm(points * 7, SEED + 5) + 3 * bed_number
    up2 = s2 - numpy.floor(s2)
    out += LAMINA_OUT * numpy.where(up2 < 0.15, up2 / 0.15, ((1 - up2) / 0.85) ** 0.8)
    # not on the foot; lower steps on what faces up (a small island of a bed on the top would stand up as a spike)
    out *= smoothstep(-0.92, -0.6, normals[:, 2]) * (1 - 0.6 * smoothstep(0.55, 0.95, normals[:, 2]))
    # what it looks like: the tone darker in the seams under a lip and in the cracks, a little different a bed and a
    # flake, paler weathered streaks along the beds and on what faces up
    seam = numpy.clip(1 - profile, 0, 1) ** 2
    shade = (0.78 + 0.44 * bed_number) * (0.86 + 0.28 * flake_number) * (1 - 0.6 * seam) * (1 - 0.3 * crack)
    streak_at = numpy.stack((x * 2.5, y * 2.5, along * 14), axis=1)
    streak = smoothstep(0.2, 0.6, fbm(streak_at, SEED + 6, octaves=3)) * (0.5 + 0.5 * profile)
    weathered = numpy.maximum(0.5 * streak, 0.35 * smoothstep(0.4, 0.95, normals[:, 2]))
    tone = numpy.array(TONE)[None, :] * shade[:, None]
    tone = tone + (numpy.array((0.1, 0.105, 0.11)) - tone) * weathered[:, None]
    roughness = numpy.clip(0.82 + 0.08 * fbm(points * 4, SEED + 10), 0, 1)
    # the veins: along a few beds (the beds' height, wandering), across them (planes); where each is broken
    wander = along + 0.05 * fbm(points * 2, SEED + 7) + 0.006 * fbm(points * 22, SEED + 8)
    fields, pieces = [], []
    for index, level in enumerate(BED_VEINS):
        fields.append(wander - level)
        pieces.append(smoothstep(-0.05, 0.25, fbm(points * 1.6, SEED + 20 + index, octaves=3)))
    for index, (normal, offset) in enumerate((((0.55, -0.1, 0.83), 0.05), ((-0.7, 0.2, 0.69), 0.18))):
        normal = numpy.array(normal) / numpy.linalg.norm(normal)
        fields.append(points @ normal + 0.01 * fbm(points * 6, SEED + 30 + index) - offset)
        pieces.append(smoothstep(0.1, 0.35, fbm(points * 2.0, SEED + 40 + index, octaves=3)))
    return out, tone, roughness, numpy.stack(fields, axis=1), numpy.stack(pieces, axis=1)


def shaped():
    """The detailed boulder, its look in colour attributes: `tone`, `rough` (roughness), `veins0`, `veins1` (the veins'
    fields), `pieces0`, `pieces1` (where they are broken)."""
    made, directions = cube_sphere()
    body = block(directions)
    normals = normals_of(made, body)
    out, tone, roughness, fields, pieces = beds(body, normals)
    points = body + normals * out[:, None]
    # into the ellipsoid of the stone passenger: its bottom middle at the origin
    low, high = points.min(axis=0), points.max(axis=0)
    size = numpy.array(layout.HANGING_SEMI) * 2
    points = (points - numpy.array(((low[0] + high[0]) / 2, (low[1] + high[1]) / 2, low[2]))) * (size / (high - low))
    made.data.vertices.foreach_set("co", points.ravel())
    made.data.update()
    looks = (("tone", tone), ("rough", numpy.repeat(roughness[:, None], 3, axis=1)), ("veins0", fields[:, :3]),
             ("veins1", fields[:, 3:]), ("pieces0", pieces[:, :3]), ("pieces1", pieces[:, 3:]))
    for name, values in looks:
        attribute = made.data.color_attributes.new(name, "FLOAT_COLOR", "POINT")
        attribute.data.foreach_set("color", numpy.hstack((values, numpy.ones((len(values), 1)))).ravel())
    made.data.shade_smooth()
    return made


def light(detail):
    """The light boulder: the detailed one collapsed to FACES, smooth, unwrapped."""
    made = kit.new_object("stone_slate", detail.data.copy())
    made.data.name = "stone_slate"
    for name in [attribute.name for attribute in made.data.color_attributes]:
        made.data.color_attributes.remove(made.data.color_attributes[name])
    lighter = made.modifiers.new("lighter", "DECIMATE")
    lighter.ratio = FACES / len(made.data.polygons)
    bpy.ops.object.select_all(action="DESELECT")
    bpy.context.view_layer.objects.active = made
    made.select_set(True)
    bpy.ops.object.modifier_apply(modifier=lighter.name)
    made.data.shade_smooth()
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=math.radians(60), island_margin=0.004)
    bpy.ops.object.mode_set(mode="OBJECT")
    return made


# ------------------------------------------------------------------ the maps

def _slate_picture(nodes, links, name):
    """A map of the slate dark_rock_02 seen from three sides (object coordinates, a TILE in metres)."""
    coordinates = nodes.new("ShaderNodeTexCoord")
    mapping = nodes.new("ShaderNodeMapping")
    mapping.inputs["Scale"].default_value = (1 / TILE,) * 3
    links.new(coordinates.outputs["Object"], mapping.inputs["Vector"])
    texture = nodes.new("ShaderNodeTexImage")
    texture.image = bpy.data.images.load(os.path.join(SLATE, name), check_existing=True)
    texture.image.colorspace_settings.name = "sRGB" if "diff" in name else "Non-Color"
    texture.projection = "BOX"
    texture.projection_blend = 0.3
    links.new(mapping.outputs["Vector"], texture.inputs["Vector"])
    return texture.outputs["Color"]


def _mean_brightness(path):
    """The mean linear brightness of an sRGB picture."""
    image = bpy.data.images.load(path, check_existing=True)
    pixels = numpy.array(image.pixels[:], dtype=numpy.float32).reshape(-1, image.channels)[::7, :3]
    linear = numpy.where(pixels <= 0.04045, pixels / 12.92, ((pixels + 0.055) / 1.055) ** 2.4)
    return float((linear @ numpy.array((0.2126, 0.7152, 0.0722))).mean())


def _attribute(nodes, links, name):
    """The channels (red, green, blue) of the detailed boulder's colour attribute `name`."""
    attribute = nodes.new("ShaderNodeAttribute")
    attribute.attribute_name = name
    split = nodes.new("ShaderNodeSeparateColor")
    links.new(attribute.outputs["Color"], split.inputs["Color"])
    return [split.outputs[channel] for channel in ("Red", "Green", "Blue")]


def _veins(nodes, links, spread=1.0):
    """How much of a quartz vein is at a point, 0 .. 1: thin lines where a vein's field is 0, as sharp as the bake
    (`spread` times as wide: the paler rock around it)."""
    fields = _attribute(nodes, links, "veins0") + _attribute(nodes, links, "veins1")
    pieces = _attribute(nodes, links, "pieces0") + _attribute(nodes, links, "pieces1")
    vein = None
    for field, piece, width in zip(fields, pieces, [width * spread for width in VEIN_WIDTHS]):
        away = bake.math(nodes, links, "ABSOLUTE", field)
        edge = bake.math(nodes, links, "DIVIDE", bake.math(nodes, links, "SUBTRACT", away, width * 0.3), width * 0.7,
                         clamp=True)
        line = bake.math(nodes, links, "MULTIPLY", bake.math(nodes, links, "SUBTRACT", 1.0, edge), piece)
        vein = line if vein is None else bake.math(nodes, links, "MAXIMUM", vein, line)
    return vein


def _graph(material, kind):
    """The detailed boulder's material for a bake: `kind` color (emitted), roughness (emitted) or normal (its relief:
    the slate's height on the surface's own)."""
    nodes, links = material.node_tree.nodes, material.node_tree.links
    nodes.clear()
    output = nodes.new("ShaderNodeOutputMaterial")
    if kind == "normal":
        bump = nodes.new("ShaderNodeBump")
        bump.inputs["Distance"].default_value = 0.006
        bump.inputs["Strength"].default_value = 0.7
        links.new(_slate_picture(nodes, links, "dark_rock_02_disp_2k.png"), bump.inputs["Height"])
        shader = nodes.new("ShaderNodeBsdfDiffuse")
        links.new(bump.outputs["Normal"], shader.inputs["Normal"])
        links.new(shader.outputs["BSDF"], output.inputs["Surface"])
        return
    emission = nodes.new("ShaderNodeEmission")
    links.new(emission.outputs["Emission"], output.inputs["Surface"])
    vein = _veins(nodes, links)
    if kind == "roughness":
        rough = _attribute(nodes, links, "rough")[0]
        links.new(bake.math(nodes, links, "SUBTRACT", rough, bake.math(nodes, links, "MULTIPLY", vein, 0.3)),
                  emission.inputs["Color"])
        return
    # the slate's picture as grey detail around 1 on the tone, the veins over it
    tone = nodes.new("ShaderNodeAttribute")
    tone.attribute_name = "tone"
    grey = nodes.new("ShaderNodeRGBToBW")
    links.new(_slate_picture(nodes, links, "dark_rock_02_diff_2k.jpg"), grey.inputs["Color"])
    detail = nodes.new("ShaderNodeMapRange")
    detail.inputs["From Min"].default_value = 0
    detail.inputs["From Max"].default_value = _mean_brightness(os.path.join(SLATE, "dark_rock_02_diff_2k.jpg"))
    detail.inputs["To Min"].default_value = 0.25
    detail.inputs["To Max"].default_value = 1.0
    detail.clamp = False
    links.new(grey.outputs["Val"], detail.inputs["Value"])
    clamp = nodes.new("ShaderNodeClamp")
    clamp.inputs["Min"].default_value, clamp.inputs["Max"].default_value = 0.3, 1.9
    links.new(detail.outputs["Result"], clamp.inputs["Value"])
    toned = nodes.new("ShaderNodeVectorMath")
    toned.operation = "SCALE"
    links.new(tone.outputs["Color"], toned.inputs[0])
    links.new(clamp.outputs["Result"], toned.inputs["Scale"])
    colour = toned.outputs["Vector"]
    for factor in (bake.math(nodes, links, "MULTIPLY", _veins(nodes, links, 6.0), 0.2), vein):
        mix = nodes.new("ShaderNodeMix")
        mix.data_type = "RGBA"
        rgba = [socket for socket in mix.inputs if socket.type == "RGBA"]
        links.new(factor, mix.inputs["Factor"])
        links.new(colour, rgba[0])
        rgba[1].default_value = (*VEIN, 1)
        colour = next(socket for socket in mix.outputs if socket.type == "RGBA")
    links.new(colour, emission.inputs["Color"])


def bake_maps(detail, made):
    """The detailed boulder's colour, normal and roughness baked onto the light one's UVs."""
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = 4
    scene.render.bake.margin = 16
    source = bpy.data.materials.new("slate_source")
    source.use_nodes = True
    detail.data.materials.append(source)
    target = bpy.data.materials.new("slate_target")
    target.use_nodes = True
    texture = target.node_tree.nodes.new("ShaderNodeTexImage")
    target.node_tree.nodes.active = texture
    made.data.materials.append(target)
    bpy.ops.object.select_all(action="DESELECT")
    detail.select_set(True)
    made.select_set(True)
    bpy.context.view_layer.objects.active = made
    images = {}
    for kind in ("color", "roughness", "normal"):
        image = bpy.data.images.new(f"stone_slate_{kind}", MAP_SIZE, MAP_SIZE, alpha=False)
        image.colorspace_settings.name = "sRGB" if kind == "color" else "Non-Color"
        texture.image = image
        _graph(source, kind)
        if kind == "normal":
            bpy.ops.object.bake(type="NORMAL", normal_space="TANGENT", use_selected_to_active=True,
                                cage_extrusion=0.03, max_ray_distance=0.09)
        else:
            bpy.ops.object.bake(type="EMIT", use_selected_to_active=True, cage_extrusion=0.03, max_ray_distance=0.09)
        image.filepath_raw = os.path.join(FOLDER, image.name + ".png")
        image.file_format = "PNG"
        image.save()
        images[kind] = image
    made.data.materials.clear()
    made.data.materials.append(kit.material("stone", images["color"], images["normal"], roughness=images["roughness"]))


# ------------------------------------------------------------------ the eyes

def eyes(rock):
    """Two eyes sunk into the front of the rock, looking forward, under heavy lids of the rock (a cap of a shell over
    the top of each eye, the rock's picture on it)."""
    bm = bmesh.new()
    height = layout.HANGING_SEMI[2] * 2
    for side in (1, -1):
        start = Vector((side * EYES[0], -10, EYES[1] * height))
        hit, point, normal, _ = rock.ray_cast(start, Vector((0, 1, 0)))
        if not hit:
            raise RuntimeError(f"no rock in front of the eye at {start}")
        look = Vector((side * 0.08, -1, 0.06))
        middle = point - normal * (SINK * EYE_RADIUS)
        creature.wet_eye(bm, middle, EYE_RADIUS, look, 0)
        creature.lid(bm, middle, LID * EYE_RADIUS, look, LID_LOW, 1)
    made = kit.mesh_object("eyes", bm, [creature.wet_eye_material(FOLDER, "stone_eye"), rock.data.materials[0]])
    made.data.uv_layers[0].name = rock.data.uv_layers[0].name   # one UV map once joined
    kit.surface_uvs(made, 1, rock)
    return made


def build():
    kit.clear_scene()
    detail = shaped()
    rock = light(detail)
    bake_maps(detail, rock)
    bpy.data.objects.remove(detail)
    made = eyes(rock)
    bpy.ops.object.select_all(action="DESELECT")
    made.select_set(True)
    rock.select_set(True)
    bpy.context.view_layer.objects.active = rock
    bpy.ops.object.join()
    print("stone_slate triangles", sum(len(polygon.vertices) - 2 for polygon in rock.data.polygons))
    kit.export(os.path.join(FOLDER, "stone_slate.glb"), [rock])


build()
