"""Textures baked with Cycles from a procedural material onto a mesh's UVs: what the scripts that refine a model (the
triceratops' skin, the pterodactyl's hide and wings, the tree's bark) give the glTF exporter as plain images. A shader
graph (`graph(nodes, links)`) returns three sockets: a colour (linear, as the shader nodes are: an sRGB triple as
kit.srgb_to_linear makes it), a roughness and a height; the bake writes <name>_color.png, <name>_normal.png (tangent
space, OpenGL as glTF wants: the bump of the height on the mesh's own normals) and <name>_roughness.png (grey: the
green channel kit.material reads).
"""
import os

import bpy

SAMPLES = 4
MARGIN = 16


def _image(name, size, colour):
    image = bpy.data.images.new(name, size, size, alpha=False)
    image.colorspace_settings.name = "sRGB" if colour else "Non-Color"
    return image


def bake(figure, folder, name, size, graph, bump_distance=0.004):
    """Bakes `graph` on `figure` (its first UV map, every slot) to the three textures in `folder` (each size x size);
    returns (colour, normal, roughness) images. The figure's materials are restored after."""
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = SAMPLES
    scene.render.bake.margin = MARGIN
    kept = [slot.material for slot in figure.material_slots]
    material = bpy.data.materials.new(name + "_bake")
    material.use_nodes = True
    nodes, links = material.node_tree.nodes, material.node_tree.links
    nodes.clear()
    output = nodes.new("ShaderNodeOutputMaterial")
    colour, roughness, height = graph(nodes, links)
    target = nodes.new("ShaderNodeTexImage")
    nodes.active = target
    if not figure.material_slots:
        figure.data.materials.append(material)
    for slot in figure.material_slots:
        slot.material = material
    bpy.ops.object.select_all(action="DESELECT")
    figure.select_set(True)
    bpy.context.view_layer.objects.active = figure
    made = []
    passes = (("color", colour, "EMIT"), ("roughness", roughness, "EMIT"), ("normal", None, "NORMAL"))
    for suffix, socket, kind in passes:
        image = _image(f"{name}_{suffix}", size, suffix == "color")
        target.image = image
        if kind == "EMIT":
            emission = nodes.new("ShaderNodeEmission")
            links.new(socket, emission.inputs["Color"])
            links.new(emission.outputs["Emission"], output.inputs["Surface"])
            bpy.ops.object.bake(type="EMIT")
        else:
            shader = nodes.new("ShaderNodeBsdfDiffuse")
            bump = nodes.new("ShaderNodeBump")
            bump.inputs["Distance"].default_value = bump_distance
            links.new(height, bump.inputs["Height"])
            links.new(bump.outputs["Normal"], shader.inputs["Normal"])
            links.new(shader.outputs["BSDF"], output.inputs["Surface"])
            bpy.ops.object.bake(type="NORMAL", normal_space="TANGENT")
        image.filepath_raw = os.path.join(folder, image.name + ".png")
        image.file_format = "PNG"
        image.save()
        made.append(image)
    for slot, before in zip(figure.material_slots, kept):
        slot.material = before
    bpy.data.materials.remove(material)
    colour_image, roughness_image, normal_image = made
    return colour_image, normal_image, roughness_image


def node(nodes, kind, **inputs):
    """A new node of `kind` with some inputs set (by name or index)."""
    made = nodes.new(kind)
    for key, value in inputs.items():
        made.inputs[key.replace("_", " ")].default_value = value
    return made


def math(nodes, links, operation, a, b=None, clamp=False):
    """A math node: `a` and `b` sockets or numbers."""
    made = nodes.new("ShaderNodeMath")
    made.operation = operation
    made.use_clamp = clamp
    for index, value in enumerate((a, b)):
        if value is None:
            continue
        if isinstance(value, bpy.types.NodeSocket):
            links.new(value, made.inputs[index])
        else:
            made.inputs[index].default_value = value
    return made.outputs[0]
