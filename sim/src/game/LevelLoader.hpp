// A level attempt and the level load.
#pragma once

#include "model/Level.hpp"

namespace ugh::game {

/**
 * 113b:3d66 and 3976 - Level.kt levelSetup and loadLevel: the state of a new attempt, and the load of the level
 * from its definition: copters, water, pads (list A), passengers (list B), enemies (list C), rain.
 */
class LevelLoader {
public:
    /** A new attempt at the level of the session: the start state, and the level loaded. */
    static void startAttempt(model::Level& level);

private:
    static void load(model::Level& level);
};

}  // namespace ugh::game
