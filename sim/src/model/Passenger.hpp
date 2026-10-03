// A passenger (level list B).
#pragma once

#include "core/Audit.hpp"
#include "core/Fixed.hpp"
#include "core/Speed.hpp"
#include "core/Word.hpp"
#include "data/Animation.hpp"
#include "data/PassengerKind.hpp"
#include "data/PassengerPlacement.hpp"
#include "data/Route.hpp"
#include "data/Sprite.hpp"
#include "model/Animator.hpp"
#include "model/PassengerCounter.hpp"
#include "model/PassengerTimer.hpp"

namespace ugh::passengers {
class PassengerState;
}

namespace ugh::model {

class Copter;
class Level;

/**
 * A passenger: its kind, the state it is in (passengers/), where it is, where it goes, and what it shows. The states
 * drive it through the methods below; the passenger itself does not know what the states are.
 */
class Passenger {
public:
    struct Snapshot {
        const data::PassengerKind* kind = nullptr;   // changes to its swimming kind in the water and back
        const passengers::PassengerState* state = nullptr;
        core::Word startPad;
        data::RouteCursor route;
        int pickupPad = 0, targetPad = 0;
        PassengerCounter counter;
        PassengerTimer timer;
        core::Fixed x, y;           // top left
        core::Word pixelX, pixelY;  // x, y in whole pixels as of the end of the last frame; the states read these
        core::Word vy;              // the y speed: 1/32 px per frame when falling, 1/64 of that in the water (2c6d)
        Animator animator;
        data::Sprite sprite = data::NO_SPRITE;
        data::Sprite bubble = data::NO_SPRITE;   // the speech bubble over the passenger
        core::Word bonusTimer;      // a delivery before it runs out drops a bonus item
        const char* vyMeaning = "unset";
        bool bonusTimerSet = false;
    };

    Passenger() = default;
    /** The passenger in slot `index` of the level list (the events name it by the index). */
    explicit Passenger(int index) : index_(index) {}

    int index() const { return index_; }
    const data::PassengerKind& kind() const { return *s_.kind; }
    const passengers::PassengerState& state() const { return *s_.state; }

    // ------------------------------------------------------------ the state machine

    /** 113b:3976 - the level load puts a walking passenger at the start of its route, in `state`. */
    void placeWalking(const data::PassengerPlacement& placement, const passengers::PassengerState& state);
    /** 113b:3976 - the level load puts the standing passenger on its pad, in `state`. */
    void placeStanding(const data::PassengerPlacement& placement, const passengers::PassengerState& state);

    /** The passenger's turn in the frame: its state's update. */
    void update(Level& level);

    /**
     * Into `next` from the next frame on: the entry action of `next` runs now, its update in the next frame.
     * The original sets the address of the state's handler for the next frame.
     */
    void changeState(const passengers::PassengerState& next, Level& level);

    /**
     * Into `next` and on in it in this frame: the entry action of `next` and its update run now. The original jumps
     * to the state's handler (JMP [SI+n]).
     */
    void continueIn(const passengers::PassengerState& next, Level& level);

    /** The walking kind turns into its swimming kind and back. */
    void switchKind() { s_.kind = s_.kind->other; }

    // ------------------------------------------------------------ the route

    int pickupPad() const { return s_.pickupPad; }
    int targetPad() const { return s_.targetPad; }
    bool routeFinished() const { return s_.route.finished(); }

    /** 113b:149c - the next stop of the route: from where to where, after which delay. */
    void startStop();

    /** The stop is done; the next one starts. */
    void advanceRoute() { s_.route.stop++; }

    PassengerCounter& counter() { return s_.counter; }
    PassengerTimer& timer() { return s_.timer; }
    const PassengerTimer& timer() const { return s_.timer; }

    // ------------------------------------------------------------ where it is

    core::Fixed x() const { return s_.x; }
    core::Fixed y() const { return s_.y; }
    core::Word pixelX() const { return s_.pixelX; }
    core::Word pixelY() const { return s_.pixelY; }

