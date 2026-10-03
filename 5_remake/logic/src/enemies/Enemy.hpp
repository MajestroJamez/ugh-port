// An enemy.
#pragma once

#include "enemies/EnemyContext.hpp"
#include "enemies/EnemyVisitor.hpp"
#include "units/Fixed.hpp"
#include "world/Facing.hpp"
#include "world/Figure.hpp"

namespace ugh::enemies {

/**
 * An enemy: what all have - what they show (`world::Figure`, with their place in the level's list). The flyer, the
 * walker, the blower and the tree are its kinds, each with its own state machine.
 */
class Enemy : public world::Figure {
public:
    explicit Enemy(int index) : Figure(index) {}
    Enemy(int index, units::Fixed x, units::Fixed y) : Figure(index, x, y) {}
    virtual ~Enemy() = default;
    Enemy(const Enemy&) = delete;
    Enemy& operator=(const Enemy&) = delete;

    /** One frame in its state. */
    virtual void update(const EnemyContext& context) = 0;
    virtual void accept(EnemyVisitor& visitor) const = 0;

    // ------------------------------------------------------------ for the states

    /**
     * A standing passenger falling onto the enemy bounces off it (back up as fast as it fell), shown hit when
     * `showHit`; true when one did.
     */
    bool bounceFallingPassenger(const EnemyContext& context, bool showHit) const;
    /** A passenger stunned it: its score. */
    void scoreStun(int score, const EnemyContext& context) const;

    /** A speed turned to point towards `side` (its size stays). */
    static units::Fixed headed(units::Fixed speed, world::Facing side) {
        bool left = speed < units::Fixed();
        return (side == world::Facing::Left) == left ? speed : -speed;
    }
};

}  // namespace ugh::enemies
