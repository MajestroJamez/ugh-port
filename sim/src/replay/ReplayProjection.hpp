// The adapter between the model and the golden replays.
#pragma once

#include <string>
#include <utility>
#include <vector>

#include "data/GameData.hpp"
#include "model/Level.hpp"
#include "replay/FieldReader.hpp"
#include "replay/FieldVisitor.hpp"

namespace ugh::replay {

/**
 * The C++ side of StateProjection.kt (format "UGR 0", verify/src/test/kotlin/ugh/verify/replay/ReplayWriter.kt):
 * the only place that knows the field names and value formats of the replays, and the original's offsets they use
 * for data (kinds, routes, animations). The fields of each entity are in one list (GameFields, CopterFields ...);
 * the projection runs a visitor over them on a snapshot of the entity (Memento) and restores the snapshot.
 *
 * It is also the boundary of the replay state: a value the logic could not work with (a pad index out of the
 * slots, an unknown wind) is refused, so the logic needs no checks.
 */
class ReplayProjection {
public:
    explicit ReplayProjection(const data::GameData& data) : data_(data) {}

    /**
     * Forgets the replay state: the level lists are empty, no bonus item in use, and every other value of the
     * replay state gets a pattern from `fill` (the raindrops stay: the replay has only their checksum). A value set
     * afterwards, or computed only from such values, does not depend on the pattern; the replay player runs two
     * cores with different patterns and compares only the values they agree on (memory the original has but the
     * core was never given, like what the attract mode left).
     */
    void forget(model::Level& level, int fill);

    /** The program start: forget(), and the raindrops (not in the replay) are unknown too. */
    void reset(model::Level& level, int fill);

    /** Sets a field ("copter.0.xf") from its replay text: 1 = set, 0 = not a field of the core, -1 = a bad value. */
    int set(model::Level& level, const std::string& field, const std::string& value);

    /** Every field of the replay state, as the replays write it (the lists up to their ends, the bonus items in use). */
    std::vector<std::pair<std::string, std::string>> fields(const model::Level& level) const;

private:
    const data::GameData& data_;
    int fill_ = 0;

    // one entity's fields, on a snapshot that is restored when the visitor changed a value
    void changeGame(FieldVisitor& v, model::Level& level) const;
    void change(FieldVisitor& v, model::Copter& copter) const;
    void change(FieldVisitor& v, model::Pad& pad) const;
    void change(FieldVisitor& v, model::Passenger& passenger) const;
    void change(FieldVisitor& v, model::Enemy& enemy) const;
    void change(FieldVisitor& v, model::BonusItem& item) const;

    /** Sets a field of entry `index` of a group ("copter", "pad" ...). */
    int setInGroup(model::Level& level, const std::string& group, int index, FieldReader& reader) const;

    /** A field of a list entry ("pad", "passenger", "object") makes the list that long; false past the slots. */
    static bool lengthenList(model::Level& level, const std::string& group, int index);
};

}  // namespace ugh::replay
