"""The materials of the copter (copter.py): procedural textures of bamboo, wicker, leather, bone, leaves and rope,
and two texture sets of Assets.json (wood: palm_bark, stone: rock_face_03). Each player's copter has its own leather
(dyed in the player's colour) and leaves (the second player's darker and bushier, as in the original)."""
import numpy

import ugh_kit as kit

# the players' colours: leather dyed red ochre and teal (the colours of the clay copters), light and dark leaves
LEATHER = ((0.95, 0.42, 0.16), (0.2, 0.62, 0.68))
LEAVES = ((0.9, 1.0, 0.72), (0.55, 0.78, 0.5))


def make(folder, wood_folder, stone_folder):
    """The materials by name: wood, stone, bamboo, wicker, bone, rope, leather_1, leather_2, leaf_1, leaf_2."""
    size = kit.TEXTURE_SIZE
    made = {"wood": kit.poly_haven_material("wood", wood_folder),
            "stone": kit.poly_haven_material("stone", stone_folder)}

    # bamboo: fibres along V, a node (a darker ridge) once per texture
    fibres = kit.noise(size, 1.5, seed=21, stretch=(12, 1))
    v = (numpy.arange(size)[:, None] + 0.5) / size
    node = numpy.exp(-((v - 0.5) / 0.02) ** 2) * numpy.ones((1, size))
    # green bamboo, freshly cut (step 24b2: the copter greener)
    colour = kit.colour_ramp(fibres, (0.43, 0.46, 0.22), (0.6, 0.62, 0.33)) * (1 - 0.35 * node[..., None])
    made["bamboo"] = kit.material(
        "bamboo", kit.save_image(folder, "copter_bamboo", colour),
        kit.save_image(folder, "copter_bamboo_normal", kit.normals_from_height(fibres * 0.3 + node, 6), colour=False),
        roughness=0.5)

    # wicker: two layers of strips crossing diagonally, over and under, the gaps between them dark. Not cut out: holes
    # finer than a texel of the sun's virtual shadow map make its depth jump between the wall and the rock behind it,
    # and the shadow's rays (SMRT) take those jumps for occluders - the wall shadowed itself in blocks (step 24a)
    u = (numpy.arange(size)[None, :] + 0.5) / size
    a, b = (u + v) * 4, (u - v) * 4
    strip_a = numpy.clip(1 - numpy.abs((a % 1) - 0.5) / 0.32, 0, 1)
    strip_b = numpy.clip(1 - numpy.abs((b % 1) - 0.5) / 0.32, 0, 1)
    a_on_top = ((numpy.floor(a) + numpy.floor(b)) % 2) == 0
    height = numpy.where(a_on_top, numpy.maximum(strip_a + 0.3, strip_b), numpy.maximum(strip_a, strip_b + 0.3))
    gap = 1 - numpy.clip(numpy.maximum(strip_a, strip_b) / 0.25, 0, 1)   # 1 in a gap, 0 on a strip
    grain = kit.noise(size, 1.5, seed=22, stretch=(6, 6))
    colour = kit.colour_ramp(grain, (0.5, 0.38, 0.18), (0.75, 0.6, 0.32)) * (0.55 + 0.45 * height[..., None] / 1.3)
    colour = colour * (1 - 0.85 * gap[..., None])
    made["wicker"] = kit.material(
        "wicker", kit.save_image(folder, "copter_wicker", colour),
        kit.save_image(folder, "copter_wicker_normal", kit.normals_from_height(height, 4), colour=False),
        roughness=0.75, double_sided=True)

    # leather: fine grain and creases, light so that its tint is the player's colour
    grain = kit.noise(size, 1.0, seed=23)
    creases = numpy.abs(kit.noise(size, 2.2, seed=24) - 0.5)
    colour = kit.colour_ramp(grain * 0.5 + creases, (0.55, 0.5, 0.45), (0.85, 0.8, 0.72))
    leather = kit.save_image(folder, "copter_leather", colour)
    leather_normal = kit.save_image(folder, "copter_leather_normal",
                                    kit.normals_from_height(grain * 0.4 - creases, 2.5), colour=False)
    for player, tint in enumerate(LEATHER, 1):
        made[f"leather_{player}"] = kit.material(f"leather_{player}", leather, leather_normal, roughness=0.5,
                                                 tint=tint, double_sided=True)

    # bone: ivory with streaks along it
    streaks = kit.noise(size, 1.8, seed=25, stretch=(8, 1))
    made["bone"] = kit.material(
        "bone", kit.save_image(folder, "copter_bone", kit.colour_ramp(streaks, (0.78, 0.72, 0.6), (0.95, 0.91, 0.8))),
        kit.save_image(folder, "copter_bone_normal", kit.normals_from_height(streaks, 2), colour=False),
        roughness=0.45)

    # rope: twisted strands
    twist = numpy.sin((u * 3 + v * 12) * 2 * numpy.pi) * 0.5 + 0.5
    fuzz = kit.noise(size, 1.2, seed=26)
    made["rope"] = kit.material(
        "rope", kit.save_image(folder, "copter_rope", kit.colour_ramp(twist * 0.7 + fuzz * 0.3, (0.45, 0.35, 0.2),
                                                                     (0.78, 0.66, 0.44))),
        kit.save_image(folder, "copter_rope_normal", kit.normals_from_height(twist, 3), colour=False), roughness=0.85)

    # leaves: U across the leaf, V along it; a midrib, veins slanting out, darker towards the edges
    across = numpy.abs(u - 0.5) * 2
    veins = numpy.clip(1 - numpy.abs(((v - across * 0.35) * 14) % 1 - 0.5) / 0.08, 0, 1) * (across > 0.03)
    rib = numpy.clip(1 - across / 0.03, 0, 1)
    cells = kit.noise(size, 1.8, seed=27)
    colour = kit.colour_ramp(cells * 0.6 + (1 - across) * 0.4, (0.16, 0.34, 0.07), (0.38, 0.62, 0.16))
    colour = colour * (1 - 0.25 * veins[..., None]) + rib[..., None] * 0.25
    leaf = kit.save_image(folder, "copter_leaf", colour)
    leaf_normal = kit.save_image(folder, "copter_leaf_normal", kit.normals_from_height(rib - veins * 0.5, 2),
                                 colour=False)
    for player, tint in enumerate(LEAVES, 1):
        made[f"leaf_{player}"] = kit.material(f"leaf_{player}", leaf, leaf_normal, roughness=0.55, tint=tint,
                                              double_sided=True)
    return made
