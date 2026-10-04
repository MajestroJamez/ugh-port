"""The copter of the remake: a stone age pedal copter as copter.glb, its parts as meshes of their own, each with its
origin where the game turns it (copter_layout):

- Body_1, Body_2 (the players' copters, leather dyed in their colours): a cage of bamboo poles lashed with rope on a
  floor of logs, wicker at the back and the sides, two stumps with leather cushions as seats, the handlebar on a
  post, leather banners at the top and the bottom of the front, tusks on the front corners. Origin: the bottom
  middle of the body.
- Rotor_1, Rotor_2: a stone hub with bone blades, a big leaf lashed on each (the second player's rotor bushier, as in
  the original). Origin: the hub, it turns about Z.
- Crank: the axle with bone arms and stone pedals the pilot pedals. Origin: the middle of the axle, it turns about X;
  the left pedal (+X) on top.
- Sling: the rope that holds a hanging stone passenger below the body. Origin: the bottom middle of the body.

    blender -b --factory-startup --python-exit-code 1 --python copter.py -- <folder> <palm_bark> <rock_face_03>

(the last two: the folders of those texture sets of Assets.json)
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import bmesh  # noqa: E402
from mathutils import Vector  # noqa: E402

import copter_layout as layout  # noqa: E402
import copter_materials  # noqa: E402
import ugh_kit as kit  # noqa: E402

FOLDER, WOOD, STONE = kit.arguments()[:3]
HALF_X, HALF_Y = layout.BODY_WIDTH / 2 - 0.1, layout.BODY_DEPTH - 0.08   # the corner posts
FLOOR, TOP, RAIL = 0.12, 1.72, 0.92
POLE, ROPE = 0.055, 0.009


class Part:
    """A mesh being built with the materials it uses."""

    def __init__(self, name):
        self.name, self.materials, self.bm = name, [], bmesh.new()

    def slot(self, material):
        if material not in self.materials:
            self.materials.append(material)
        return self.materials.index(material)

    def tube(self, points, radii, material, sides=10, uv_length=None, twist=0.0):
        points = [Vector(p) for p in points]
        radii = radii if isinstance(radii, (list, tuple)) else [radii] * len(points)
        kit.tube(self.bm, points, radii, sides, uv_length or 2 * math.pi * max(radii),
                 material_index=self.slot(material), twist=twist)

    def blob(self, centre, radii, material, seed=0, bumps=0.0):
        kit.blob(self.bm, Vector(centre), radii, seed=seed, bumps=bumps, material_index=self.slot(material))

    def lash(self, centre, axis, radius, turns=3):
        """Rope wound round a pole of `radius` at `centre` along `axis`."""
        axis = Vector(axis).normalized()
        side = axis.orthogonal().normalized()
        up = axis.cross(side)
        steps = turns * 10
        points = [Vector(centre) + axis * (0.05 * (i / steps - 0.5)) +
                  (side * math.cos(2 * math.pi * i / 10) + up * math.sin(2 * math.pi * i / 10)) * (radius + ROPE * 0.6)
                  for i in range(steps + 1)]
        self.tube(points, ROPE, "rope", sides=6, uv_length=0.05)

    def quad(self, corners, uvs, material):
        uv = self.bm.loops.layers.uv.verify()
        face = self.bm.faces.new([self.bm.verts.new(c) for c in corners])
        face.material_index = self.slot(material)
        for loop, coord in zip(face.loops, uvs):
            loop[uv].uv = coord

    def done(self, materials):
        return kit.mesh_object(self.name, self.bm, [materials[name] for name in self.materials])


def wicker(part, a, b, height_from, height_to):
    """A wicker wall between corners a and b (x, y) from one height to another."""
    a, b = Vector((*a, 0)), Vector((*b, 0))
    width = (b - a).length
    corners = [a + Vector((0, 0, height_from)), b + Vector((0, 0, height_from)), b + Vector((0, 0, height_to)),
               a + Vector((0, 0, height_to))]
    scale = 0.6   # metres per repeat of the texture
    u, v0, v1 = width / scale, height_from / scale, height_to / scale
    part.quad(corners, [(0, v0), (u, v0), (u, v1), (0, v1)], "wicker")


def body(player):
    part = Part(f"Body_{player}")
    leather = f"leather_{player}"
    corners = [(x, y) for x in (-HALF_X, HALF_X) for y in (-HALF_Y, HALF_Y)]
    for x, y in corners:
        part.tube([(x, y, 0.02), (x, y, TOP + 0.08)], POLE, "bamboo", uv_length=0.36)
        for low, high in ((TOP - 0.12, TOP - 0.02), (FLOOR, FLOOR + 0.12)):
            part.tube([(x, y, low), (x, y, high)], POLE + 0.006, leather, sides=12)
        part.lash((x, y, RAIL), (0, 0, 1), POLE)
    for log in range(5):
        y = -HALF_Y + 2 * HALF_Y * log / 4
        part.tube([(-HALF_X - 0.08, y, FLOOR - 0.04), (HALF_X + 0.08, y, FLOOR - 0.04)], 0.07, "wood")
    for y in (-HALF_Y, HALF_Y):   # beams along X at the top, a rail at the back
        part.tube([(-HALF_X - 0.08, y, TOP), (HALF_X + 0.08, y, TOP)], POLE * 0.9, "bamboo", uv_length=0.36)
    part.tube([(-HALF_X, HALF_Y, RAIL), (HALF_X, HALF_Y, RAIL)], POLE * 0.8, "bamboo", uv_length=0.36)
    for x in (-HALF_X, HALF_X):   # beams along Y at the top and the rails at the sides
        part.tube([(x, -HALF_Y - 0.08, TOP + 0.05), (x, HALF_Y + 0.08, TOP + 0.05)], POLE * 0.9, "bamboo",
                  uv_length=0.36)
        part.tube([(x, -HALF_Y, RAIL), (x, HALF_Y, RAIL)], POLE * 0.8, "bamboo", uv_length=0.36)
        wicker(part, (x, -HALF_Y), (x, HALF_Y), FLOOR, RAIL)
    wicker(part, (-HALF_X, HALF_Y), (HALF_X, HALF_Y), FLOOR, TOP)
    # the mast's cross beams and the mast up to the rotor's hub
    part.tube([(-HALF_X, 0, TOP + 0.05), (HALF_X, 0, TOP + 0.05)], POLE * 0.8, "bamboo", uv_length=0.36)
    part.tube([(0, 0, TOP + 0.04), (0, 0, layout.ROTOR_HUB[2] - 0.05)], 0.05, "wood")
    part.lash((0, 0, TOP + 0.05), (0, 0, 1), 0.05)
    seats(part, leather)
    banner(part, leather, TOP - 0.02, TOP - 0.2)   # under the top front beam
    banner(part, leather, FLOOR + 0.04, FLOOR - 0.06)   # over the front log
    for side in (-1, 1):   # tusks on the front corners
        points = [(side * (HALF_X + 0.02 * t), -HALF_Y - 0.04 * t, TOP + 0.05 + 0.2 * t + 0.02 * math.sin(3 * t))
                  for t in (i / 6 for i in range(7))]
        part.tube(points, [0.04 - 0.032 * i / 6 for i in range(7)], "bone", sides=8)
    return part


def seats(part, leather):
    for (x, y, z), radius in ((layout.PILOT_SEAT, 0.16), (layout.PASSENGER_SEAT, 0.15)):
        part.tube([(x, y, FLOOR), (x, y, z - 0.05)], radius, "wood", sides=14)
        part.blob((x, y, z - 0.03), (radius + 0.01, radius, 0.04), leather)
    seat = Vector(layout.PILOT_SEAT)
    axle = seat + Vector(layout.PEDAL_AXLE)
    grip = seat + Vector(layout.GRIP)
    left, right = Vector((grip.x, grip.y, grip.z)), Vector((seat.x - layout.GRIP[0], grip.y, grip.z))
    part.tube([(axle.x, axle.y, FLOOR), (axle.x, axle.y, grip.z)], 0.035, "bamboo", uv_length=0.22)
    part.tube([right + Vector((-0.07, 0, 0)), left + Vector((0.07, 0, 0))], [0.026, 0.026], "bone", sides=8)
    for end in (right + Vector((-0.08, 0, 0)), left + Vector((0.08, 0, 0))):
        part.blob(end, (0.035, 0.035, 0.035), "bone")
    part.lash((axle.x, axle.y, grip.z), (1, 0, 0), 0.026)
    part.lash((axle.x, axle.y, axle.z), (0, 0, 1), 0.035)


def banner(part, leather, top, bottom):
    """A leather banner in front of the cage from `top` down to `bottom`, its lower edge jagged."""
    columns = 16
    uv = part.bm.loops.layers.uv.verify()
    upper, lower = [], []
    for i in range(columns + 1):
        x = -HALF_X + 0.08 + (2 * HALF_X - 0.16) * i / columns
        sag = 0.03 * math.sin(math.pi * i / columns)
        upper.append(part.bm.verts.new((x, -HALF_Y - 0.075, top)))
        lower.append(part.bm.verts.new((x, -HALF_Y - 0.08, bottom - sag - (0.04 if i % 2 else 0))))
    for i in range(columns):
        face = part.bm.faces.new((lower[i], lower[i + 1], upper[i + 1], upper[i]))
        face.material_index = part.slot(leather)
        for loop, coord in zip(face.loops, ((i, 0), (i + 1, 0), (i + 1, 1), (i, 1))):
            loop[uv].uv = (coord[0] / columns * 2, coord[1] * 0.4)


def rotor(player):
    part = Part(f"Rotor_{player}")
    blades, leaf_width, leaves = (3, 0.28, 1) if player == 1 else (4, 0.34, 2)
    part.blob((0, 0, 0), (0.1, 0.1, 0.07), "stone", seed=player, bumps=0.25)
    for blade in range(blades):
        angle = 2 * math.pi * blade / blades
        out = Vector((math.cos(angle), math.sin(angle), 0))
        side = Vector((-out.y, out.x, 0))
        bone = [out * r + Vector((0, 0, 0.04 * r)) for r in (0.06, 0.12, 0.6, 1.08, 1.15)]
        part.tube(bone, [0.03, 0.022, 0.018, 0.022, 0.032], "bone", sides=8)
        for index in range(leaves):
            leaf(part, out, side, 0.22 + 0.12 * index, 1.3 - 0.25 * index, leaf_width, 0.05 + 0.03 * index,
                 (index - 0.5 * (leaves - 1)) * 0.35, f"leaf_{player}")
        for r in (0.3, 0.85):
            part.lash(out * r + Vector((0, 0, 0.04 * r)), out, 0.02, turns=2)
    return part


def leaf(part, out, side, start, end, width, lift, spread, material):
    """A leaf along the blade from `start` to `end` metres out, folded along its midrib, its front edge a little
    higher (it pitches like a blade), turned `spread` radians off the blade."""
    out = (out * math.cos(spread) + side * math.sin(spread)).normalized()
    side = Vector((-out.y, out.x, 0))
    uv = part.bm.loops.layers.uv.verify()
    rows = []
    steps = 10
    for i in range(steps + 1):
        t = i / steps
        r = start + (end - start) * t
        half = width / 2 * math.sin(math.pi * min(t * 1.15, 1)) ** 0.7
        centre = out * r + Vector((0, 0, lift + 0.035 * r - 0.12 * t * t))
        rows.append([part.bm.verts.new(centre + side * half * s + Vector((0, 0, -0.025 * abs(s) * half / width +
                                                                           0.55 * s * half)))
                     for s in (-1, 0, 1)])
    for i in range(steps):
        for j in range(2):
            face = part.bm.faces.new((rows[i][j], rows[i][j + 1], rows[i + 1][j + 1], rows[i + 1][j]))
            face.material_index = part.slot(material)
            face.smooth = True
            for loop, coord in zip(face.loops, ((j, i), (j + 1, i), (j + 1, i + 1), (j, i + 1))):
                loop[uv].uv = (coord[0] / 2, coord[1] / steps)


def crank():
    part = Part("Crank")
    spread, radius = layout.PEDAL_SPREAD, layout.PEDAL_RADIUS
    part.tube([(-spread - 0.08, 0, 0), (spread + 0.08, 0, 0)], 0.02, "wood", sides=8)
    for side, up in ((1, 1), (-1, -1)):
        x = side * (spread + 0.065)
        part.tube([(x, 0, -up * 0.025), (x, 0, up * (radius + 0.02))], [0.022, 0.018], "bone", sides=8)
        part.tube([(x, 0, up * radius), (side * (spread - 0.05), 0, up * radius)], 0.01, "wood", sides=6)
        part.blob((side * spread, 0, up * radius), (0.045, 0.04, 0.022), "stone", seed=3 + side, bumps=0.2)
    return part


def sling():
    """Two rope loops round the hanging boulder, tied together above it and to the floor."""
    part = Part("Sling")
    middle, semi = Vector(layout.HANGING_MIDDLE), Vector(layout.HANGING_SEMI)
    knot = Vector((0, 0, middle.z + semi.z + 0.06))
    part.tube([(0, 0, FLOOR), knot], ROPE * 1.5, "rope", sides=6, uv_length=0.05, twist=4)
    part.blob(knot, (0.04, 0.04, 0.035), "rope")
    for y in (-semi.y * 0.45, semi.y * 0.45):
        shrink = math.sqrt(1 - (y / semi.y) ** 2)
        points = [Vector((math.sin(a) * (semi.x * shrink + ROPE * 2), y, middle.z + math.cos(a) * (semi.z * shrink +
                                                                                                    ROPE * 2)))
                  for a in (2 * math.pi * i / 40 for i in range(41))]
        part.tube(points, ROPE * 1.3, "rope", sides=6, uv_length=0.05)
        part.tube([knot, points[0]], ROPE * 1.2, "rope", sides=6, uv_length=0.05)
    return part


def build():
    kit.clear_scene()
    materials = copter_materials.make(FOLDER, WOOD, STONE)
    parts = [body(1), body(2), rotor(1), rotor(2), crank(), sling()]
    kit.export(os.path.join(FOLDER, "copter.glb"), [part.done(materials) for part in parts])


build()
