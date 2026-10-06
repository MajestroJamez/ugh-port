"""The copter of the remake: a stone age pedal copter as copter.glb, its parts as meshes of their own, each with its
origin where the game turns it (copter_layout):

- Body_1, Body_2 (the players' copters, leather dyed in their colours): a cage of bamboo poles lashed with rope on a
  floor of logs, wicker low at the sides, open at the back but for two poles crossed, ivy winding up the posts and
  the crossed poles and along the top beams, lianas hanging from them, moss on the logs (vines.py's stems, leaves and
  moss), the pilot's seat (a leather seat with a backrest on bamboo legs), the passenger's chair (a seat of split
  logs with a leather cushion, a back of bone ribs, armrests ending in tusks), the handlebar on the post that bears
  the crank, the hanger that bears the layshaft, small leather pennants under the top front beam and leather collars
  at the top of the posts, tusks on the front corners. Origin: the bottom middle of the body.
- Rotor_1, Rotor_2: a stone hub with bone blades, a big leaf lashed on each (the second player's rotor bushier, as in
  the original). Origin: the hub, it turns about Z.
- Shaft: the rotor's shaft down into the cage with the crown wheel the layshaft's pinion turns (copter_layout's
  drive). Origin: the hub, it turns with the rotor.
- Crank: the axle with bone arms and stone pedals the pilot pedals, and the chainring on it. Origin: the middle of
  the axle, in the pilot's frame (the game turns it by PILOT_YAW); it turns about X, the left pedal (+X) on top.
- Drive: the layshaft with the sprocket the chain turns and the lantern pinion. Origin: the sprocket's middle, in the
  pilot's frame as the crank (the rotor's shaft towards -X); it turns about X, RATIO times as fast as the crank.
- ChainLink: one link of the chain (bone), along Y; the game moves the links along the chain.
- Sling: the rope that holds a hanging stone passenger below the body. Origin: the bottom middle of the body.

    blender -b --factory-startup --python-exit-code 1 --python copter.py -- <folder> <palm_bark> <rock_face_03>

(the last two: the folders of those texture sets of Assets.json)
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import bmesh  # noqa: E402
import numpy  # noqa: E402
from mathutils import Vector  # noqa: E402

import copter_layout as layout  # noqa: E402
import copter_materials  # noqa: E402
import ugh_kit as kit  # noqa: E402
import vines  # noqa: E402

FOLDER, WOOD, STONE = kit.arguments()[:3]
HALF_X, HALF_Y = layout.BODY_WIDTH / 2 - 0.1, layout.BODY_DEPTH - 0.08   # the corner posts
FLOOR, TOP, RAIL = 0.12, 1.72, 0.92
POLE, ROPE = 0.055, 0.009
# the copter's body (copter_layout: BODY_WIDTH x BODY_HEIGHT, BODY_DEPTH in front and behind)
BOX = ((-layout.BODY_WIDTH / 2, -layout.BODY_DEPTH, 0.0), (layout.BODY_WIDTH / 2, layout.BODY_DEPTH, layout.BODY_HEIGHT))


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


def clamped(part, make):
    """What `make()` adds to the part, its vertices pushed into the copter's body (BOX): ivy and lianas on the
    frame's edges (copter_layout's body, which the game's test holds the model to)."""
    part.bm.verts.ensure_lookup_table()
    first = len(part.bm.verts)
    make()
    part.bm.verts.ensure_lookup_table()
    for vert in part.bm.verts[first:]:
        vert.co = Vector([min(max(c, low), high) for c, low, high in zip(vert.co, *BOX)])


