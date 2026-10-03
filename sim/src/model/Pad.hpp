// A pad during the play.
#pragma once

#include "core/Word.hpp"
#include "data/PadDefinition.hpp"

namespace ugh::model {

/** A pad of the running level: where it is (from the level data) and which passenger waits on it. */
class Pad {
public:
    static constexpr int NOBODY = -1;

    struct Snapshot {
        data::PadDefinition place;
        int waiting = NOBODY;   // the passenger waiting there
    };

    Pad() = default;
    explicit Pad(const data::PadDefinition& place) { s_.place = place; }

    core::Word left() const { return s_.place.left; }
    core::Word right() const { return s_.place.right; }
    core::Word y() const { return s_.place.y; }
    core::Word doorX() const { return s_.place.doorX; }
    core::Word waitX() const { return s_.place.waitX; }
    core::Word standX() const { return s_.place.standX; }
    core::Word number() const { return s_.place.number; }

    /** x is over the landing area. */
    bool spans(core::Word x) const { return x >= s_.place.left && x <= s_.place.right; }

    bool free() const { return s_.waiting == NOBODY; }
    void occupy(int passenger) { s_.waiting = passenger; }
    void vacate() { s_.waiting = NOBODY; }

    const Snapshot& snapshot() const { return s_; }
    void restore(const Snapshot& snapshot) { s_ = snapshot; }

private:
    Snapshot s_;
};

}  // namespace ugh::model
