// The rain of the windy levels.
#pragma once

#include <array>

#include "data/levels/Wind.hpp"
#include "events/Diagnostics.hpp"
#include "world/scenery/Raindrop.hpp"
#include "world/session/RandomNumbers.hpp"

namespace ugh::world::scenery {

/**
 * The rain of the windy levels: 193 drops fall diagonally with the wind, the even ones 3 px a frame, the odd ones
 * 2 px; a drop that reaches the floor row starts again at a random place, which uses up random numbers.
 *
 * The floor row is the water surface as it was last shown. It lasts from one attempt to the next, so the rain that
 * falls before a windy level starts (577 frames) stops at the water of the level before; a new game takes it from the
 * screens before the game (NewGameSettings).
 */
class Rain {
public:
    static constexpr int DROPS = 193;

    /** A windy level starts with the rain already falling. */
    void start(int waterRow, data::levels::Wind wind, session::RandomNumbers& random,
               events::Diagnostics& diagnostics);

    /** One frame: the drops above the water fall; one at the floor row starts again. */
    void move(int waterRow, data::levels::Wind wind, session::RandomNumbers& random,
              events::Diagnostics& diagnostics);

    /** The water surface shown this frame becomes the floor row. */
    void stopAt(int waterRow) { floorRow_ = waterRow; }

    int floorRow() const { return floorRow_; }
    void setFloorRow(int row) { floorRow_ = row; }
    const std::array<Raindrop, DROPS>& drops() const { return drops_; }

private:
    static constexpr int PREFALL_FRAMES = 577;

    std::array<Raindrop, DROPS> drops_{};
    int floorRow_ = 0;

    /** Drop i starts again on the top edge, or on the side the wind blows from. */
    void spawn(int i, int waterRow, data::levels::Wind wind, session::RandomNumbers& random);
};

}  // namespace ugh::world::scenery
