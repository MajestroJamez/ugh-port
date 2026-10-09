#include "world/session/Session.hpp"

namespace ugh::world::session {

void Session::startGame() {
    lives_ = Lives(Lives::START);
    score_.start();
}

void Session::resumeGame(int lives, uint32_t points, int multiplier) {
    lives_ = Lives(lives);
    score_.resume(points, multiplier);
}

bool Session::nextLevel(int levelCount) {
    levelNumber_++;
    return levelNumber_ < levelCount;
}

bool Session::loseLife() {
    if (!lives_.lose()) return false;
    score_.resetMultiplier();
    return true;
}

}  // namespace ugh::world::session
