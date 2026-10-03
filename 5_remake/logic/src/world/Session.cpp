#include "world/Session.hpp"

namespace ugh::world {

void Session::startGame() {
    lives_ = START_LIVES;
    score_.start();
}

bool Session::nextLevel(int levelCount) {
    levelNumber_++;
    return levelNumber_ < levelCount;
}

bool Session::loseLife() {
    lives_--;
    if (lives_ <= 0) return false;
    score_.resetMultiplier();
    return true;
}

void Session::addLives(int amount) {
    lives_ += amount;
    if (lives_ > MAX_LIVES) lives_ = MAX_LIVES;
}

}  // namespace ugh::world
