// The bonus item state Falling.
#pragma once

#include "bonuses/BonusState.hpp"
#include "core/Fixed.hpp"
#include "core/Word.hpp"
#include "data/BonusKind.hpp"

namespace ugh::bonuses {

/** Flies and falls until it lands on a pad or leaves the screen. */
class Falling : public BonusState {
public:
    static const Falling instance;

    /**
     * 113b:2b96 - Bonuses.kt bonusSpawn: drops a bonus item of `kind` with its middle / bottom at x, y, flying
     * sideways with `vx` and up with its kind's lift plus `lift` (1/32 px per frame). It takes the last free slot.
     */
    static void drop(model::Level& level, const data::BonusKind& kind, core::Fixed x, core::Fixed y, core::Fixed vx,
                     core::Word lift);

    const char* name() const override { return "Falling"; }
    void update(model::BonusItem& item, model::Level& level) const override;
};

}  // namespace ugh::bonuses
