// A pointer of the original an enemy uses for two things.
#pragma once

#include <cstdint>
#include <optional>
#include <variant>

#include "data/Animation.hpp"
#include "data/DropList.hpp"

namespace ugh::model {

/**
 * The enemy's word 2cd5, a pointer into the data: the flyer's flight animation (left or right), or the tree's next
 * bonus item. A walker or a blower does not use it, so it keeps what an earlier enemy in the slot left there.
 */
class EnemyTable {
public:
    EnemyTable() = default;

    static EnemyTable flight(const data::Animation* animation) { return EnemyTable(Value(animation)); }
    static EnemyTable drops(data::DropCursor cursor) { return EnemyTable(Value(cursor)); }
    /** A word the core cannot read (left by an earlier use of the slot). */
    static EnemyTable leftover(uint16_t word) { return EnemyTable(Value(word)); }

    /** The flyer's flight animation; nullptr when the word is something else. */
    const data::Animation* flightAnimation() const {
        auto a = std::get_if<const data::Animation*>(&value_);
        return a ? *a : nullptr;
    }
    bool isFlightAnimation() const { return std::holds_alternative<const data::Animation*>(value_); }

    /** The tree's next bonus item; nullptr when the word is something else. */
    const data::DropCursor* dropCursor() const { return std::get_if<data::DropCursor>(&value_); }
    data::DropCursor* dropCursor() { return std::get_if<data::DropCursor>(&value_); }

    std::optional<uint16_t> leftoverWord() const {
        auto w = std::get_if<uint16_t>(&value_);
        return w ? std::optional<uint16_t>(*w) : std::nullopt;
    }

private:
    using Value = std::variant<uint16_t, const data::Animation*, data::DropCursor>;

    explicit EnemyTable(Value value) : value_(value) {}

    Value value_;
};

}  // namespace ugh::model
