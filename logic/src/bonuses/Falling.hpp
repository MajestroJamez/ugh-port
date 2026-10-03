// A bonus item falls.
#pragma once

#include "bonuses/BonusState.hpp"

namespace ugh::bonuses {

/** Thrown up and sideways, it falls until it lands on a pad or leaves the screen. */
class Falling : public BonusState {
public:
    static const Falling instance;

    const char* name() const override { return "Falling"; }
    void update(BonusItem& item, const world::PlayContext& context) const override;
};

}  // namespace ugh::bonuses
