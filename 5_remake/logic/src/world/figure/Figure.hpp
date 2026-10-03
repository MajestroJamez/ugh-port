// What an entity of the play shows.
#pragma once

#include <optional>

#include "data/kinds/Animation.hpp"
#include "units/Fixed.hpp"
#include "world/figure/Animator.hpp"

namespace ugh::world::figure {

/**
 * An entity of the play as it is shown: its place in its list, where it is (its top left corner), the sprite it shows
 * and where it is in its animation. The passengers, the enemies and the bonus items derive from it; their states move
 * it and pick its frames.
 */
class Figure {
public:
    /** Its place in its list (of the passengers, the enemies, the bonus slots): events and the replays name it so. */
    int index() const { return index_; }
    units::Fixed x() const { return x_; }
    units::Fixed y() const { return y_; }
    std::optional<int> sprite() const { return sprite_; }
    const Animator& animator() const { return animator_; }

    void moveTo(units::Fixed x, units::Fixed y) {
        x_ = x;
        y_ = y;
    }
    void moveToX(units::Fixed x) { x_ = x; }
    void moveToY(units::Fixed y) { y_ = y; }

    /** Nothing of it is shown. */
    void hide() { sprite_.reset(); }
    void showSprite(int sprite) { sprite_ = sprite; }

    void restartAnimation() { animator_.restart(); }
    void rewindAnimation() { animator_.rewind(); }
    /** One frame of the animation delay; true when the next frame is due. */
    bool animate(int delay) { return animator_.step(delay); }
    /** Shows the frame of `animation`, from its start again after its end. */
    void show(const data::kinds::Animation& animation) { sprite_ = animator_.show(animation); }
    /** Shows the frame of `animation` that runs once. */
    void showFrameOf(const data::kinds::Animation& animation) { sprite_ = animator_.frameOf(animation); }
    bool pastEndOf(const data::kinds::Animation& animation) const { return animator_.pastEndOf(animation); }
    bool atLastFrameOf(const data::kinds::Animation& animation) const { return animator_.atLastFrameOf(animation); }

protected:
    explicit Figure(int index) : index_(index) {}
    Figure(int index, units::Fixed x, units::Fixed y) : index_(index), x_(x), y_(y) {}

private:
    int index_;
    units::Fixed x_, y_;
    std::optional<int> sprite_;
    Animator animator_;
};

}  // namespace ugh::world::figure
