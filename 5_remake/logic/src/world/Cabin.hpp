// Whom a copter carries.
#pragma once

#include <optional>

#include "world/Cargo.hpp"

namespace ugh::world {

/** Whom a copter carries - a passenger of a route inside, or the standing passenger hanging below - and the fare. */
class Cabin {
public:
    bool hasRoom() const { return !cargo_.has_value(); }
    const std::optional<Cargo>& cargo() const { return cargo_; }
    int fare() const { return fare_; }

    /** A passenger of a route boards: who it is, the number of its destination pad, its fare and minimum fare. */
    void takeOnBoard(int look, int destination, int fare, int fareMin) {
        cargo_ = Cargo{look, destination, fareMin};
        fare_ = fare;
    }
    /** The standing passenger hangs below. */
    void pickUpHanging(int look) { cargo_ = Cargo{look, std::nullopt, 0}; }
    /** One frame of the ride: the fare drops by one down to its minimum. */
    void lowerFare() {
        if (cargo_ && fare_ > cargo_->fareMin) fare_ -= 1;
    }
    /** The passenger got out or was let go (the fare stays shown). */
    void unload() { cargo_.reset(); }
    /** At the start of an attempt: nobody, no fare. */
    void clear() {
        cargo_.reset();
        fare_ = 0;
    }

private:
    std::optional<Cargo> cargo_;
    int fare_ = 0;
};

}  // namespace ugh::world
