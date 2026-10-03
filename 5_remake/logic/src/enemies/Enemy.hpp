// An enemy.
#pragma once

#include "data/kinds/Facing.hpp"
#include "enemies/EnemyContext.hpp"
#include "enemies/EnemyVisitor.hpp"
#include "passengers/standing/StandingPassenger.hpp"
#include "units/Fixed.hpp"
#include "world/figure/Figure.hpp"

namespace ugh::enemies {

/**
 * An enemy: what all have - what they show (`world::figure::Figure`, with their place in the level's list). The
 * flyer, the walker, the blower and the tree are its kinds, each with its own state machine.
 */
class Enemy : public world::figure::Figure {
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

    /** How fast a standing passenger bounces back up off it: as fast as it fell, or half as fast (the tree). */
    enum class Rebound { Full, Half };

    /** A standing passenger falling onto the enemy bounces off it, shown hit; the passenger, nullptr if none. */
    passengers::standing::StandingPassenger* bounceFallingPassenger(const EnemyContext& context, Rebound rebound) const;
    /** A falling standing passenger bounces off as fast as it fell and shows no hit (a blower); true when one did. */
    bool bounceFallingPassengerUnseen(const EnemyContext& context) const;
    /** A passenger stunned it: its score. */
    void scoreStun(int score, const EnemyContext& context) const;

    /** A speed turned to point towards `side` (its size stays). */
    static units::Fixed headed(units::Fixed speed, data::kinds::Facing side) {
        bool left = speed < units::Fixed();
        return (side == data::kinds::Facing::Left) == left ? speed : -speed;
    }
};

}  // namespace ugh::enemies
