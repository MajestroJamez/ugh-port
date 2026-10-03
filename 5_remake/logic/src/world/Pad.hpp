// A pad during the play.
#pragma once

#include <optional>

#include "data/PadDefinition.hpp"

namespace ugh::world {

/** A pad of the running level: where it is, and the passenger who waits on it. */
class Pad {
public:
    explicit Pad(const data::PadDefinition& place) : place_(&place) {}

    const data::PadDefinition& place() const { return *place_; }

    bool free() const { return !waiting_.has_value(); }
    std::optional<int> waiting() const { return waiting_; }
    void occupy(int passenger) { waiting_ = passenger; }
    void vacate() { waiting_.reset(); }

private:
    const data::PadDefinition* place_;
    std::optional<int> waiting_;
};

}  // namespace ugh::world
