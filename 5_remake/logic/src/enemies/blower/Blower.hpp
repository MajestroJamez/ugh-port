// The blower.
#pragma once

#include "data/kinds/BlowerKind.hpp"
#include "data/levels/BlowerPlacement.hpp"
#include "enemies/Enemy.hpp"
#include "enemies/Stun.hpp"
#include "enemies/blower/BlowerState.hpp"
#include "state/StateMachine.hpp"

namespace ugh::enemies::blower {

/** The blower: it blows copters in front of it to and fro with its animation; a passenger dropped onto it stuns it. */
class Blower : public Enemy, public state::StateMachine<Blower, EnemyContext> {
public:
    Blower(int index, const data::kinds::BlowerKind& kind, const data::levels::BlowerPlacement& placement);

    void update(const EnemyContext& context) override;
    void accept(EnemyVisitor& visitor) const override;

    const data::kinds::BlowerKind& kind() const { return *kind_; }

    /** How long it stays stunned. */
    Stun& stun() { return stun_; }
    const Stun& stun() const { return stun_; }

private:
    const data::kinds::BlowerKind* kind_;
    Stun stun_;
};

}  // namespace ugh::enemies::blower
