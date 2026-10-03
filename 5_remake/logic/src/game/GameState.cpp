#include "game/GameState.hpp"

namespace ugh::game {

void GameState::reset(const data::Rules& rules, const NewGameSettings& settings) {
    session.emplace(rules, settings.players, settings.difficulty, settings.firstLevel,
                    world::session::RandomNumbers(settings.randomSeed));
    level = world::Level(settings.players);
    level.rain().stopAt(settings.rainFloorRow);
    passengers = passengers::Passengers();
    enemies = enemies::Enemies();
    bonuses.clear();
    menu = input::MenuInput();
}

}  // namespace ugh::game