def ivy(part, a, b, radius, seed, turns=1.6, leaves=46.0, sizes=(0.055, 0.11)):
    """Ivy winding round a pole of `radius` from a to b: a thin stem in a loose spiral (`turns` a metre), leaves on
    it (`leaves` a metre) lying on the pole, pointing along it and away from it, mostly drooping."""
    a, b = Vector(a), Vector(b)
    axis = (b - a).normalized()
    side = axis.orthogonal().normalized()
    up = axis.cross(side)
    length = (b - a).length
    rng = numpy.random.default_rng(seed)
    phase = rng.uniform(0, 2 * math.pi)
    steps = max(8, int(length * 24))
    points, radials = [], []
    for i in range(steps + 1):
        t = i / steps
        angle = phase + 2 * math.pi * turns * length * t + 0.4 * math.sin(7 * t + phase)
        radial = side * math.cos(angle) + up * math.sin(angle)
        points.append(a + (b - a) * t + radial * (radius + 0.012))
        radials.append(radial)
    uv = part.bm.loops.layers.uv.verify()

    def make():
        part.tube(points, [0.011 - 0.004 * i / steps for i in range(steps + 1)], "stem", sides=5, uv_length=0.5)
        for point, radial in zip(points, radials):
            for _ in range(int(rng.poisson(leaves * length / steps))):
                tangent = axis.cross(radial)
                out = (axis * rng.uniform(-1, 0.35) + tangent * rng.uniform(-0.8, 0.8) + radial * 0.2).normalized()
                vines.leaf(part.bm, point + radial * 0.006, out, rng.uniform(*sizes), uv, facing=radial,
                           material_index=part.slot("leaf"))
    clamped(part, make)


def liana(part, top, length, seed):
    """A liana (vines.py's stem with its leaves, swaying less) hanging from `top` on the frame, a tuft of moss there."""
    rng = numpy.random.default_rng(seed)
    path = [Vector(top) + Vector((p.x * 0.6, p.y * 0.25, p.z)) for p in vines.stem_path(length, rng, 0.1)]
    uv = part.bm.loops.layers.uv.verify()
    clamped(part, lambda: vines.hanging(part.bm, path, 0.02, rng, uv, leaves=6.0, sizes=(0.07, 0.14),
                                        stem=part.slot("stem"), leafy=part.slot("leaf")))
    part.blob(Vector(top) + Vector((0, 0, 0.02)), (0.07, 0.05, 0.045), "moss", seed=seed, bumps=0.3)


