"""What the Blender scripts that make the remake's own models share: procedural textures (numpy, tileable), PBR
materials the glTF exporter understands, shapes (tubes along a path, blobs) and the export.

Units are metres, Z up, the front of a model towards -Y (the camera of the game looks at it from there). A texture is
made in the output folder as a PNG, so the glTF file carries it.
"""
import math
import os
import sys

import bmesh
import bpy
import numpy
from mathutils import Vector

TEXTURE_SIZE = 512


def arguments():
    """The arguments after `--`: the output folder first."""
    return sys.argv[sys.argv.index("--") + 1:]


def clear_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)


# ------------------------------------------------------------------ textures (rows from the bottom, values 0 .. 1)

def noise(size=TEXTURE_SIZE, roughness=2.0, seed=0, stretch=(1.0, 1.0)):
    """Tileable fractal noise 0 .. 1: white noise filtered by 1 / f^(roughness / 2); `stretch` > 1 across an axis
    makes the features long along the other one (fibres)."""
    rng = numpy.random.default_rng(seed)
    spectrum = numpy.fft.fft2(rng.standard_normal((size, size)))
    fy = numpy.fft.fftfreq(size)[:, None] * stretch[1]
    fx = numpy.fft.fftfreq(size)[None, :] * stretch[0]
    radius = numpy.sqrt(fx * fx + fy * fy)
    radius[0, 0] = 1
    spectrum /= radius ** (roughness / 2)
    spectrum[0, 0] = 0
    field = numpy.real(numpy.fft.ifft2(spectrum))
    return (field - field.min()) / (field.max() - field.min())


def spots(size, count, seed):
    """Tileable distance (in texture widths) to the nearest of `count` random points, and that point's index."""
    rng = numpy.random.default_rng(seed)
    points = rng.random((count, 2))
    grid = (numpy.arange(size) + 0.5) / size
    best = numpy.full((size, size), 9.0)
    nearest = numpy.zeros((size, size), dtype=numpy.int32)
    for index, (px, py) in enumerate(points):
        dx = numpy.abs(grid[None, :] - px)
        dy = numpy.abs(grid[:, None] - py)
        dist = numpy.sqrt(numpy.minimum(dx, 1 - dx) ** 2 + numpy.minimum(dy, 1 - dy) ** 2)
        closer = dist < best
        best = numpy.where(closer, dist, best)
        nearest = numpy.where(closer, index, nearest)
    return best, nearest


def colour_ramp(value, low, high):
    """Colours (sRGB) between `low` and `high` by `value` 0 .. 1."""
    low, high = numpy.array(low, dtype=numpy.float32), numpy.array(high, dtype=numpy.float32)
    return low + (high - low) * value[..., None]


def normals_from_height(height, strength):
    """A tangent-space normal map (OpenGL, as glTF wants) from a tileable height field."""
    dx = (numpy.roll(height, -1, axis=1) - numpy.roll(height, 1, axis=1)) * strength
    dy = (numpy.roll(height, -1, axis=0) - numpy.roll(height, 1, axis=0)) * strength
    normal = numpy.dstack((-dx, -dy, numpy.ones_like(height)))
    normal /= numpy.linalg.norm(normal, axis=2, keepdims=True)
    return normal * 0.5 + 0.5


def save_image(folder, name, pixels, colour=True):
    """`pixels` (rows x columns x 3 or 4, 0 .. 1) as <folder>/<name>.png, loaded as an image of the scene."""
    height, width = pixels.shape[:2]
    if pixels.shape[2] == 3:
        pixels = numpy.dstack((pixels, numpy.ones((height, width))))
    image = bpy.data.images.new(name, width, height, alpha=True)
    image.pixels[:] = numpy.clip(pixels, 0, 1).astype(numpy.float32).ravel()
    image.filepath_raw = os.path.join(folder, name + ".png")
    image.file_format = "PNG"
    image.save()
    image.colorspace_settings.name = "sRGB" if colour else "Non-Color"
    return image


