// The world of the level being played.
#pragma once

#include "world/Rain.hpp"

namespace ugh::world {

/** The world of the level being played. */
class Level {
public:
    Rain& rain() { return rain_; }
    const Rain& rain() const { return rain_; }

private:
    Rain rain_;
};

}  // namespace ugh::world
