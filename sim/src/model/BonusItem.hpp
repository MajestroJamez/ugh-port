// A bonus item.
#pragma once

#include "core/Fixed.hpp"
#include "core/Word.hpp"
#include "data/BonusKind.hpp"
#include "data/Sprite.hpp"
#include "model/BonusTimer.hpp"

namespace ugh::bonuses {
class BonusState;
}

namespace ugh::model {

class Level;

/**
 * A bonus item: dropped by a tree or for a quick delivery, it falls onto a pad and lies there until a copter
 * collects it or its time is up. Its slot is free while it has no sprite.
 */
class BonusItem {
public:
    struct Snapshot {
        const data::BonusKind* kind = nullptr;
        const bonuses::BonusState* state = nullptr;
        data::Sprite sprite = data::NO_SPRITE;
        core::Fixed x, y;       // top left
        BonusTimer timer;
        core::Word vy;          // the y speed (1/32 px per frame)
    };

    BonusItem() = default;
    /** The item in slot `index` (the events name it by the index). */
    explicit BonusItem(int index) : index_(index) {}

    int index() const { return index_; }
    const data::BonusKind& kind() const { return *s_.kind; }

    /** It is in play: it has a sprite. */
    bool inUse() const { return s_.sprite != data::NO_SPRITE; }

    /** The slot can take a new item: the original looks only at the sign of the sprite word. */
    bool freeForDrop() const { return core::Word(s_.sprite) < 0; }

    /**
     * 113b:2b96 - a new item of `kind` with its middle / bottom at x, y, flying sideways with `vx` and up with its
     * kind's lift plus `lift` (1/32 px per frame).
     */
    void spawn(const data::BonusKind& kind, core::Fixed x, core::Fixed y, core::Fixed vx, core::Word lift);

    void update(Level& level);
    void changeState(const bonuses::BonusState& next, Level& level);

    core::Fixed x() const { return s_.x; }
    core::Fixed y() const { return s_.y; }
    core::Fixed speedX() const { return s_.timer.speedX(); }
    core::Word fallSpeed() const { return s_.vy; }
    void setFallSpeed(core::Word speed) { s_.vy = speed; }
    void moveToX(core::Fixed x) { s_.x = x; }
    void moveToY(core::Fixed y) { s_.y = y; }

    BonusTimer& timer() { return s_.timer; }

    /** Out of play: the slot is free. */
    void disappear() { s_.sprite = data::NO_SPRITE; }

    const Snapshot& snapshot() const { return s_; }
    void restore(const Snapshot& snapshot) { s_ = snapshot; }

private:
    int index_ = 0;
    Snapshot s_;
};

}  // namespace ugh::model
