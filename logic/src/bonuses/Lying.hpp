// A bonus item lies on a pad.
#pragma once

#include "bonuses/BonusState.hpp"

namespace ugh::bonuses {

/** It lies on a pad; a copter that touches it collects it: energy, a life or a higher score multiplier. */
class Lying : public BonusState {
public:
    static const Lying instance;

    /** The item landed: it lies for a while. */
    static void start(BonusItem& item);

    const char* name() const override { return "Lying"; }
    void update(BonusItem& item, const world::PlayContext& context) const override;
};

}  // namespace ugh::bonuses
