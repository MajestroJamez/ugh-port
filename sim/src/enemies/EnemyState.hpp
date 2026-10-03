// A state of the enemies' state machines (State).
#pragma once

#include "data/Sprite.hpp"
#include "model/Enemy.hpp"
#include "model/Level.hpp"

namespace ugh::enemies {

/**
 * A state an enemy is in from one frame to the next; the original keeps the address of its handler (2cf3).
 * Like the passengers' states (passengers/PassengerState.hpp), the states are stateless singletons; an enemy
 * changes its state with Enemy::changeState or Enemy::continueIn. Each kind has its own states (flyer/, walker/,
 * blower/, tree/).
 *
 * A standing passenger dropped onto a flyer, a walker or a blower stuns it and scores; one dropped onto a tree
 * shakes a bonus item out of it.
 */
class EnemyState {
public:
    virtual ~EnemyState() = default;

    /** The name of the state in the golden replays. */
    virtual const char* name() const = 0;

    /** What the enemy does when it gets into the state (the original's handler that leads to it). */
    virtual void enter(model::Enemy& enemy, model::Level& level) const;

    /** One frame in the state. */
    virtual void update(model::Enemy& enemy, model::Level& level) const = 0;

protected:
    /** The sprite of a passenger that bounced off an enemy. */
    static constexpr data::Sprite HIT_PASSENGER_SPRITE = 0x222;

    /** 113b:2196 - a falling passenger close to the enemy bounces off it (showing the hit or not); true if one did. */
    static bool bounceFallingPassenger(model::Enemy& enemy, model::Level& level, bool showHit);

    /** Stunned by a passenger: the score of the kind. */
    static void scoreStun(const model::Enemy& enemy, model::Level& level);
};

}  // namespace ugh::enemies
