// One frame of the play.
#pragma once

#include "data/GameData.hpp"
#include "input/Keyboard.hpp"
#include "model/Level.hpp"
#include "physics/CopterPhysics.hpp"

namespace ugh::game {

/**
 * 113b:0ca5 .. 0fa4 - Frame.kt frameBody / frameAfterKeys without the drawing: one frame of the play, the systems
 * in the order of the original:
 *
 *   water level, keys (Esc gives up), copter physics, passengers, enemies, bonus items,
 *   passenger pixels, rotors, rain, water surface
 */
class PlayFrame {
public:
    PlayFrame(const data::GameData& data, model::Level& level, input::Keyboard& keyboard);

    void run();

private:
    const data::GameData& data_;
    model::Level& level_;
    input::Keyboard& keyboard_;
    physics::CopterPhysics physics_;

    /** 113b:0fe8 - Frame.kt frameKeys (keyboard players only): Esc gives up the game, P would pause. */
    void readKeys();
    void flyCopters();
    void spinRotors();
};

}  // namespace ugh::game
