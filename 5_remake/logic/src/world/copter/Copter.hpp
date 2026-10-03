// A copter.
#pragma once

#include "units/Fixed.hpp"
#include "world/copter/Cabin.hpp"
#include "world/copter/Controls.hpp"
#include "world/copter/CopterShape.hpp"
#include "world/copter/Motion.hpp"
#include "world/copter/Rotor.hpp"
#include "world/scenery/Pad.hpp"

namespace ugh::world::copter {

/**
 * A copter of player 0 or 1: where it is and how fast it goes (`Motion`), the pad it stands on, the keys of its pilot,
 * its rotor and its cabin. The physics flies it; passengers board it, a walker throws it, a blower pushes it. Its
 * shape (where its door, its skids, its body are): `CopterShape`.
 */
class Copter {
public:
    explicit Copter(int player = 0) : player_(player) {}

    /** Whose copter it is: the events, the rotor sprites and the replays name it so. */
    int player() const { return player_; }

    /** At the start of an attempt: in the air at x, y, empty, the rotor at its first sprite. */
    void placeAtStart(units::Fixed x, units::Fixed y, int firstRotorSprite);

    /** Where it is and how fast it goes. */
    Motion& motion() { return motion_; }
    const Motion& motion() const { return motion_; }
    /** The keys its pilot holds. */
    Controls& controls() { return controls_; }
    const Controls& controls() const { return controls_; }
    Rotor& rotor() { return rotor_; }
    const Rotor& rotor() const { return rotor_; }
    Cabin& cabin() { return cabin_; }
    const Cabin& cabin() const { return cabin_; }

    bool landed() const { return landedOn_ != nullptr; }
    bool landedOn(const scenery::Pad& pad) const { return landedOn_ == &pad; }
    /** The pad it stands on; nullptr in the air. */
    const scenery::Pad* landedPad() const { return landedOn_; }
    /** Standing on a pad: no speed. */
    void land(const scenery::Pad& pad);
    void takeOff() { landedOn_ = nullptr; }

    /** How deep its waterline is below the water surface at `waterRow` (pixels; 0: it floats, negative: above). */
    int depthIn(int waterRow) const { return CopterShape::depthAt(motion_.pixelY(), waterRow); }

    /** Where what it lets go starts to fall (`CopterShape::DROP`). */
    units::Fixed dropX() const { return motion_.x() + units::Fixed::fromPixels(CopterShape::DROP.x); }
    units::Fixed dropY() const { return motion_.y() + units::Fixed::fromPixels(CopterShape::DROP.y); }

    /**
     * A charging walker hits it: a pixel up and into the air with the walker's speed (sideways 32 times, up 16 times
     * as much). The pixel row stays where it was until the physics moves the copter again.
     */
    void throwUp(units::Fixed walkerSpeed);

private:
    int player_;
    Motion motion_;
    const scenery::Pad* landedOn_ = nullptr;
    Controls controls_;
    Rotor rotor_;
    Cabin cabin_;
};

}  // namespace ugh::world::copter
