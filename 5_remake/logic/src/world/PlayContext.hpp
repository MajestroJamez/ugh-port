// What every update gets.
#pragma once

#include "data/GameData.hpp"
#include "events/Diagnostics.hpp"
#include "events/EventListener.hpp"
#include "world/Level.hpp"
#include "world/session/Session.hpp"

namespace ugh::world {

/** What the update of every entity gets (Parameter Object): the world, the session, the data and where to report. */
struct PlayContext {
    Level& level;
    session::Session& session;
    const data::GameData& data;
    events::EventListener& events;
    events::Diagnostics& diagnostics;

    void report(const events::Event& event) const { events.onEvent(event); }
};

}  // namespace ugh::world
