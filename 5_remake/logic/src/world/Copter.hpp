// A copter.
#pragma once

#include <optional>

#include "units/Fixed.hpp"
#include "units/Speed.hpp"
#include "world/Cargo.hpp"
#include "world/Controls.hpp"

namespace ugh::world {

/**
 * A copter (player 0 or 1): where it is, how fast it goes, the pad it stands on, whom it carries for how much, its
 * rotor and the keys of its pilot. The physics flies it; passengers board it, a walker throws it, a blower pushes it.
 */
class Copter {
public:
    /** From the top of the copter to its waterline, in pixels. */
    static constexpr int WATERLINE = 18;

    /** At the start of an attempt: in the air at x, y, empty, the rotor at its first sprite. */
    void placeAtStart(units::Fixed x, units::Fixed y, int firstRotorSprite);

    /** The keys its pilot holds. */
    const Controls& controls() const { return controls_; }
    /** The pilot pressed (true) or released a key. */
    void setKey(data::PlayerKey key, bool held) { controls_.set(key, held); }
    /** A new attempt: no key is held. */
    void releaseKeys() { controls_ = Controls{}; }

    // ------------------------------------------------------------ where it is

    units::Fixed x() const { return x_; }
    units::Fixed y() const { return y_; }
    /**
     * The position in whole pixels, as the collision probe sees it: it follows x and y, except that a copter thrown
     * by a walker keeps its pixel row for a frame.
     */
    int pixelX() const { return pixelX_; }
    int pixelY() const { return pixelY_; }
    units::Speed speedX() const { return vx_; }
    units::Speed speedY() const { return vy_; }
    std::optional<int> landedPad() const { return landedPad_; }
    bool landed() const { return landedPad_.has_value(); }
    bool landedOn(int pad) const { return landedPad_ == pad; }
    /** It moves neither up nor down. */
    bool stillVertically() const { return vy_ == units::Speed(); }
    /** How deep its waterline is below the water surface at `waterRow` (pixels; 0: it floats, negative: above). */
    int depthIn(int waterRow) const { return pixelY_ - waterRow + WATERLINE; }

    // ------------------------------------------------------------ the physics

    /** A new frame of the physics: no effort yet. */
    void startFrame() { effort_ = 0; }
    void addEffort(int effort) { effort_ += effort; }
    /** How hard the pilot works the rotor this frame; it lasts until the copter flies again. */
    int effort() const { return effort_; }
    void setSpeed(units::Speed vx, units::Speed vy);
    /** To x, and its pixel column. */
    void moveToX(units::Fixed x);
    /** To y, and its pixel row. */
    void moveToY(units::Fixed y);
    /** Standing on a pad: no speed. */
    void land(int pad);
    void takeOff() { landedPad_.reset(); }

    // ------------------------------------------------------------ the others

    /**
     * A charging walker hits it: a pixel up and into the air with the walker's speed (sideways 32 times, up 16 times
     * as much). The pixel row stays where it was until the physics moves the copter again.
     */
    void throwUp(int walkerSpeed);

    /** A blower pushes it sideways. */
    void push(units::Speed speedX) { vx_ += speedX; }

    /** The rotor turns faster with more effort: the next of its sprites (first .. last) when its counter runs out. */
    void spinRotor(int firstSprite, int lastSprite);
    int rotorSprite() const { return rotorSprite_; }
    int rotorCounter() const { return rotorCounter_; }

    // ------------------------------------------------------------ the cargo

    bool hasRoom() const { return !cargo_.has_value(); }
    const std::optional<Cargo>& cargo() const { return cargo_; }
    int fare() const { return fare_; }

    /** A passenger of a route boards: who it is, the number of its destination pad, its fare and minimum fare. */
    void takeOnBoard(int look, int destination, int fare, int fareMin);
    /** The standing passenger hangs below. */
    void pickUpHanging(int look);
    /** One frame of the ride: the fare drops by one down to its minimum (compared unsigned). */
    void lowerFare();
    /** The passenger got out or was let go. */
    void unload() { cargo_.reset(); }

    // ------------------------------------------------------------ the test pilot of the replays

    /** Puts the copter anywhere (Cheats only). */
    void placeByTestPilot(units::Fixed x, units::Fixed y, int pixelX, int pixelY, units::Speed vx,
                          units::Speed vy, std::optional<int> landedPad);

private:
    units::Fixed x_, y_;
    int pixelX_ = 0, pixelY_ = 0;
    units::Speed vx_, vy_;
    std::optional<int> landedPad_;
    int effort_ = 0;
    int rotorSprite_ = 0;
    int rotorCounter_ = 0;
    std::optional<Cargo> cargo_;
    int fare_ = 0;
    Controls controls_;
};

}  // namespace ugh::world