def load_image(path, colour=True, size=None, flip_green=False):
    """An image file (e.g. a texture set of Assets.json); `flip_green` turns a DirectX normal map into OpenGL's."""
    image = bpy.data.images.load(path)
    image.colorspace_settings.name = "sRGB" if colour else "Non-Color"
    if size and image.size[0] > size:
        image.scale(size, size)
    if flip_green:
        width, height = image.size
        pixels = numpy.array(image.pixels[:], dtype=numpy.float32).reshape(height, width, 4)
        pixels[..., 1] = 1 - pixels[..., 1]
        image.pixels[:] = pixels.ravel()
        image.update()
    return image


def poly_haven_material(name, folder, size=1024):
    """A material of a Poly Haven texture set of Assets.json in `folder` (<id>_diff_2k.jpg, _nor_dx_2k.jpg,
    _arm_2k.jpg), its maps scaled down to `size`."""
    asset = os.path.basename(os.path.normpath(folder))
    colour = load_image(os.path.join(folder, f"{asset}_diff_2k.jpg"), size=size)
    normal = load_image(os.path.join(folder, f"{asset}_nor_dx_2k.jpg"), colour=False, size=size, flip_green=True)
    arm = load_image(os.path.join(folder, f"{asset}_arm_2k.jpg"), colour=False, size=size)
    return material(name, colour, normal, roughness=arm)


# ------------------------------------------------------------------ materials

def material(name, colour, normal=None, roughness=0.7, alpha_cutoff=None, double_sided=False, tint=None):
    """A PBR material: `colour` an image or an sRGB triple, `normal` an image or none, `roughness` a number or an image
    (its green channel, an ARM map), `alpha_cutoff` cuts out where the colour image's alpha is lower (glTF MASK),
    `tint` multiplies the colour image (glTF baseColorFactor, which the game may change)."""
    made = bpy.data.materials.new(name)
    if made.node_tree is None:
        made.use_nodes = True
    made.use_backface_culling = not double_sided
    nodes, links = made.node_tree.nodes, made.node_tree.links
    shader = next(node for node in nodes if node.type == "BSDF_PRINCIPLED")
    if isinstance(colour, bpy.types.Image):
        texture = nodes.new("ShaderNodeTexImage")
        texture.image = colour
        out = texture.outputs["Color"]
        if tint:
            mix = nodes.new("ShaderNodeMix")
            mix.data_type, mix.blend_type = "RGBA", "MULTIPLY"
            mix.inputs["Factor"].default_value = 1
            rgba = [socket for socket in mix.inputs if socket.type == "RGBA"]
            links.new(out, rgba[0])
            rgba[1].default_value = (*srgb_to_linear(tint), 1)
            out = next(socket for socket in mix.outputs if socket.type == "RGBA")
        links.new(out, shader.inputs["Base Color"])
        if alpha_cutoff is not None:
            clip = nodes.new("ShaderNodeMath")
            clip.operation = "GREATER_THAN"
            clip.inputs[1].default_value = alpha_cutoff
            links.new(texture.outputs["Alpha"], clip.inputs[0])
            links.new(clip.outputs[0], shader.inputs["Alpha"])
    else:
        shader.inputs["Base Color"].default_value = (*srgb_to_linear(colour), 1)
    if isinstance(roughness, bpy.types.Image):
        texture = nodes.new("ShaderNodeTexImage")
        texture.image = roughness
        separate = nodes.new("ShaderNodeSeparateColor")
        links.new(texture.outputs["Color"], separate.inputs["Color"])
        links.new(separate.outputs["Green"], shader.inputs["Roughness"])
    else:
        shader.inputs["Roughness"].default_value = roughness
    if normal is not None:
        texture = nodes.new("ShaderNodeTexImage")
        texture.image = normal
        normal_map = nodes.new("ShaderNodeNormalMap")
        links.new(texture.outputs["Color"], normal_map.inputs["Color"])
        links.new(normal_map.outputs["Normal"], shader.inputs["Normal"])
    return made


def srgb_to_linear(colour):
    return tuple(c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4 for c in colour)


# ------------------------------------------------------------------ shapes

def new_object(name, mesh):
    made = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(made)
    return made


