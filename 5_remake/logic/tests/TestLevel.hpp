// A small level for the tests.
#pragma once

#include <memory>

#include "data/GameData.hpp"
#include "data/LevelDefinition.hpp"
#include "events/Diagnostics.hpp"
#include "events/EventQueue.hpp"
#include "world/Level.hpp"
#include "world/PlayContext.hpp"
#include "world/Session.hpp"

namespace ugh::test {

/**
 * A level made for a test: one pad (x 100 .. 200, y 150), the water at row `waterRow`, an empty mask but for what
 * the test adds, and a session and context to play it with the real data's rules.
 */
class TestLevel {
public:
    explicit TestLevel(int waterRow = 190, data::Wind wind = data::Wind::None);

    /** A solid pixel in the mask (before start()). */
    void solid(int x, int y);
    /** The attempt starts: the level is loaded. */
    void start();

    world::Level& level() { return level_; }
    world::Session& session() { return session_; }
    world::PlayContext context() { return {level_, session_, gameData(), events, diagnostics}; }

    events::EventQueue events;
    events::Diagnostics diagnostics;

private:
    static const data::GameData& gameData();

    data::LevelDefinition definition_;
    std::vector<uint8_t> mask_;
    world::Level level_;
    world::Session session_;
};

}  // namespace ugh::test