def pennants(part, leather, count=5):
    """Small leather pennants in the player's colour hanging from the top front beam, a little wavy."""
    uv = part.bm.loops.layers.uv.verify()
    width, height, y = 0.25, 0.22, -HALF_Y - 0.06
    for index in range(count):
        x = (index - (count - 1) / 2) * 0.38
        rows = 4
        verts = []
        for row in range(rows + 1):
            t = row / rows
            half = width / 2 * (1 - t)
            wave = 0.012 * math.sin(math.pi * t + index)
            verts.append([part.bm.verts.new((x + s * half + 0.015 * t * t, y - wave, TOP - 0.02 - height * t))
                          for s in ((-1, 1) if row < rows else (0,))])
        for row in range(rows):
            (a, b), below = verts[row], verts[row + 1]
            face = part.bm.faces.new((below[0], below[1], b, a) if len(below) == 2 else (below[0], b, a))
            face.material_index = part.slot(leather)
            for loop in face.loops:
                local = loop.vert.co
                loop[uv].uv = ((local.x - x) / width + 0.5 + index * 0.37, (TOP - local.z) / height * 0.5)


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
    """The cage, open at the back (step 24b2): no wall there but two bamboo poles crossed, ivy winding up them and
    up the corner posts, lianas hanging from the top beams, moss on the logs; the player's colour on the pennants
    under the top front beam, the collars at the top of the posts and the seats' leather."""
    part = Part(f"Body_{player}")
    leather = f"leather_{player}"
    corners = [(x, y) for x in (-HALF_X, HALF_X) for y in (-HALF_Y, HALF_Y)]
    for index, (x, y) in enumerate(corners):
        part.tube([(x, y, 0.02), (x, y, TOP + 0.08)], POLE, "bamboo", uv_length=0.36)
        part.tube([(x, y, TOP - 0.12), (x, y, TOP - 0.02)], POLE + 0.006, leather, sides=12)
        part.lash((x, y, RAIL), (0, 0, 1), POLE)
        ivy(part, (x, y, FLOOR), (x, y, TOP - 0.13), POLE, seed=40 + 10 * player + index)
    for log in range(5):
        y = -HALF_Y + 2 * HALF_Y * log / 4
        part.tube([(-HALF_X - 0.08, y, FLOOR - 0.04), (HALF_X + 0.08, y, FLOOR - 0.04)], 0.07, "wood")
    for y in (-HALF_Y, HALF_Y):   # beams along X at the top
        part.tube([(-HALF_X - 0.08, y, TOP), (HALF_X + 0.08, y, TOP)], POLE * 0.9, "bamboo", uv_length=0.36)
    for x in (-HALF_X, HALF_X):   # beams along Y at the top and the rails at the sides
        part.tube([(x, -HALF_Y - 0.08, TOP + 0.05), (x, HALF_Y + 0.08, TOP + 0.05)], POLE * 0.9, "bamboo",
                  uv_length=0.36)
        part.tube([(x, -HALF_Y, RAIL), (x, HALF_Y, RAIL)], POLE * 0.8, "bamboo", uv_length=0.36)
        wicker(part, (x, -HALF_Y), (x, HALF_Y), FLOOR, RAIL)
    # the back: two poles crossed, lashed where they cross, ivy up both; open between them (no wall: the rock is seen
    # through it, and no cut-out holes for the shadow map, step 24a)
    low, high = FLOOR + 0.02, TOP - 0.04
    for sign, dy in ((1, -0.022), (-1, 0.022)):
        a, b = (-sign * (HALF_X - 0.03), HALF_Y + dy, low), (sign * (HALF_X - 0.03), HALF_Y + dy, high)
        part.tube([a, b], POLE * 0.85, "bamboo", uv_length=0.36)
        ivy(part, a, b, POLE * 0.85, seed=60 + 10 * player + sign, turns=1.2, leaves=34.0)
    part.lash((0, HALF_Y, (low + high) / 2), (0, 1, 0), POLE * 1.5, turns=3)
    # ivy along the top front beam from the corners, and lianas hanging from the top beams (not in front of the
    # riders' faces, the crank or the chain)
    for side in (-1, 1):
        ivy(part, (side * (HALF_X + 0.02), -HALF_Y, TOP), (side * (HALF_X - 0.42), -HALF_Y, TOP), POLE * 0.9,
            seed=70 + 10 * player + side, turns=2.0, leaves=40.0, sizes=(0.045, 0.085))
        ivy(part, (side * HALF_X, -HALF_Y, TOP + 0.05), (side * HALF_X, HALF_Y, TOP + 0.05), POLE * 0.9,
            seed=75 + 10 * player + side, turns=2.0, leaves=30.0, sizes=(0.045, 0.085))
    hanging = ((-0.84, -HALF_Y + 0.04, 0.62), (0.86, -HALF_Y + 0.04, 0.66), (-0.7, HALF_Y - 0.04, 0.95),
               (0.12, HALF_Y - 0.04, 0.6), (0.72, HALF_Y - 0.04, 1.1))
    for index, (x, y, length) in enumerate(hanging):
        liana(part, (x, y, TOP - 0.04), length, seed=100 + 10 * player + index)
    for x in (-HALF_X, HALF_X):   # moss on the logs' ends
        for y in (-HALF_Y, 0.0, HALF_Y):
            part.blob((x * 0.96, y * 0.85, FLOOR + 0.02), (0.06, 0.07, 0.035), "moss",
                      seed=int(200 + 37 * x + 11 * y), bumps=0.35)
    # the mast's cross beams and the mast up to the rotor's hub
    part.tube([(-HALF_X, 0, TOP + 0.05), (HALF_X, 0, TOP + 0.05)], POLE * 0.8, "bamboo", uv_length=0.36)
    part.tube([(0, 0, TOP + 0.04), (0, 0, layout.ROTOR_HUB[2] - 0.05)], 0.05, "wood")
    part.lash((0, 0, TOP + 0.05), (0, 0, 1), 0.05)
    pilot_seat(part, leather)
    passenger_chair(part, leather)
    layshaft_hanger(part)
    pennants(part, leather)
    for side in (-1, 1):   # tusks on the front corners
        points = [(side * (HALF_X + 0.02 * t), -HALF_Y - 0.04 * t, TOP + 0.05 + 0.2 * t + 0.02 * math.sin(3 * t))
                  for t in (i / 6 for i in range(7))]
        part.tube(points, [0.04 - 0.032 * i / 6 for i in range(7)], "bone", sides=8)
    inside(part)
    return part


