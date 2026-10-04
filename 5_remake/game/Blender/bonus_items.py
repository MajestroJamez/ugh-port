"""The bonus items of the remake as bonus_items.glb: a mesh per kind of bonus item, named as the game data names the
kind (the sprites of the original: nine fruits for energy, a stone tablet with an X for the multiplier).

energy1 an apple, energy2 a slice of melon, energy3 a strawberry, energy4 half a kiwi, energy5 a coconut, energy6 a
pear, energy7 half a coconut, energy8 cherries, energy9 a banana, multiplier a stone tablet with a green X carved in
it (the texture set rock_face_03). Each about 1 m (the sprites are 16 px), facing -Y, its origin at its bottom middle
(where it lies).

    blender -b --factory-startup --python-exit-code 1 --python bonus_items.py -- <folder> <rock_face_03 folder>
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import bmesh  # noqa: E402
import numpy  # noqa: E402
from mathutils import Vector  # noqa: E402

import ugh_kit as kit  # noqa: E402

FOLDER, STONE = kit.arguments()[:2]


def textured(name, low, high, roughness, seed, speckles=0.0, bumps=2.0):
    """A material of mottled `low` .. `high` (sRGB), dark `speckles` (seeds, pits) as many as that part."""
    size = kit.TEXTURE_SIZE
    mottle = kit.noise(size, 2.2, seed=seed)
    distance, _ = kit.spots(size, 400, seed + 1)
    dots = (distance < 0.012).astype(numpy.float32) * speckles
    colour = kit.colour_ramp(mottle, low, high) * (1 - 0.6 * dots[..., None])
    return kit.material(name, kit.save_image(FOLDER, "bonus_" + name, colour),
                        kit.save_image(FOLDER, "bonus_" + name + "_normal",
                                       kit.normals_from_height(mottle * 0.3 - dots, bumps), colour=False),
                        roughness=roughness)


def materials():
    made = {
        "apple": textured("apple", (0.42, 0.68, 0.16), (0.62, 0.85, 0.3), 0.35, 101, 0.3),
        "melon_rind": textured("melon_rind", (0.12, 0.38, 0.1), (0.3, 0.58, 0.2), 0.4, 103),
        "melon": textured("melon", (0.88, 0.2, 0.22), (0.98, 0.42, 0.4), 0.45, 105),
        "strawberry": textured("strawberry", (0.75, 0.06, 0.08), (0.95, 0.2, 0.18), 0.35, 107, 1.0, 4),
        "kiwi_skin": textured("kiwi_skin", (0.4, 0.28, 0.14), (0.58, 0.42, 0.22), 0.9, 109, 0.0, 6),
        "kiwi": textured("kiwi", (0.35, 0.62, 0.12), (0.62, 0.82, 0.3), 0.4, 111, 1.0),
        "coconut": textured("coconut", (0.32, 0.2, 0.1), (0.5, 0.34, 0.18), 0.9, 113, 0.0, 8),
        "coconut_flesh": textured("coconut_flesh", (0.92, 0.9, 0.85), (1.0, 0.98, 0.95), 0.6, 115),
        "pear": textured("pear", (0.55, 0.7, 0.2), (0.75, 0.82, 0.32), 0.45, 117, 0.3),
        "cherry": textured("cherry", (0.45, 0.03, 0.06), (0.7, 0.08, 0.1), 0.2, 119),
        "banana": textured("banana", (0.9, 0.75, 0.18), (1.0, 0.88, 0.35), 0.45, 121),
        "leaf": kit.material("leaf", (0.24, 0.52, 0.14), roughness=0.5, double_sided=True),
        "stem": kit.material("stem", (0.35, 0.25, 0.12), roughness=0.8),
        "seed": kit.material("seed", (0.06, 0.04, 0.03), roughness=0.3),
        "hollow": kit.material("hollow", (0.6, 0.58, 0.55), roughness=0.15),
        "stone": kit.poly_haven_material("stone", STONE),
        "mark": kit.material("mark", (0.35, 0.75, 0.2), roughness=0.6),
    }
    return made


class Item:
    """A mesh being built: a bmesh and the materials of its slots."""

    def __init__(self, made):
        self.bm, self.made, self.slots = bmesh.new(), made, []

    def slot(self, name):
        if name not in self.slots:
            self.slots.append(name)
        return self.slots.index(name)

    def blob(self, material, centre, radii, bumps=0.0, seed=0):
        kit.blob(self.bm, Vector(centre), radii, seed=seed, bumps=bumps, segments=24, rings=14,
                 material_index=self.slot(material))

    def tube(self, material, points, radii):
        kit.tube(self.bm, [Vector(p) for p in points], radii, sides=8, material_index=self.slot(material))

    def leaf(self, at, along, width):
        at, along = Vector(at), Vector(along)
        side = along.cross(Vector((0, -1, 0))).normalized() * width
        corners = (at, at + along * 0.5 + side, at + along, at + along * 0.5 - side)
        face = self.bm.faces.new([self.bm.verts.new(c) for c in corners])
        face.material_index = self.slot("leaf")

    def object(self, name):
        return kit.mesh_object(name, self.bm, [self.made[slot] for slot in self.slots])


def cut(item, normal, offset, material):
    """Cuts `item` along the plane through `offset` with `normal`, keeps the side against it and caps the cut in
    `material`."""
    bm = item.bm
    geometry = bm.verts[:] + bm.edges[:] + bm.faces[:]
    result = bmesh.ops.bisect_plane(bm, geom=geometry, plane_co=offset, plane_no=normal, clear_outer=True)
    edges = [e for e in result["geom_cut"] if isinstance(e, bmesh.types.BMEdge)]
    filled = bmesh.ops.holes_fill(bm, edges=edges)
    for face in filled["faces"]:
        face.material_index = item.slot(material)
        face.smooth = False


def apple(made):
    item = Item(made)
    item.blob("apple", (0, 0, 0.45), (0.48, 0.46, 0.44), bumps=0.06, seed=1)
    item.tube("stem", [(0, 0, 0.82), (0.03, 0, 0.98), (0.08, 0, 1.06)], [0.035, 0.03, 0.025])
    item.leaf((0.06, 0, 0.94), (0.3, -0.05, 0.12), 0.09)
    return item.object("energy1")


def melon(made):
    """A half disc of flesh, its straight edge up, the rind along its round edge, black seeds on its face."""
    item = Item(made)
    item.blob("melon", (0, 0, 0.6), (0.56, 0.12, 0.56))
    cut(item, Vector((0, 0, 1)), Vector((0, 0, 0.6)), "melon")
    item.tube("melon_rind", [(0.6 * math.cos(a), 0, 0.6 - 0.6 * math.sin(a)) for a in numpy.linspace(0, math.pi, 16)],
              [0.07] * 16)
    for x in (-0.3, -0.12, 0.08, 0.26):
        item.blob("seed", (x, -0.11, 0.3 + 0.05 * math.cos(x * 6)), (0.025, 0.012, 0.04))
    return item.object("energy2")


def strawberry(made):
    item = Item(made)
    item.blob("strawberry", (0, 0, 0.48), (0.38, 0.36, 0.42), bumps=0.08, seed=3)
    for vertex in item.bm.verts:   # narrower towards the bottom: a heart's tip
        squeeze = 0.55 + 0.45 * min(1.0, vertex.co.z / 0.6)
        vertex.co.x *= squeeze
        vertex.co.y *= squeeze
    for index in range(6):
        angle = 2 * math.pi * index / 6
        item.leaf((0, 0, 0.88), (0.3 * math.cos(angle), 0.3 * math.sin(angle), 0.05), 0.08)
    item.tube("stem", [(0, 0, 0.86), (0, 0, 1.0)], [0.03, 0.025])
    return item.object("energy3")


def kiwi(made):
    item = Item(made)
    item.blob("kiwi_skin", (0, 0.1, 0.42), (0.38, 0.5, 0.4), bumps=0.04, seed=4)
    cut(item, Vector((0, -1, 0)), Vector((0, 0.0, 0)), "kiwi")
    for index in range(14):   # seeds in a ring around the pale middle
        angle = 2 * math.pi * index / 14
        item.blob("seed", (0.17 * math.cos(angle), -0.005, 0.42 + 0.17 * math.sin(angle)), (0.02, 0.006, 0.03))
    item.blob("coconut_flesh", (0, -0.002, 0.42), (0.08, 0.006, 0.08))
    return item.object("energy4")


def coconut(made):
    item = Item(made)
    item.blob("coconut", (0, 0, 0.46), (0.46, 0.44, 0.47), bumps=0.08, seed=5)
    for x, z in ((-0.1, 0.62), (0.1, 0.62), (0, 0.48)):
        item.blob("seed", (x, -0.42, z), (0.05, 0.03, 0.05))
    return item.object("energy5")


def pear(made):
    item = Item(made)
    item.blob("pear", (0, 0, 0.36), (0.4, 0.38, 0.36), bumps=0.04, seed=6)
    item.blob("pear", (0, 0, 0.72), (0.24, 0.23, 0.3), bumps=0.04, seed=7)
    item.tube("stem", [(0, 0, 0.98), (0.04, 0, 1.12), (0.1, 0, 1.18)], [0.035, 0.03, 0.025])
    return item.object("energy6")


def half_coconut(made):
    item = Item(made)
    item.blob("coconut", (0, 0, 0.46), (0.46, 0.44, 0.47), bumps=0.08, seed=8)
    cut(item, Vector((0.25, -1, 0.35)).normalized(), Vector((0, -0.05, 0.46)), "coconut_flesh")
    item.blob("hollow", (0, -0.08, 0.46), (0.15, 0.1, 0.15))   # the hollow inside the flesh
    return item.object("energy7")


def cherries(made):
    item = Item(made)
    for x in (-0.24, 0.24):
        item.blob("cherry", (x, 0, 0.26), (0.26, 0.25, 0.25), seed=9)
        item.tube("stem", [(x, 0, 0.48), (x * 0.6, 0, 0.75), (0.02, 0, 1.0)], [0.025, 0.022, 0.02])
    item.leaf((0.02, 0, 1.0), (0.3, -0.03, 0.1), 0.08)
    return item.object("energy8")


def banana(made):
    item = Item(made)
    points = [(0.62 * math.sin(a), 0, 0.14 + 0.62 * (1 - math.cos(a))) for a in numpy.linspace(-0.9, 0.9, 9)]
    radii = [0.07 + 0.12 * math.sin(math.pi * s) for s in numpy.linspace(0, 1, 9)]
    item.tube("banana", points, radii)
    item.tube("stem", [points[-1], (0.6, 0, 0.72)], [0.05, 0.04])
    return item.object("energy9")


def tablet(made):
    """A rounded stone slab standing on edge, a green X carved in its face."""
    item = Item(made)
    item.blob("stone", (0, 0, 0.5), (0.5, 0.14, 0.48), bumps=0.05, seed=10)
    for vertex in item.bm.verts:   # flatter: a slab
        vertex.co.x = math.copysign(min(abs(vertex.co.x) * 1.25, 0.5), vertex.co.x)
        vertex.co.z = 0.5 + math.copysign(min(abs(vertex.co.z - 0.5) * 1.25, 0.48), vertex.co.z - 0.5)
    for sign in (1, -1):
        item.tube("mark", [(-0.28, -0.14, 0.5 - sign * 0.28), (0.28, -0.14, 0.5 + sign * 0.28)], [0.06, 0.06])
    return item.object("multiplier")


def build():
    kit.clear_scene()
    made = materials()
    items = [make(made) for make in (apple, melon, strawberry, kiwi, coconut, pear, half_coconut, cherries, banana,
                                     tablet)]
    kit.export(os.path.join(FOLDER, "bonus_items.glb"), items)


build()
