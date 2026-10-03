// The blower's state Stunned.
#pragma once

#include "enemies/blower/BlowerState.hpp"
#include "units/Int16.hpp"

namespace ugh::enemies::blower {

/** A passenger fell onto it: stunned for a while. */
class Stunned : public BlowerState {
public:
    static const Stunned instance;
    static constexpr units::Int16 STUN_TIME = 350;

    const char* name() const override { return "Stunned"; }
    void enter(Blower& blower, const EnemyContext& context) const override;
    void update(Blower& blower, const EnemyContext& context) const override;
};

}  // namespace ugh::enemies::blower
