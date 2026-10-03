// One frame of the play.
#pragma once

#include "bonuses/BonusSlots.hpp"
#include "enemies/Enemies.hpp"
#include "input/MenuInput.hpp"
#include "passengers/Passengers.hpp"
#include "world/PlayContext.hpp"

namespace ugh::game {

/**
 * One frame of the play, the systems in the order of the original:
 *
 *   the fade, the water, the keys (Esc gives up), the copters (unless the level is still fading in), the passengers,
 *   the enemies, the bonus items, the seen positions of the passengers, the rotors, the rain, the water surface (and
 *   the rain's floor row)
 */
class PlayFrame {
public:
    PlayFrame(const world::PlayContext& context, const input::MenuInput& menu, passengers::Passengers& passengers,
              enemies::Enemies& enemies, bonuses::BonusSlots& bonuses)
        : context_(context), menu_(menu), passengers_(passengers), enemies_(enemies), bonuses_(bonuses) {}

    void run();

private:
    const world::PlayContext& context_;
    const input::MenuInput& menu_;
    passengers::Passengers& passengers_;
    enemies::Enemies& enemies_;
    bonuses::BonusSlots& bonuses_;

    /** The game loop looks at the last key: Esc gives the game up, P would pause. */
    void readKeys();
    void flyCopters();
    void spinRotors();
};

}  // namespace ugh::game
