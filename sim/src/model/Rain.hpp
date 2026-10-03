// The rain of the windy levels.
#pragma once

#include <array>
#include <cstdint>

#include "core/Diagnostics.hpp"
#include "core/Random.hpp"
#include "core/Word.hpp"

namespace ugh::model {

/**
 * The rain of the windy levels: 193 drops falling diagonally with the wind down to the water. A drop's position is a
 * pixel index in the 384 px wide background page (y * 384 + x), so a drop blown over the edge runs into the next row
 * like in the original.
 */
class Rain {
public:
    static constexpr int DROPS = 193;

    struct Snapshot {
        std::array<int32_t, DROPS> drops{};
        uint8_t floorRow = 180;   // drops at or below this row start again (2907 = row * 96)
    };

    /** 113b:3976 (the end of loadLevel) - a windy level starts with the rain already falling. */
    void start(core::Word waterRow, uint8_t wind, core::Random& random, core::Diagnostics& diagnostics);

    /**
     * 113b:3c78 - Draw.kt moveRain: the drops down to the water fall diagonally with the wind (1 to the left,
     * 2 to the right), the even ones 3 px a frame, the odd ones 2 px; a drop at the water starts again.
     */
    void move(core::Word waterRow, uint8_t wind, core::Random& random, core::Diagnostics& diagnostics);

    /** 113b:2db9 - the drops start again at the surface of the water as it is drawn this frame. */
    void stopAt(core::Word waterRow) { s_.floorRow = static_cast<uint8_t>(waterRow.bits()); }

    const Snapshot& snapshot() const { return s_; }
    void restore(const Snapshot& snapshot) { s_ = snapshot; }

private:
    Snapshot s_;

    /** 113b:3c35 - Draw.kt spawnRaindrop: drop i starts again at a random place on the top edge or the windward side. */
    void spawn(int i, core::Word waterRow, uint8_t wind, core::Random& random);
};

}  // namespace ugh::model
