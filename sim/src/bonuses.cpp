// Bonus items (12 slots) - 113b:2b7f .. 113b:2d1b and 2207, Bonuses.kt.
#include "bonuses.hpp"

#include "game.hpp"

namespace ugh {

namespace {

constexpr int16_t LYING_TIME = 0x230;
constexpr int16_t MAX_ENERGY = 0x5a3b;
constexpr uint8_t MAX_LIVES = 0x63;

extern const BonusState Falling, Lying;

/** The touch box of a bonus item: 6 x 6 px around its middle, above its bottom. */
Box touchBox(const BonusKind& kind) { return {kind.x, kind.y, 3, 3}; }

/** 113b:2c97 - landed: lies on the pad for a while. */
void land(BonusItem& b) {
    b.state = &Lying;
    b.vx = LYING_TIME;
}

/** 113b:2be4 - flies and falls until it lands on a pad or leaves the screen. */
void falling(Game& game, int slot) {
    BonusItem& b = game.world.bonuses[slot];
    Fixed x = b.x + Fixed(b.vx);
    if (x <= Fixed(-0x200) || x >= Fixed(0x2800)) { b.sprite = NO_SPRITE; return; }
    b.x = x;
    b.vy = static_cast<int16_t>(b.vy + 3);
    if (b.vy < 0) { b.y += Fixed(b.vy); return; }
    Fixed before = b.y;
    Fixed y = before + Fixed(b.vy);
    if (y >= Fixed(0x1800)) { b.sprite = NO_SPRITE; return; }
    b.y = y;
    auto bottom = static_cast<int16_t>(y.pixels() + b.kind->y);
    auto bottomBefore = static_cast<int16_t>(before.pixels() + b.kind->y);
    auto middle = static_cast<int16_t>(b.x.pixels() + b.kind->x);
    for (int i = 0; i < game.world.padCount; i++) {
        const Pad& pad = game.world.pads[i];
        if (bottomBefore < pad.y && bottom >= pad.y && middle >= pad.left && middle - 1 <= pad.right) {
            b.y = Fixed::fromPixels(pad.y - b.kind->y);
            land(b);
            return;
        }
    }
}

/** 113b:2ca9 - lying; a copter touching it collects it: energy, a life or a higher score multiplier. */
void lying(Game& game, int slot) {
    World& world = game.world;
    BonusItem& b = world.bonuses[slot];
    b.vx = static_cast<int16_t>(b.vx - 1);
    if (b.vx == 0) { b.sprite = NO_SPRITE; return; }
    int copter = touchingCopter(world, touchBox(*b.kind), b.x, b.y);
    if (copter < 0) return;
    switch (b.kind->effect) {
        case BonusKind::Effect::Energy: {
            auto energy = static_cast<uint16_t>(b.kind->amount + world.energy);
            world.energy = static_cast<int16_t>(energy > MAX_ENERGY ? MAX_ENERGY : energy);
            break;
        }
        case BonusKind::Effect::Life: {
            auto lives = static_cast<uint8_t>(b.kind->amount + world.lives);
            world.lives = lives > MAX_LIVES ? MAX_LIVES : lives;
            break;
        }
        case BonusKind::Effect::Multiplier:
            if (world.multiplier < game.data.multiplierLimit(world.difficulty)) world.multiplier++;
            break;
    }
    b.sprite = NO_SPRITE;
    game.report({EventKind::BonusCollected, copter, slot, static_cast<int>(b.kind->effect)});
}

const BonusState Falling{"Falling", falling};
const BonusState Lying{"Lying", lying};

}  // namespace

const std::vector<const BonusState*>& bonusStates() {
    static const std::vector<const BonusState*> all = {&Falling, &Lying};
    return all;
}

void dropBonus(Game& game, const BonusKind& kind, Fixed x, Fixed y, int16_t vx, int16_t lift) {
    auto& slots = game.world.bonuses;
    int slot = static_cast<int>(slots.size()) - 1;
    while (slot >= 0 && (slots[slot].sprite & 0x8000) == 0) slot--;   // a free slot, the last first
    if (slot < 0) {
        // the original returns without its POP BX and jumps to CS:BX
        game.problems.push_back("all 12 bonus slots in use: the original would jump to CS:BX");
        return;
    }
    BonusItem& b = slots[slot];
    b.kind = &kind;
    b.x = x - Fixed::fromPixels(kind.x);
    b.y = y - Fixed(kind.y * 16);   // half its height
    b.vx = vx;
    b.vy = static_cast<int16_t>(-(lift + kind.lift));
    b.sprite = kind.sprite;
    b.state = &Falling;
}

/** 113b:2b7f - Bonuses.kt bonusesUpdate: every bonus item in use, the last slot first. */
void updateBonuses(Game& game) {
    for (int slot = static_cast<int>(game.world.bonuses.size()) - 1; slot >= 0; slot--) {
        BonusItem& b = game.world.bonuses[slot];
        if (!b.used()) continue;
        if (b.state && b.kind) b.state->update(game, slot);
        else game.problems.push_back("bonus item " + std::to_string(slot) + " without a state or kind");
    }
}

}  // namespace ugh
