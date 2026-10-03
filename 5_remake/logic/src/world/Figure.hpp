// What an entity of the play shows.
#pragma once

#include <optional>

#include "data/Animation.hpp"
#include "units/Fixed.hpp"
#include "units/Int16.hpp"
#include "world/Animator.hpp"

namespace ugh::world {

/**
 * What a passenger or an enemy shows: where it is (its top left corner), the sprite it shows and where it is in its
 * animation. The passengers and the enemies derive from it; their states move it and pick its frames.
 */
class Figure {
public:
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
    bool animate(units::Int16 delay) { return animator_.step(delay); }
    /** Shows the frame of `animation`, from its start again after its end. */
    void show(const data::Animation& animation) { sprite_ = animator_.show(animation); }
    /** Shows the frame of `animation` that runs once. */
    void showFrameOf(const data::Animation& animation) { sprite_ = animator_.frameOf(animation); }
    bool pastEndOf(const data::Animation& animation) const { return animator_.pastEndOf(animation); }
    bool atLastFrameOf(const data::Animation& animation) const { return animator_.atLastFrameOf(animation); }

protected:
    Figure() = default;
    Figure(units::Fixed x, units::Fixed y) : x_(x), y_(y) {}

private:
    units::Fixed x_, y_;
    std::optional<int> sprite_;
    Animator animator_;
};

}  // namespace ugh::world
