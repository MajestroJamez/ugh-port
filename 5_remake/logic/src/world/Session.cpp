#include "world/Session.hpp"

namespace ugh::world {

void Session::startGame() {
    lives_ = Lives(Lives::START);
    score_.start();
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

}  // namespace ugh::world