def tube(bm, points, radii, sides=10, uv_length=1.0, material_index=0, twist=0.0):
    """A tube along `points` (Vectors) with a radius at each, into bmesh `bm`: U around it, V along it (1 per
    `uv_length` metres). `twist` turns the ring by that many turns per metre (rope)."""
    uv = bm.loops.layers.uv.verify()
    rings, along = [], 0.0
    for index, point in enumerate(points):
        ahead = points[min(index + 1, len(points) - 1)] - points[max(index - 1, 0)]
        axis = ahead.normalized()
        side = axis.orthogonal().normalized()
        up = axis.cross(side)
        if index > 0:
            along += (point - points[index - 1]).length
        ring = []
        for step in range(sides):
            angle = 2 * math.pi * (step / sides + twist * along)
            offset = (side * math.cos(angle) + up * math.sin(angle)) * radii[index]
            ring.append(bm.verts.new(point + offset))
        rings.append((ring, along / uv_length))
    for (ring, v0), (next_ring, v1) in zip(rings, rings[1:]):
        for step in range(sides):
            quad = (ring[step], ring[(step + 1) % sides], next_ring[(step + 1) % sides], next_ring[step])
            face = bm.faces.new(quad)
            face.material_index = material_index
            face.smooth = True
            for loop, (u, v) in zip(face.loops, ((step, v0), (step + 1, v0), (step + 1, v1), (step, v1))):
                loop[uv].uv = (u / sides, v)
    for ring in (list(reversed(rings[0][0])), rings[-1][0]):   # the ends closed
        face = bm.faces.new(ring)
        face.material_index = material_index
        for loop in face.loops:
            loop[uv].uv = (0.5, 0.5)


def blob(bm, centre, radii, seed=0, bumps=0.0, segments=16, rings=10, material_index=0):
    """A lumpy ellipsoid into bmesh `bm` (a stone, a knot): `bumps` is how far the noise moves its surface (0 .. 1
    of its radius). UVs from its angles."""
    rng = numpy.random.default_rng(seed)
    waves = rng.normal(size=(6, 3))
    phases = rng.random(6) * 6.28
    uv = bm.loops.layers.uv.verify()
    grid = []
    for ring in range(rings + 1):
        theta = math.pi * ring / rings
        row = []
        for segment in range(segments):
            phi = 2 * math.pi * segment / segments
            direction = Vector((math.sin(theta) * math.cos(phi), math.sin(theta) * math.sin(phi), math.cos(theta)))
            lump = 1 + bumps * sum(math.sin(Vector(w).dot(direction) * 2 + p) for w, p in zip(waves, phases)) / 6
            row.append(bm.verts.new(centre + Vector(r * d * lump for r, d in zip(radii, direction))))
        grid.append(row)
    for ring in range(rings):
        for segment in range(segments):
            nxt = (segment + 1) % segments
            quad = (grid[ring][segment], grid[ring + 1][segment], grid[ring + 1][nxt], grid[ring][nxt])
            if len(set(quad)) < 3:
                continue
            face = bm.faces.new(quad)
            face.material_index = material_index
            face.smooth = True
            coords = ((segment, ring), (segment, ring + 1), (segment + 1, ring + 1), (segment + 1, ring))
            for loop, (u, v) in zip(face.loops, coords):
                loop[uv].uv = (u / segments, 1 - v / rings)
    bmesh.ops.remove_doubles(bm, verts=[v for row in grid for v in row], dist=1e-6)


def mesh_object(name, bm, materials):
    """An object of the bmesh with `materials` in its slots (the faces' material_index)."""
    mesh = bpy.data.meshes.new(name)
    bm.normal_update()
    bm.to_mesh(mesh)
    bm.free()
    for made in materials:
        mesh.materials.append(made)
    return new_object(name, mesh)


def export(path, objects):
    """`objects` (and what their armatures animate: every action) as a binary glTF."""
    bpy.ops.object.select_all(action="DESELECT")
    for each in objects:
        each.select_set(True)
    bpy.ops.export_scene.gltf(filepath=path, export_format="GLB", use_selection=True, export_apply=True,
                              export_animation_mode="ACTIONS", export_yup=True)
    print("made", os.path.basename(path), [each.name for each in objects])
