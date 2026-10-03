// An enemy.
#pragma once

#include "enemies/EnemyContext.hpp"
#include "enemies/EnemyVisitor.hpp"
#include "units/Fixed.hpp"
#include "world/Facing.hpp"
#include "world/Figure.hpp"

namespace ugh::enemies {

/**
 * An enemy: what all have - their place in the level's list and what they show (`world::Figure`). The flyer, the
 * walker, the blower and the tree are its kinds, each with its own state machine.
 */
class Enemy : public world::Figure {
public:
    explicit Enemy(int index) : index_(index) {}
    Enemy(int index, units::Fixed x, units::Fixed y) : Figure(x, y), index_(index) {}
    virtual ~Enemy() = default;
    Enemy(const Enemy&) = delete;
    Enemy& operator=(const Enemy&) = delete;

    /** Its place in the level's list: events and the replays name it so. */
    int index() const { return index_; }

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

private:
    int index_ = 0;
};

}  // namespace ugh::enemies
