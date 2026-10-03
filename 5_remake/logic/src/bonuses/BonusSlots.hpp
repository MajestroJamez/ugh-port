// The places of the bonus items.
#pragma once

#include <array>
#include <optional>

#include "bonuses/BonusItem.hpp"
#include "world/PlayContext.hpp"

namespace ugh::bonuses {

/** The places of the bonus items (SLOTS); a dropped item takes a free one, the last first. */
class BonusSlots {
public:
    static constexpr int SLOTS = 12;

    /**
     * A new item of `kind` with its anchor column and half its anchor row at x, y (BonusItem), thrown sideways with
     * `speedX` and up with its kind's lift and `lift` more (Fixed per frame). With all slots in use the original jumps
     * into nowhere: diagnostics.
     */
    void drop(const data::kinds::BonusKind& kind, units::Fixed x, units::Fixed y, units::Fixed speedX, units::Fixed lift,
              events::Diagnostics& diagnostics);

    /** Every item, the last slot first. */
    void update(const world::PlayContext& context);

    /** All slots free (before the play of an attempt). */
    void clear();

    const std::optional<BonusItem>& operator[](int slot) const { return items_[slot]; }

private:
    std::array<std::optional<BonusItem>, SLOTS> items_;
};

}  // namespace ugh::bonuses
