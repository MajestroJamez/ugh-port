// A copter.
#pragma once

#include <optional>

#include "units/Fixed.hpp"
#include "units/Speed.hpp"
#include "world/Cabin.hpp"
#include "world/Controls.hpp"
#include "world/Motion.hpp"
#include "world/Pad.hpp"
#include "world/Rotor.hpp"

namespace ugh::testing {
class TestPilot;   // the test pilot of the replays (5_remake/logic/testing)
}

namespace ugh::world {

/**
 * A copter of player 0 or 1: where it is and how fast it goes (`Motion`), the pad it stands on, the keys of its pilot,
 * its rotor and its cabin. The physics flies it; passengers board it, a walker throws it, a blower pushes it.
 */
class Copter : public Motion {
public:
    /** From the top of the copter to its waterline, in pixels. */
    static constexpr int WATERLINE = 18;

    explicit Copter(int player = 0) : player_(player) {}

    /** Whose copter it is: the events, the rotor sprites and the replays name it so. */
    int player() const { return player_; }

    /** At the start of an attempt: in the air at x, y, empty, the rotor at its first sprite. */
    void placeAtStart(units::Fixed x, units::Fixed y, int firstRotorSprite);

    /** The keys its pilot holds. */
    Controls& controls() { return controls_; }
    const Controls& controls() const { return controls_; }
    Rotor& rotor() { return rotor_; }
    const Rotor& rotor() const { return rotor_; }
    Cabin& cabin() { return cabin_; }
    const Cabin& cabin() const { return cabin_; }

    bool landed() const { return landedPad_.has_value(); }
    bool landedOn(const Pad& pad) const { return landedPad_ == pad.index(); }
    /** The index of the pad it stands on. */
    std::optional<int> landedPad() const { return landedPad_; }
    /** Standing on a pad: no speed. */
    void land(const Pad& pad);
    void takeOff() { landedPad_.reset(); }

    /** How deep its waterline is below the water surface at `waterRow` (pixels; 0: it floats, negative: above). */
    int depthIn(int waterRow) const { return pixelY() - waterRow + WATERLINE; }

    /**
     * A charging walker hits it: a pixel up and into the air with the walker's speed (sideways 32 times, up 16 times
     * as much). The pixel row stays where it was until the physics moves the copter again.
     */
    void throwUp(int walkerSpeed);

private:
    friend class testing::TestPilot;   // it puts a copter onto a pad

    int player_;
    std::optional<int> landedPad_;
    Controls controls_;
    Rotor rotor_;
    Cabin cabin_;
};

}  // namespace ugh::world
