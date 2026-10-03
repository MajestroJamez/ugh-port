#include "model/GameSession.hpp"

namespace ugh::model {

namespace {

constexpr uint8_t START_LIVES = 3;
constexpr uint8_t MAX_LIVES = 0x63;

}  // namespace

void GameSession::newGame() {
    s_.lives = START_LIVES;
    s_.multiplier = 1;
    s_.score = 0;
}

const data::LevelDefinition* GameSession::levelDefinition() const {
    return data_.level(s_.players.value(), s_.levelNumber.value());
}

bool GameSession::nextLevel() {
    int next = s_.levelNumber.bits() + 1;
    s_.levelNumber = next;
    return next < data_.levelCount(s_.players.value());
}

bool GameSession::loseLife() {
    s_.lives--;
    if (s_.lives == 0 || (s_.lives & 0x80)) return false;   // none left, or below zero after Esc
    s_.multiplier = 1;
    return true;
}

void GameSession::addLives(core::Word amount) {
    auto lives = static_cast<uint8_t>(amount.value() + s_.lives);
    s_.lives = lives > MAX_LIVES ? MAX_LIVES : lives;
}

bool GameSession::multiplierBelowLimit() const {
    return s_.multiplier < data_.multiplierLimit(s_.difficulty.value()).bits();
}

void GameSession::raiseMultiplier() {
    if (multiplierBelowLimit()) s_.multiplier++;
}

}  // namespace ugh::model
