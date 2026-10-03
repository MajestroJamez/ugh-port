// A field of the replay state.
#pragma once

#include <optional>
#include <string>

namespace ugh::replay {

/**
 * A value of the model as a field of the replays: how it is written as text, read from a text, and filled with
 * a pattern. A field refers to its value in a snapshot of an entity (it does not own it), so its methods are const:
 * they change the value, not the field.
 */
class Field {
public:
    virtual ~Field() = default;

    /** The text in the replay; nothing when the replay does not write the field now (no kind, no state yet). */
    virtual std::optional<std::string> text() const = 0;

    /** Sets the value from a replay text; false: a value the logic cannot work with (the value may be changed). */
    virtual bool read(const std::string& text) const = 0;

    /** Fills the value with `pattern`: memory of the original the core was never given. */
    virtual void fill(int pattern) const = 0;
};

}  // namespace ugh::replay
