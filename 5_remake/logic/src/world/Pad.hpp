// A pad during the play.
#pragma once

#include "data/levels/PadDefinition.hpp"
#include "world/Figure.hpp"

namespace ugh::world {

/** A pad of the running level: where it is, its place in the level's list, and the passenger who waits on it. */
class Pad {
public:
    Pad(int index, const data::levels::PadDefinition& place) : index_(index), place_(&place) {}

    /** Its place in the level's list: the data, the copter that stands on it and the replays name it so. */
    int index() const { return index_; }
    const data::levels::PadDefinition& place() const { return *place_; }

    bool free() const { return waiting_ == nullptr; }
    /** The passenger who waits on it; nullptr if none. */
    const Figure* waiting() const { return waiting_; }
    void occupy(const Figure& passenger) { waiting_ = &passenger; }
    void vacate() { waiting_ = nullptr; }

private:
    int index_;
    const data::levels::PadDefinition* place_;
    const Figure* waiting_ = nullptr;
};

}  // namespace ugh::world
