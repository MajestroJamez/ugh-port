// The blower.
#pragma once

#include "data/BlowerKind.hpp"
#include "data/BlowerPlacement.hpp"
#include "enemies/Enemy.hpp"
#include "enemies/blower/BlowerState.hpp"
#include "state/StateMachine.hpp"
#include "units/Countdown.hpp"

namespace ugh::enemies::blower {

/** The blower: it blows copters in front of it to and fro with its animation; a passenger dropped onto it stuns it. */
class Blower : public Enemy, public state::StateMachine<Blower, EnemyContext> {
public:
    Blower(int index, const data::BlowerKind& kind, const data::BlowerPlacement& placement);

    void update(const EnemyContext& context) override;
    void accept(EnemyVisitor& visitor) const override;

    const data::BlowerKind& kind() const { return *kind_; }


    void startStunTime(units::Int16 frames) { stunTime_.start(frames); }
    bool stunTimeOver() { return stunTime_.tick(); }
    units::Int16 stunTime() const { return stunTime_.remaining(); }

private:
    const data::BlowerKind* kind_;
    units::Countdown stunTime_;
};

}  // namespace ugh::enemies::blower