    /** The x of its feet (the middle of the sprite), in pixels as of the last frame. */
    core::Word feetX() const { return s_.pixelX + s_.kind->box.x; }

    void moveTo(core::Fixed x, core::Fixed y);
    void moveToX(core::Fixed x) { s_.x = x; }
    void moveToY(core::Fixed y) { s_.y = y; }

    /** Out of the door of the pickup pad: its feet at the door, on the pad. */
    void standAtDoor(core::Word doorX, core::Word padY);

    /** One pixel to the left (-1) or to the right (1). */
    void stepBy(int pixels) { s_.x += core::Fixed::fromPixels(pixels); }

    /** A step towards the copter (or a swim stroke): true when the passenger is level with it. */
    bool walkTowards(const Copter& copter);

    /** A swimmer stays on the surface when it moved away from the passenger. */
    void floatOnSurface(core::Word waterRow);

    /** Frame.kt frameAfterKeys (the drawing): the pixel position the states read next frame. */
    void updatePixels();

    // ------------------------------------------------------------ falling and swimming

    /** The speed of the dropped standing passenger: 1/32 px per frame. */
    core::Word fallSpeed() const { vyReading("fall"); return s_.vy; }
    void setFallSpeed(core::Word speed) { s_.vy = speed; s_.vyMeaning = "fall"; }

    /** The speed of a swimmer going under and up: 1/64 Fixed per frame. */
    core::Speed swimSpeed() const { vyReading("swim"); return core::Speed(s_.vy); }
    void setSwimSpeed(core::Speed speed) { s_.vy = speed.raw(); s_.vyMeaning = "swim"; }
    void vyReading(const char* meaning) const;

    /** A standing passenger dropped from a copter on its way down (113b:2196: descriptor 78dc in its falling state). */
    bool fallingDown() const;

    // ------------------------------------------------------------ what it shows

    /** The animation from its start. */
    void restartAnimation() { s_.animator.restart(); }
    /** One frame of the animation delay of its kind; true when the next frame is due. */
    bool animate() { return s_.animator.step(s_.kind->animDelay); }
    /** Back to frame 0 at once. */
    void rewindAnimation() { s_.animator.rewind(); }
    /** Shows the current frame of `animation`, from its start again past the end. */
    void show(const data::Animation& animation) { s_.sprite = s_.animator.show(animation); }
    /** Shows the current frame of `animation` as it is (the door animations run once). */
    void showFrameOf(const data::Animation& animation) { s_.sprite = s_.animator.frameOf(animation); }
    bool pastEndOf(const data::Animation& animation) const { return s_.animator.pastEndOf(animation); }
    bool atLastFrameOf(const data::Animation& animation) const { return s_.animator.atLastFrameOf(animation); }

    void showSprite(data::Sprite sprite) { s_.sprite = sprite; }
    void hide() { s_.sprite = data::NO_SPRITE; }
    bool hidden() const { return s_.sprite == data::NO_SPRITE; }

    void showBubble(data::Sprite bubble) { s_.bubble = bubble; }
    void hideBubble() { s_.bubble = data::NO_SPRITE; }

    // ------------------------------------------------------------ the quick delivery

    /** 113b:19fb - the time a delivery earns a bonus item starts when the passenger boards. */
    void startQuickDeliveryTime();
    /** 113b:1a42 - one frame of the ride. */
    void tickQuickDeliveryTime();
    bool deliveredQuickly() const {
        if (!s_.bonusTimerSet) core::audit::count("Q2 Passenger.bonusTimer read unset");
        return s_.bonusTimer != 0;
    }

    const Snapshot& snapshot() const { return s_; }
    void restore(const Snapshot& snapshot) { s_ = snapshot; }

private:
    int index_ = 0;
    Snapshot s_;
};

}  // namespace ugh::model
