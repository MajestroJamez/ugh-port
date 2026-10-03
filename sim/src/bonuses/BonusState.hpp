// A state of the bonus items' state machine (State).
#pragma once

#include "model/BonusItem.hpp"
#include "model/Level.hpp"

namespace ugh::bonuses {

/**
 * A state of a bonus item; the original keeps the address of its handler (2d83). A dropped item falls onto a pad
 * and lies there until a copter collects it or its time is up. Like the passengers' states, the states are
 * stateless singletons.
 */
class BonusState {
public:
    virtual ~BonusState() = default;

    /** The name of the state in the golden replays. */
    virtual const char* name() const = 0;

    virtual void enter(model::BonusItem& item, model::Level& level) const;
    virtual void update(model::BonusItem& item, model::Level& level) const = 0;
};

}  // namespace ugh::bonuses
