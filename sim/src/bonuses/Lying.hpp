// The bonus item state Lying.
#pragma once

#include "bonuses/BonusState.hpp"

namespace ugh::bonuses {

/** Lies on a pad for a while; a copter touching it collects it. */
class Lying : public BonusState {
public:
    static const Lying instance;

    const char* name() const override { return "Lying"; }
    void enter(model::BonusItem& item, model::Level& level) const override;
    void update(model::BonusItem& item, model::Level& level) const override;
};

}  // namespace ugh::bonuses
