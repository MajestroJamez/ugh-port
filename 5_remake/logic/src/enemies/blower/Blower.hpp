// The blower.
#pragma once

#include "data/BlowerKind.hpp"
#include "data/BlowerPlacement.hpp"
#include "enemies/Enemy.hpp"
#include "enemies/blower/BlowerState.hpp"
#include "units/Countdown.hpp"

namespace ugh::enemies::blower {

/** The blower: it blows copters in front of it to and fro with its animation; a passenger dropped onto it stuns it. */
class Blower : public Enemy {
public:
    Blower(int index, const data::BlowerKind& kind, const data::BlowerPlacement& placement);

    void update(const EnemyContext& context) override;
    void accept(EnemyVisitor& visitor) const override;

    const data::BlowerKind& kind() const { return *kind_; }
    const BlowerState& state() const { return *state_; }

    /** Into `next` from the next frame on. */
    void changeState(const BlowerState& next, const EnemyContext& context);
    /** Into `next` and on in it in this frame. */
    void continueIn(const BlowerState& next, const EnemyContext& context);

    void startStunTime(units::Int16 frames) { stunTime_.start(frames); }
    bool stunTimeOver() { return stunTime_.tick(); }
    units::Int16 stunTime() const { return stunTime_.remaining(); }

private:
    const data::BlowerKind* kind_;
    const BlowerState* state_;
    units::Countdown stunTime_;
};

}  // namespace ugh::enemies::blower
