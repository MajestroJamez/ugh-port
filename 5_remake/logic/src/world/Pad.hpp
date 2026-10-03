// A pad during the play.
#pragma once

#include <optional>

#include "data/levels/PadDefinition.hpp"

namespace ugh::world {

/** A pad of the running level: where it is, its place in the level's list, and the passenger who waits on it. */
class Pad {
public:
    Pad(int index, const data::levels::PadDefinition& place) : index_(index), place_(&place) {}

    /** Its place in the level's list: the data, the copter that stands on it and the replays name it so. */
    int index() const { return index_; }
    const data::levels::PadDefinition& place() const { return *place_; }

    bool free() const { return !waiting_.has_value(); }
    /** The index of the passenger who waits on it. */
    std::optional<int> waiting() const { return waiting_; }
    void occupy(int passenger) { waiting_ = passenger; }
    void vacate() { waiting_.reset(); }

private:
    int index_;
    const data::levels::PadDefinition* place_;
    std::optional<int> waiting_;
};

}  // namespace ugh::world
