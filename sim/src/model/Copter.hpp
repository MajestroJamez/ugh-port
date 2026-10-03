// A copter (player 0 or 1).
#pragma once

#include "core/Fixed.hpp"
#include "core/Speed.hpp"
#include "core/Word.hpp"
#include "data/Sprite.hpp"
#include "model/Controls.hpp"

namespace ugh::model {

/**
 * A copter: where it is and how fast it goes, whom it carries and for how much, its rotor and the keys of its pilot.
 * The physics (physics/CopterPhysics) flies it; passengers board it, a walker throws it, a blower pushes it.
 */
class Copter {
public:
    static constexpr int IN_THE_AIR = -1;
    static constexpr core::Word WATERLINE = 0x12;   // from the top of the copter, in pixels

    struct Snapshot {
        core::Fixed x, y;           // top left
        core::Word pixelX, pixelY;  // x, y in whole pixels: the physics updates them with x, y, but a walker that
                                    //   throws the copter moves only y, so pixelY is behind until the next frame
        core::Speed vx, vy;
        int landedPad = IN_THE_AIR; // the pad the copter stands on
        core::Word effort;          // pedalling this frame (makes the rotor spin faster)
        core::Word impact;          // the hardest bounce this frame; crashes over the crash limit
        core::Word carrying;        // the look of the passenger on board (PassengerKind::look), 0 = nobody
        core::Word targetPad;       // the number of the pad the passenger wants to go to, 0 = none
        core::Word fare;            // what the passenger pays now, drops every frame to
        core::Word fareMin;         //   this minimum
        data::Sprite rotor = 0;     // the rotor animation: the current sprite
        core::Word rotorCounter;    //   and the countdown to the next one
        Controls keys;
    };

    // ------------------------------------------------------------ the level

    /** 113b:3976 - at the start of a level: in the air at x, y, empty, the rotor at its first sprite. */
    void placeAtStart(core::Fixed x, core::Fixed y, data::Sprite rotor);

    Controls& controls() { return s_.keys; }
    const Controls& controls() const { return s_.keys; }

    // ------------------------------------------------------------ where it is

    core::Fixed x() const { return s_.x; }
    core::Fixed y() const { return s_.y; }
    core::Word pixelX() const { return s_.pixelX; }
    core::Word pixelY() const { return s_.pixelY; }
    core::Speed speedX() const { return s_.vx; }
    core::Speed speedY() const { return s_.vy; }
    bool landed() const { return s_.landedPad != IN_THE_AIR; }
    bool landedOn(int pad) const { return s_.landedPad == pad; }
    /** It does not move up or down (a swimmer swims only to a copter that floats still). */
    bool stillVertically() const { return s_.vy == core::Speed(0); }

    /** How deep its waterline (WATERLINE px below its top) is under the surface at `waterRow`: 0 = it floats. */
    core::Word depthIn(core::Word waterRow) const { return s_.pixelY - waterRow + WATERLINE; }

    // ------------------------------------------------------------ the physics

    /** A new frame of the physics: no effort, no impact yet. */
    void startFrame();
    void addEffort(core::Word effort) { s_.effort += effort; }
    void setSpeed(core::Speed vx, core::Speed vy);
    /** To x, and its whole pixel. */
    void moveToX(core::Fixed x);
    /** To y, and its whole pixel. */
    void moveToY(core::Fixed y);
    void setImpact(core::Word impact) { s_.impact = impact; }
    core::Word impact() const { return s_.impact; }
    core::Word effort() const { return s_.effort; }
    /** Standing on a pad: no speed. */
    void land(int pad);
    void takeOff() { s_.landedPad = IN_THE_AIR; }

    // ------------------------------------------------------------ the others

    /** 113b:2748 - a charging walker hits it: up a pixel and into the air with the walker's speed. */
    void throwUp(core::Word speed);

    /** 113b:2973 - a blower pushes it sideways. */
    void push(core::Speed speedX) { s_.vx += speedX; }

    /** 113b:418d - Draw.kt drawCopter (without the drawing): the rotor spins faster with the effort. */
    void spinRotor(data::Sprite first, data::Sprite end);

    // ------------------------------------------------------------ the passenger on board

    bool hasRoom() const { return s_.carrying == 0; }
    core::Word fare() const { return s_.fare; }

    /** 113b:19fb - a passenger boards: who it is (its look), where it goes (the pad's number) and its fare. */
    void takeOnBoard(core::Word look, core::Word targetNumber, core::Word fare, core::Word fareMin);

    /** 113b:1c48 - picks up the standing passenger, who hangs below and can be dropped anywhere. */
    void pickUpHanging(core::Word look);

    /** 113b:1a42 - the fare drops by one each frame of the ride, down to its minimum (compared unsigned). */
    void lowerFare();

    /** The passenger got off. */
    void unload();

    const Snapshot& snapshot() const { return s_; }
    void restore(const Snapshot& snapshot) { s_ = snapshot; }

private:
    Snapshot s_;
};

}  // namespace ugh::model
