#include "LevelSetup.hpp"

#include "TestFramework.hpp"
#include "game/LevelLoader.hpp"
#include "passengers/PassengerState.hpp"

namespace ugh::test {

using core::Fixed;
using core::Word;

LevelSetup::LevelSetup(int number) : game(gameData()), level(game.level()) {
    game.addListener(events);
    model::GameSession::Snapshot session = level.session().snapshot();
    session.levelNumber = number;
    level.session().restore(session);
    game.newGame();
    game::LevelLoader::startAttempt(level);
}

std::string LevelSetup::passengerState(int i) const { return level.passenger(i).state().name(); }

bool LevelSetup::runPassengersUntil(int i, const std::string& state, int frames) {
    for (int f = 0; f < frames; f++) {
        if (passengerState(i) == state) return true;
        level.updatePassengers();
        level.updatePassengerPixels();
    }
    return passengerState(i) == state;
}

void LevelSetup::land(int c, int pad, Word x) {
    model::Copter& copter = level.copter(c);
    copter.moveToX(Fixed::fromPixels(x));
    copter.land(pad);
}

void LevelSetup::hover(int c, int height) {
    model::Copter& copter = level.copter(c);
    const model::Pad& pad = level.pad(1);
    copter.moveToX(Fixed::fromPixels(Word((pad.left().value() + pad.right().value()) / 2) - 0x10));
    copter.moveToY(Fixed::fromPixels(pad.y() - 0x14 - height));
}

bool LevelSetup::reported(core::EventKind kind) const {
    for (const core::Event& e : events.events())
        if (e.kind == kind) return true;
    return false;
}

}  // namespace ugh::test
