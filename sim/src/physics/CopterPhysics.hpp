// How a copter flies.
#pragma once

#include "core/Speed.hpp"
#include "core/Word.hpp"
#include "model/Copter.hpp"
#include "model/Level.hpp"

namespace ugh::physics {

/**
 * 113b:1095 - Game.kt copterUpdate: one frame of a copter's flight. The wind and the arrow keys push it sideways,
 * gravity pulls it down and the water lifts it up, the up key lifts it and the down key dives; it moves pixel by
 * pixel against the collision mask, bounces off what it hits, touches down on a pad when it comes down softly, and
 * crashes when it hits anything too hard. Flying costs energy.
 */
class CopterPhysics {
public:
    explicit CopterPhysics(model::Level& level) : level_(level) {}

    void fly(int player);

private:
    /** Where the copter is relative to the water surface (the high byte of the depth in the original). */
    enum class Depth { Above, Surface, Below };

    model::Level& level_;

    Depth depthOf(const model::Copter& copter) const;
    void blowWithWind(model::Copter& copter, Depth depth);
    void steer(model::Copter& copter);
    void moveHorizontally(model::Copter& copter);
    void liftAndFall(model::Copter& copter, Depth depth);
    void moveVertically(model::Copter& copter, Depth depth);
    void bounceVertically(model::Copter& copter, core::Fixed y);
    void touchDownOnPad(model::Copter& copter, core::Fixed y);
    void checkCrash(const model::Copter& copter, int player);

    /** 113b:1457 - Game.kt probe: a point of the copter's outline, around `origin`, hits the background. */
    bool probeHits(int origin) const;
    /** The probe origin of a copter: its pixel position, 5 px to the right, as a pixel index of the mask. */
    static int probeOrigin(const model::Copter& copter);
    /** A bounce: half the speed back; the impact is the speed it had (the doubled half, as the original computes). */
    static core::Word bounce(core::Speed& speed);
};

}  // namespace ugh::physics
