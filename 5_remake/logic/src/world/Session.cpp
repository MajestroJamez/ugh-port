#include "world/Session.hpp"

namespace ugh::world {

void Session::startGame() {
    lives_ = START_LIVES;
    multiplier_ = 1;
    score_ = 0;
}

bool Session::nextLevel(int levelCount) {
    levelNumber_++;
    return levelNumber_ < levelCount;
}

bool Session::loseLife() {
    lives_--;
    if (lives_ <= 0) return false;
    multiplier_ = 1;
    return true;
}

void Session::addLives(units::Int16 amount) {
    lives_ += amount.value();
    if (lives_ > MAX_LIVES) lives_ = MAX_LIVES;
}

void Session::raiseMultiplier() {
    if (multiplierBelowLimit()) multiplier_++;
}

}  // namespace ugh::world