def inside(part):
    """Fails unless the part is in the copter's body (BOX), as the game's test Ugh.Copter.Model wants it."""
    for vert in part.bm.verts:
        for c, low, high in zip(vert.co, *BOX):
            if not low - 1e-4 <= c <= high + 1e-4:
                raise ValueError(f"{part.name}: {tuple(vert.co)} is out of the body {BOX}")


def frame_of(seat, yaw):
    """A point of a seat's own frame (facing -Y) in the copter."""
    return lambda offset: Vector(seat) + Vector(layout.turned(offset, yaw))


def pilot_seat(part, leather):
    """A leather seat on four bamboo legs with a slanting backrest, and the handlebar on the post that bears the crank;
    an outer post bears the axle beyond the chainring."""
    at = frame_of(layout.PILOT_SEAT, layout.PILOT_YAW)
    floor = FLOOR - layout.PILOT_SEAT[2]
    corners = [(x, y) for y in (-0.12, 0.16) for x in (-0.15, 0.15)]
    for x, y in corners:
        part.tube([at((x * 1.25, y + (0.04 if y > 0 else -0.04), floor)), at((x, y, -0.07))], 0.024, "bamboo",
                  uv_length=0.22)
        part.lash(at((x, y, -0.075)), (0, 0, 1), 0.024, turns=2)
    for (x0, y0), (x1, y1) in ((corners[0], corners[1]), (corners[2], corners[3]), (corners[0], corners[2]),
                               (corners[1], corners[3])):
        part.tube([at((x0, y0, -0.07)), at((x1, y1, -0.07))], 0.02, "bamboo", uv_length=0.22)
    part.blob(at((0, 0.02, -0.035)), (0.17, 0.16, 0.04), leather)   # the seat: leather stuffed with moss
    tops = []
    for x in (-0.14, 0.14):   # the backrest
        top = at((x * 0.9, 0.25, 0.4))
        part.tube([at((x, 0.16, -0.07)), top], 0.022, "bamboo", uv_length=0.22)
        tops.append(top)
    width = tops[1] - tops[0]
    part.tube([tops[0] - width * 0.15, tops[1] + width * 0.15], 0.02, "bone", sides=8)
    for end in (tops[0] - width * 0.17, tops[1] + width * 0.17):
        part.blob(end, (0.026, 0.026, 0.026), "bone")
    part.blob(at((0, 0.215, 0.2)), (0.125, 0.022, 0.15), leather)
    # the handlebar on the post bearing the crank's axle, in front of the pilot
    axle = at(layout.PEDAL_AXLE)
    left, right = (at((side * layout.GRIP[0], layout.GRIP[1], layout.GRIP[2])) for side in (1, -1))
    across = Vector(layout.turned((1, 0, 0), layout.PILOT_YAW))
    top = Vector((axle.x, axle.y, left.z))
    part.tube([(axle.x, axle.y, FLOOR), top], 0.035, "bamboo", uv_length=0.22)
    part.tube([right - across * 0.04, left + across * 0.04], [0.026, 0.026], "bone", sides=8)
    for end in (right - across * 0.05, left + across * 0.05):
        part.blob(end, (0.035, 0.035, 0.035), "bone")
    part.lash(top, across, 0.026)
    part.lash(axle, (0, 0, 1), 0.035)
    outer = at((layout.CHAINRING_SIDE + 0.06, layout.PEDAL_AXLE[1], layout.PEDAL_AXLE[2]))
    part.tube([(outer.x, outer.y, FLOOR), outer + Vector((0, 0, 0.03))], 0.026, "bamboo", uv_length=0.22)
    part.lash(outer, (0, 0, 1), 0.026, turns=2)


