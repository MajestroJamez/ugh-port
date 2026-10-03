#include "game/LevelLoader.hpp"

#include <string>

#include "core/Audit.hpp"

#include "enemies/EnemyBehavior.hpp"
#include "passengers/standing/StartStanding.hpp"
#include "passengers/walking/NextStop.hpp"

namespace ugh::game {

void LevelLoader::startAttempt(model::Level& level) {
    level.startAttempt();
    load(level);
}

/** 113b:3976 - Level.kt loadLevel. */
void LevelLoader::load(model::Level& level) {
    core::audit::Scope scope("level load");
    const data::LevelDefinition* definition = level.definition();
    model::GameSession& session = level.session();
    if (!definition) {
        level.diagnostics().report("no level " + std::to_string(session.levelNumber().bits()) + " for " +
                                   std::to_string(session.players().bits()) + " players");
        return;
    }
    level.loadLists(*definition);
    for (int player = 0; player < model::Level::COPTERS; player++)   // both copters, also with one player
        level.copter(player).placeAtStart(definition->startX[player], definition->startY[player],
                                          level.data().rotorFirst(player));
    level.water().fillTo(definition->water);
    for (int i = 0; i < level.passengerCount(); i++) {
        const data::PassengerPlacement& placement = definition->passengers[i];
        if (placement.kind->type == data::PassengerKind::Type::Standing)
            level.passenger(i).placeStanding(placement, passengers::StartStanding::instance);
        else
            level.passenger(i).placeWalking(placement, passengers::NextStop::instance);
    }
    for (int i = 0; i < level.enemyCount(); i++) {
        const data::EnemyPlacement& placement = definition->enemies[i];
        enemies::EnemyBehavior::of(placement.kind->type).place(level.enemy(i), placement);
    }
    if (level.windy())
        level.rain().start(level.water().row(), level.wind(), session.random(), level.diagnostics());
}

}  // namespace ugh::game
