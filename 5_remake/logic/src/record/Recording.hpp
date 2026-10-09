// The replay of a level.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "game/AttemptStart.hpp"
#include "record/Input.hpp"

namespace ugh::record {

/**
 * The replay of a level (a `.ughr`, docs/replay-format.md): what its first attempt started from, the inputs of the
 * logic between its steps, and how it went - from the step its first attempt started in to the step it ended in (the
 * next level's attempt started, or the game ended). A game resumed from `start` with these inputs (`Playback`) plays
 * it again exactly, on the same logic (`logicVersion`) and data (`dataHash`). The label (the level's password, the
 * pilots' names, the date) is the frontend's.
 */
struct Recording {
    /** The version of the logic's behaviour: a replay of another version may play differently. */
    static constexpr int LOGIC_VERSION = 1;
    static constexpr int MAX_NAMES = 2;
    static constexpr size_t MAX_TEXT = 32;   // bytes of the password and of a name (UTF-8)

    int logicVersion = LOGIC_VERSION;
    uint32_t dataHash = 0;   // CRC-32 of the game data (ugh-data.ugd)
    game::AttemptStart start;
    std::vector<Input> inputs;   // in their order, `after` never falling
    int steps = 0;       // from the first attempt's step to the level's last step
    int playSteps = 0;   // the steps the play shows (not the captions): its time
    int attempts = 0;
    uint32_t points = 0;   // earned in the level
    bool done = false;     // the level was done (else the game ended in it)

    int64_t date = 0;   // seconds since 1970 (UTC), 0 unknown
    std::string password;
    std::vector<std::string> names;

    /**
     * Better than `other` (none: nothing) as the best replay of its level and mode: only a level done counts; more
     * points, at the same points less time in the play.
     */
    bool betterThan(const Recording* other) const {
        if (!done) return false;
        if (!other || !other->done) return true;
        return points != other->points ? points > other->points : playSteps < other->playSteps;
    }

    bool operator==(const Recording&) const = default;
};

}  // namespace ugh::record
