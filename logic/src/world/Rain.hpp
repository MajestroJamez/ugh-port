// The rain of the windy levels.
#pragma once

#include <array>

#include "data/Wind.hpp"
#include "events/Diagnostics.hpp"
#include "units/Int16.hpp"
#include "world/RandomNumbers.hpp"
#include "world/Raindrop.hpp"

namespace ugh::world {

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
    void start(units::Int16 waterRow, data::Wind wind, RandomNumbers& random, events::Diagnostics& diagnostics);

    /** One frame: the drops above the water fall; one at the floor row starts again. */
    void move(units::Int16 waterRow, data::Wind wind, RandomNumbers& random, events::Diagnostics& diagnostics);

    /** The water surface shown this frame becomes the floor row. */
    void stopAt(units::Int16 waterRow) { floorRow_ = waterRow.value(); }

    int floorRow() const { return floorRow_; }
    void setFloorRow(int row) { floorRow_ = row; }
    const std::array<Raindrop, DROPS>& drops() const { return drops_; }

private:
    static constexpr int PREFALL_FRAMES = 577;
    static constexpr int SCREEN_WIDTH = 320;

    std::array<Raindrop, DROPS> drops_{};
    int floorRow_ = 0;

    /** Drop i starts again on the top edge, or on the side the wind blows from. */
    void spawn(int i, units::Int16 waterRow, data::Wind wind, RandomNumbers& random);
};

}  // namespace ugh::world