def passenger_chair(part, leather):
    """A chair of logs: a seat of split logs with a leather cushion, a back of bone ribs under a bamboo rail, armrests
    of bamboo ending in tusks."""
    at = frame_of(layout.PASSENGER_SEAT, layout.PASSENGER_YAW)
    floor = FLOOR - layout.PASSENGER_SEAT[2]
    for x in (-0.17, 0.17):
        for y in (-0.13, 0.12):
            part.tube([at((x, y, floor)), at((x, y, -0.06))], 0.034, "bamboo", uv_length=0.3)
    for y in (-0.12, -0.04, 0.04, 0.12):   # the seat: split logs across
        part.tube([at((-0.2, y, -0.065)), at((0.2, y, -0.065))], 0.035, "wood", sides=10)
    part.blob(at((0, 0.0, -0.025)), (0.18, 0.15, 0.035), leather)
    rail = []
    for x in (-0.18, 0.18):   # the back's posts and the arms
        top = at((x, 0.15, 0.5))
        part.tube([at((x, 0.12, -0.06)), top], 0.03, "bamboo", uv_length=0.3)
        rail.append(top)
        part.tube([at((x * 1.12, 0.12, 0.18)), at((x * 1.15, -0.05, 0.19)), at((x * 1.12, -0.2, 0.2))], 0.022,
                  "bamboo", uv_length=0.3)
        part.tube([at((x * 1.12, -0.13, floor + 0.3)), at((x * 1.12, -0.14, 0.19))], 0.02, "bamboo", uv_length=0.3)
        part.lash(at((x * 1.12, -0.14, 0.19)), (0, 0, 1), 0.02, turns=2)
        tip = [at((x * 1.12 + x * 0.05 * t, -0.2 - 0.09 * t, 0.2 + 0.06 * t * t)) for t in (i / 5 for i in range(6))]
        part.tube(tip, [0.026 - 0.02 * i / 5 for i in range(6)], "bone", sides=8)
    width = rail[1] - rail[0]
    part.tube([rail[0] - width * 0.04, rail[1] + width * 0.04], 0.03, "bamboo", uv_length=0.3)
    for top in rail:
        part.lash(top, width, 0.03, turns=2)
    for index in range(4):   # the ribs, bowed backwards
        x = -0.12 + 0.08 * index
        part.tube([at((x, 0.12, -0.03)), at((x, 0.18, 0.2)), at((x, 0.16, 0.47))], [0.016, 0.02, 0.014], "bone",
                  sides=8)


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


def wheel(part, centre, radius, teeth, tooth, spokes, material="wood"):
    """A wheel square to X at `centre`: a rim, spokes of bone from a stone hub, `teeth` pegs out of the rim up to
    `tooth` beyond `radius` (where the chain's links run)."""
    centre = Vector(centre)
    rim = radius - 0.012

    def on(r, angle):
        return centre + Vector((0, r * math.cos(angle), r * math.sin(angle)))
    part.tube([on(rim, 2 * math.pi * i / 48) for i in range(49)], 0.013, material, sides=8)
    for i in range(spokes):
        angle = 2 * math.pi * (i + 0.5) / spokes
        part.tube([on(0.02, angle), on(rim, angle)], 0.009, "bone", sides=6)
    part.blob(centre, (0.03, 0.035, 0.035), "stone", seed=7, bumps=0.15)
    for i in range(teeth):
        angle = 2 * math.pi * i / teeth
        part.tube([on(rim - 0.005, angle), on(radius + tooth, angle)], [0.009, 0.006], "bone", sides=6)


def crank():
    part = Part("Crank")
    spread, radius = layout.PEDAL_SPREAD, layout.PEDAL_RADIUS
    side = layout.CHAINRING_SIDE
    part.tube([(-spread - 0.08, 0, 0), (side + 0.08, 0, 0)], 0.02, "wood", sides=8)
    for x_side, up in ((1, 1), (-1, -1)):
        x = x_side * (spread + 0.065)
        part.tube([(x, 0, -up * 0.03), (x, 0, up * (radius + 0.025))], [0.024, 0.019], "bone", sides=8)
        part.tube([(x, 0, up * radius), (x_side * (spread - 0.06), 0, up * radius)], 0.011, "wood", sides=6)
        part.blob((x_side * spread, 0, up * radius), (0.055, 0.05, 0.024), "stone", seed=3 + x_side, bumps=0.2)
    wheel(part, (side, 0, 0), layout.CHAINRING_RADIUS, layout.CHAINRING_TEETH, 0.016, 6)
    return part


