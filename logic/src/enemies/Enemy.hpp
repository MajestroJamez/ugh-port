// An enemy.
#pragma once

#include <optional>

#include "data/Animation.hpp"
#include "enemies/EnemyContext.hpp"
#include "enemies/EnemyVisitor.hpp"
#include "units/Fixed.hpp"
#include "units/Int16.hpp"
#include "world/Animator.hpp"

namespace ugh::enemies {

/**
 * An enemy: what all have - their place in the level's list, where they are, the sprite they show and their
 * animation. The flyer, the walker, the blower and the tree are its kinds, each with its own state machine.
 */
class Enemy {
public:
    explicit Enemy(int index) : index_(index) {}
    virtual ~Enemy() = default;
    Enemy(const Enemy&) = delete;
    Enemy& operator=(const Enemy&) = delete;

    /** Its place in the level's list: events and the replays name it so. */
    int index() const { return index_; }

    /** One frame in its state. */
    virtual void update(const EnemyContext& context) = 0;
    virtual void accept(EnemyVisitor& visitor) const = 0;

    units::Fixed x() const { return x_; }
    units::Fixed y() const { return y_; }
    std::optional<int> sprite() const { return sprite_; }
    const world::Animator& animator() const { return animator_; }

    /** Nothing of it is shown. */
    void hide() { sprite_.reset(); }

    // ------------------------------------------------------------ for the states

    void moveToX(units::Fixed x) { x_ = x; }
    void moveToY(units::Fixed y) { y_ = y; }
    void restartAnimation() { animator_.restart(); }
    /** One frame of the animation delay; true when the next frame is due. */
    bool animate(units::Int16 delay) { return animator_.step(delay); }
    /** Shows the frame of `animation`, from its start again after its end. */
    void show(const data::Animation& animation) { sprite_ = animator_.show(animation); }
    /** Shows the frame of an animation that runs once. */
    void showFrameOf(const data::Animation& animation) { sprite_ = animator_.frameOf(animation); }
    bool pastEndOf(const data::Animation& animation) const { return animator_.pastEndOf(animation); }
    void showSprite(int sprite) { sprite_ = sprite; }

    /**
     * A standing passenger falling onto the enemy bounces off it (back up as fast as it fell), shown hit when
     * `showHit`; true when one did.
     */
    bool bounceFallingPassenger(const EnemyContext& context, bool showHit) const;
    /** A passenger stunned it: its score. */
    void scoreStun(units::Int16 score, const EnemyContext& context) const;

protected:
    units::Fixed x_, y_;   // top left corner
    std::optional<int> sprite_;
    world::Animator animator_;

private:
    int index_;
};

}  // namespace ugh::enemies
