"""The palms of Nobiax's palm pack (OpenGameArt "Free Palm Treez v3", CC0) as glTF files the engine imports.

The pack has the meshes as OBJ and one texture for all (diffuse.tga: leaves and bark, the leaves cut out by its
alpha; normal.tga). This makes palm_straight.glb and palm_bend.glb next to them: the mesh in metres, one material
with the colour (the leaves made livelier), the cut-out (an alpha mask, glTF alphaMode MASK) and the normal map,
seen from both sides.
fetch-assets.ps1 runs it on the asset's folder:

    blender -b --factory-startup --python-exit-code 1 --python palm.py -- <folder>
"""
import os
import sys

import bpy
import numpy

FOLDER = sys.argv[sys.argv.index("--") + 1]
PALMS = ("palm_straight", "palm_bend")
SCALE = 0.03  # the pack's units are about 3 cm: the straight palm is 6.4 m tall, the bent one 5.4 m
CUTOFF = 0.3  # leaf pixels with less alpha are cut out
LEAF_BRIGHTNESS, LEAF_SATURATION = 1.6, 1.3  # the pack's leaves are dark olive: livelier in the diorama's evening
ROUGHNESS = 0.7


def image(name, colour_space):
    loaded = bpy.data.images.load(os.path.join(FOLDER, name))
    loaded.colorspace_settings.name = colour_space
    return loaded


def livelier_leaves(colour):
    """The right half of the texture (the leaf) brighter and more saturated; the bark (left half) as it is."""
    width, height = colour.size
    pixels = numpy.array(colour.pixels[:], dtype=numpy.float32).reshape(height, width, 4)
    leaf = pixels[:, width // 2:, :3]
    grey = leaf.mean(axis=2, keepdims=True)
    pixels[:, width // 2:, :3] = numpy.clip((grey + (leaf - grey) * LEAF_SATURATION) * LEAF_BRIGHTNESS, 0, 1)
    colour.pixels[:] = pixels.ravel()
    colour.update()


def material():
    """The pack's texture: colour, alpha mask (alpha > CUTOFF, which the glTF exporter writes as MASK), normals."""
    made = bpy.data.materials.new("palm")
    if made.node_tree is None:
        made.use_nodes = True
    made.use_backface_culling = False  # leaves are cards: glTF doubleSided
    nodes, links = made.node_tree.nodes, made.node_tree.links
    shader = next(node for node in nodes if node.type == "BSDF_PRINCIPLED")
    shader.inputs["Roughness"].default_value = ROUGHNESS

    colour = nodes.new("ShaderNodeTexImage")
    colour.image = image("diffuse.tga", "sRGB")
    livelier_leaves(colour.image)
    links.new(colour.outputs["Color"], shader.inputs["Base Color"])
    clip = nodes.new("ShaderNodeMath")
    clip.operation = "GREATER_THAN"
    clip.inputs[1].default_value = CUTOFF
    links.new(colour.outputs["Alpha"], clip.inputs[0])
    links.new(clip.outputs[0], shader.inputs["Alpha"])

    normals = nodes.new("ShaderNodeTexImage")
    normals.image = image("normal.tga", "Non-Color")
    normal_map = nodes.new("ShaderNodeNormalMap")
    links.new(normals.outputs["Color"], normal_map.inputs["Color"])
    links.new(normal_map.outputs["Normal"], shader.inputs["Normal"])
    return made


def convert(name):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.wm.obj_import(filepath=os.path.join(FOLDER, name + ".obj"))
    palm = bpy.context.selected_objects[0]
    palm.name = palm.data.name = name
    bpy.context.view_layer.objects.active = palm
    palm.scale = (SCALE, SCALE, SCALE)
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    palm.data.materials.clear()
    palm.data.materials.append(material())
    bpy.ops.export_scene.gltf(filepath=os.path.join(FOLDER, name + ".glb"), export_format="GLB",
                              use_selection=True, export_apply=True)
    print("made", name + ".glb", "size", tuple(round(d, 2) for d in palm.dimensions))


for palm_name in PALMS:
    convert(palm_name)
