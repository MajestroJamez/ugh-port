// The walker's state Stunned.
#pragma once

#include "enemies/walker/WalkerState.hpp"

namespace ugh::enemies::walker {

/** A passenger fell onto it: stunned for a while. */
class Stunned : public WalkerState {
public:
    static const Stunned instance;
    static constexpr int STUN_TIME = 350;

    const char* name() const override { return "Stunned"; }
    void enter(Walker& walker, const EnemyContext& context) const override;
    void update(Walker& walker, const EnemyContext& context) const override;
};

}  // namespace ugh::enemies::walker