def drive():
    """The layshaft (the rotor's shaft towards -X) with the sprocket at the origin and the lantern pinion: two discs
    with pegs between them that the crown wheel's pegs mesh with."""
    part = Part("Drive")
    sprocket = layout.sprocket()
    pinion = layout.CROWN_RADIUS - math.hypot(sprocket[0], sprocket[1])
    part.tube([(pinion - 0.035, 0, 0), (0.05, 0, 0)], 0.016, "wood", sides=8)
    wheel(part, (0, 0, 0), layout.SPROCKET_RADIUS, layout.CHAINRING_TEETH // layout.RATIO, 0.014, 3)
    for x in (pinion - 0.024, pinion + 0.024):
        part.blob((x, 0, 0), (0.008, layout.CROWN_RADIUS + 0.012, layout.CROWN_RADIUS + 0.012), "wood")
    for i in range(layout.CROWN_PEGS):
        angle = 2 * math.pi * (i + 0.5) / layout.CROWN_PEGS
        y, z = layout.CROWN_RADIUS * math.cos(angle), layout.CROWN_RADIUS * math.sin(angle)
        part.tube([(pinion - 0.024, y, z), (pinion + 0.024, y, z)], 0.009, "bone", sides=6)
    return part


def shaft():
    """The rotor's shaft from the hub down to the crown wheel: a disc with pegs pointing down at CROWN_RADIUS to the
    pinion's top."""
    part = Part("Shaft")
    crown = layout.LAYSHAFT_HEIGHT + layout.CROWN_RADIUS + 0.05 - layout.ROTOR_HUB[2]
    part.tube([(0, 0, -0.04), (0, 0, crown - 0.015)], 0.028, "wood", sides=10)
    part.blob((0, 0, crown), (layout.CROWN_RADIUS + 0.03, layout.CROWN_RADIUS + 0.03, 0.014), "wood")
    for i in range(layout.CROWN_PEGS):
        angle = 2 * math.pi * i / layout.CROWN_PEGS
        x, y = layout.CROWN_RADIUS * math.cos(angle), layout.CROWN_RADIUS * math.sin(angle)
        part.tube([(x, y, crown - 0.005), (x, y, crown - 0.055)], [0.01, 0.007], "bone", sides=6)
    return part


def layshaft_hanger(part):
    """A bamboo hanger from the top front beam to the layshaft's end beyond the sprocket, lashed round it."""
    across = Vector(layout.turned((1, 0, 0), layout.PILOT_YAW))
    end = Vector(layout.sprocket()) + across * 0.035
    top = Vector((end.x, -HALF_Y, TOP))
    part.tube([top, end + Vector((0, 0, 0.03))], 0.022, "bamboo", uv_length=0.22)
    part.lash(top, (1, 0, 0), POLE * 0.9, turns=2)
    part.lash(end, across, 0.018, turns=2)


def chain_link():
    """A bone link along Y, its knuckles reaching the next links'."""
    part = Part("ChainLink")
    half = layout.CHAIN_PITCH * 0.42
    part.tube([(0, -half, 0), (0, half, 0)], 0.011, "bone", sides=8)
    for y in (-half, half):
        part.blob((0, y, 0), (0.016, 0.014, 0.016), "bone")
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
    materials.update((made.name, made) for made in vines.materials(FOLDER))   # stem, leaf, moss: ivy and lianas
    parts = [body(1), body(2), rotor(1), rotor(2), shaft(), crank(), drive(), chain_link(), sling()]
    kit.export(os.path.join(FOLDER, "copter.glb"), [part.done(materials) for part in parts])


build()
