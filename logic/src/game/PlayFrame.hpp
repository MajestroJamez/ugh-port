// One frame of the play.
#pragma once

#include "bonuses/BonusSlots.hpp"
#include "enemies/Enemies.hpp"
#include "input/PcKeyboard.hpp"
#include "passengers/Passengers.hpp"
#include "world/PlayContext.hpp"

namespace ugh::game {

/**
 * One frame of the play, the systems in the order of the original:
 *
 *   the water, the keys (Esc gives up), the copters (unless the level is still fading in), the passengers, the
 *   enemies, the bonus items, the seen positions of the passengers, the rotors, the rain, the water surface (and the
 *   rain's floor row)
 */
class PlayFrame {
public:
    PlayFrame(const world::PlayContext& context, input::PcKeyboard& keyboard, passengers::Passengers& passengers,
              enemies::Enemies& enemies, bonuses::BonusSlots& bonuses)
        : context_(context), keyboard_(keyboard), passengers_(passengers), enemies_(enemies), bonuses_(bonuses) {}

    void run();

private:
    static constexpr uint8_t SCANCODE_ESC = 0x01, SCANCODE_P = 0x19;

    const world::PlayContext& context_;
    input::PcKeyboard& keyboard_;
    passengers::Passengers& passengers_;
    enemies::Enemies& enemies_;
    bonuses::BonusSlots& bonuses_;

    /** The game loop looks at the last scancode: Esc gives the game up, P would pause. */
    void readKeys();
    void flyCopters();
    void spinRotors();
};

}  // namespace ugh::game
