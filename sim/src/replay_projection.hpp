// The adapter between the model and the golden replays (format "UGR 0", verify/src/test/kotlin/ugh/verify/replay/
// ReplayWriter.kt): the C++ side of StateProjection.kt. The only place that knows the field names and value
// formats of the replays, and the original's offsets they use for data (kinds, routes, animations).
#pragma once

#include <string>
#include <utility>
#include <vector>

#include "data.hpp"
#include "world.hpp"

namespace ugh {

class ReplayProjection {
public:
    explicit ReplayProjection(const GameData& data) : data_(data) {}

    /**
     * Forgets the replay state: the level lists are empty, no bonus item in use, and every other value of the
     * replay state gets a pattern from `fill` (the raindrops stay: the replay has only their checksum). A value set
     * afterwards, or computed only from such values, does not depend on the pattern; the replay player runs two
     * cores with different patterns and compares only the values they agree on (memory the original has but the
     * core was never given, like what the attract mode left).
     */
    void forget(World& world, int fill);

    /** The program start: forget(), and the raindrops (not in the replay) are unknown too. */
    void reset(World& world, int fill);

    /** Sets a field from its replay text: 1 = set, 0 = not a field of the core, -1 = a bad value. */
    int set(World& world, const std::string& field, const std::string& value);

    /** Every field of the replay state, as the replays write it (the lists up to their ends, the bonus items in use). */
    std::vector<std::pair<std::string, std::string>> fields(const World& world) const;

private:
    const GameData& data_;
    int fill_ = 0;
};

}  // namespace ugh
