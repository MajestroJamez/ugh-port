// One frame of the play.
#pragma once

#include "input/PcKeyboard.hpp"
#include "world/PlayContext.hpp"

namespace ugh::game {

/**
 * One frame of the play, the systems in the order of the original:
 *
 *   the water, the keys (Esc gives up), the copters (unless the level is still fading in), the seen positions of the
 *   passengers, the rotors, the rain, the water surface (and the rain's floor row)
 */
class PlayFrame {
public:
    PlayFrame(const world::PlayContext& context, input::PcKeyboard& keyboard) : context_(context), keyboard_(keyboard) {}

    void run();

private:
    static constexpr uint8_t SCANCODE_ESC = 0x01, SCANCODE_P = 0x19;

    const world::PlayContext& context_;
    input::PcKeyboard& keyboard_;

    /** The game loop looks at the last scancode: Esc gives the game up, P would pause. */
    void readKeys();
    void flyCopters();
    void spinRotors();
};

}  // namespace ugh::game
