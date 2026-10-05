"""The boards with the pads' numbers for the remake as signs.glb: a weathered board pegged to a crooked post, the pad's
number carved into it in tally marks as the original's sprites draw it (sign_1 one stroke .. sign_4 four, sign_5 four
crossed by a fifth; sign_0 a blank board, the original's for a pad without a number). The cuts show the pale fresh
wood under the weathered grey surface, so the strokes read from afar.

As the original's board (16 x 12 px of 10 cm): 1.46 m wide, 0.8 m high from 0.3 m above the ground, the post 1.22 m;
facing -Y, its origin at the foot of the post (on the ground, the post a little sunk into it).

    blender -b --factory-startup --python-exit-code 1 --python signs.py -- <folder> <rough_wood folder>
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import bmesh  # noqa: E402
import numpy  # noqa: E402
from mathutils import Vector  # noqa: E402

import ugh_kit as kit  # noqa: E402

FOLDER, WOOD = kit.arguments()[:2]
SLOTS = ("wood", "cut", "post")

WIDTH, HEIGHT, BOTTOM, THICKNESS = 1.46, 0.8, 0.3, 0.09
POST_RADIUS, POST_TOP, POST_SUNK = 0.085, 1.22, 0.15
# the board's front as a grid of this many cells (about 1.5 cm), its corners rounded, its edges a little uneven
CELLS_ACROSS, CELLS_UP, CORNER, RAGGED = 96, 52, 0.08, 0.018
# a stroke: this long, this wide, cut this deep, this far from the next one; the texture covers TEXTURE_METRES
STROKE_LENGTH, STROKE_WIDTH, STROKE_DEPTH, STROKE_GAP = 0.5, 0.11, 0.032, 0.22
TEXTURE_METRES = 0.5


def materials():
    """The weathered wood (rough_wood warmed into the brown of the original's board), the pale fresh
    wood of the cuts, the post's wood darker."""
    asset = os.path.basename(os.path.normpath(WOOD))
    colour = kit.load_image(os.path.join(WOOD, f"{asset}_diff_2k.jpg"), size=1024)
    pixels = numpy.array(colour.pixels[:], dtype=numpy.float32).reshape(1024, 1024, 4)
    pixels[..., :3] = numpy.clip(pixels[..., :3] * numpy.array((1.0, 0.84, 0.68)), 0, 1)
    warm = kit.save_image(FOLDER, "sign_wood", pixels[..., :3])
    normal = kit.load_image(os.path.join(WOOD, f"{asset}_nor_dx_2k.jpg"), colour=False, size=1024, flip_green=True)
    arm = kit.load_image(os.path.join(WOOD, f"{asset}_arm_2k.jpg"), colour=False, size=1024)
    return [kit.material("sign_wood", warm, normal, roughness=arm),
            kit.material("sign_cut", (0.78, 0.62, 0.42), normal, roughness=0.8),
            kit.material("sign_post", colour, normal, roughness=arm, tint=(0.75, 0.7, 0.65))]


def strokes(marks):
    """The strokes of `marks` tally marks as segments ((x, z), (x, z)) on the board's front."""
    middle = BOTTOM + HEIGHT / 2
    upright = min(marks, 4)
    lines = []
    for index in range(upright):
        x = (index - (upright - 1) / 2) * STROKE_GAP
        lines.append(((x, middle - STROKE_LENGTH / 2), (x, middle + STROKE_LENGTH / 2)))
    if marks == 5:   # the fifth crosses the four from the bottom left to the top right
        reach = 1.5 * STROKE_GAP + 0.14
        lines.append(((-reach, middle - STROKE_LENGTH * 0.32), (reach, middle + STROKE_LENGTH * 0.32)))
    return lines


def distance_to(point, segment):
    (ax, az), (bx, bz) = segment
    px, pz = point
    dx, dz = bx - ax, bz - az
    along = max(0.0, min(1.0, ((px - ax) * dx + (pz - az) * dz) / (dx * dx + dz * dz)))
    return math.hypot(px - ax - along * dx, pz - az - along * dz)


def inside(x, z, ragged):
    """The point of the board's front is on the board: a rectangle with rounded corners, its edges `ragged`."""
    half_w, half_h = WIDTH / 2 + ragged, HEIGHT / 2 + ragged
    dx = max(abs(x) - (half_w - CORNER), 0.0)
    dz = max(abs(z - BOTTOM - HEIGHT / 2) - (half_h - CORNER), 0.0)
    return math.hypot(dx, dz) <= CORNER


def board(bm, marks, seed):
    """The board: its front a grid cut in by the strokes (V-shaped grooves in the pale wood), solidified back."""
    uv = bm.loops.layers.uv.verify()
    lines = strokes(marks)
    warp = kit.noise(64, 2.6, seed=seed)
    edge = kit.noise(64, 2.0, seed=seed + 1)
    step_x, step_z = WIDTH / CELLS_ACROSS, HEIGHT / CELLS_UP
    verts = {}

    def vertex(i, j):
        if (i, j) not in verts:
            x, z = -WIDTH / 2 + i * step_x, BOTTOM + j * step_z
            groove = max((max(0.0, 1 - (distance_to((x, z), line) / (STROKE_WIDTH / 2)) ** 2) for line in lines),
                         default=0.0)
            bow = (warp[j % 64, i % 64] - 0.5) * 0.008
            verts[(i, j)] = bm.verts.new((x, -THICKNESS / 2 + groove * STROKE_DEPTH + bow, z))
        return verts[(i, j)]

    for i in range(CELLS_ACROSS):
        for j in range(CELLS_UP):
            x, z = -WIDTH / 2 + (i + 0.5) * step_x, BOTTOM + (j + 0.5) * step_z
            ragged = (edge[j % 64, i % 64] - 0.5) * 2 * RAGGED
            if not inside(x, z, ragged):
                continue
            corners = (vertex(i, j), vertex(i, j + 1), vertex(i + 1, j + 1), vertex(i + 1, j))
            face = bm.faces.new(corners)
            in_cut = any(distance_to((x, z), line) < STROKE_WIDTH * 0.42 for line in lines)
            face.material_index = SLOTS.index("cut" if in_cut else "wood")
            face.smooth = True
            for loop in face.loops:   # the grain along the board
                loop[uv].uv = (loop.vert.co.z / TEXTURE_METRES, loop.vert.co.x / TEXTURE_METRES + seed * 0.37)
    front = list(bm.faces)
    bmesh.ops.recalc_face_normals(bm, faces=front)
    if front[0].normal.y > 0:
        bmesh.ops.reverse_faces(bm, faces=front)
    bmesh.ops.solidify(bm, geom=front, thickness=THICKNESS)


def post(bm, seed):
    """The post behind the board's middle, a little crooked, its top above the board; two pegs hold the board."""
    rng = numpy.random.default_rng(seed)
    y = THICKNESS / 2 + POST_RADIUS * 0.9
    points = [Vector((rng.uniform(-0.02, 0.02), y + rng.uniform(-0.01, 0.01), z))
              for z in numpy.linspace(-POST_SUNK, POST_TOP, 7)]
    radii = [POST_RADIUS * (1.05 - 0.15 * t) for t in numpy.linspace(0, 1, 7)]
    kit.tube(bm, points, radii, sides=12, uv_length=TEXTURE_METRES, material_index=SLOTS.index("post"))
    for z in (BOTTOM + 0.07, BOTTOM + HEIGHT - 0.07):
        kit.tube(bm, [Vector((0, -THICKNESS / 2 - 0.012, z)), Vector((0, y, z))], [0.024, 0.024], sides=8,
                 material_index=SLOTS.index("post"))


def sign(made, marks):
    bm = bmesh.new()
    board(bm, marks, 300 + marks)
    post(bm, 310 + marks)
    return kit.mesh_object(f"sign_{marks}", bm, made)


def build():
    kit.clear_scene()
    made = materials()
    kit.export(os.path.join(FOLDER, "signs.glb"), [sign(made, marks) for marks in range(6)])


build()
