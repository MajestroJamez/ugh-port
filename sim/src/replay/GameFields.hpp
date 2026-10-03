// The game fields of the replay state (game.*).
#pragma once

#include "core/Random.hpp"
#include "model/GameSession.hpp"
#include "model/Level.hpp"
#include "model/Rain.hpp"
#include "model/Water.hpp"
#include "replay/FieldVisitor.hpp"

namespace ugh::replay {

/** The game fields: the session, the values of the level, the water, the rain. */
class GameFields {
public:
    /** The snapshots the game fields are in. */
    struct State {
        model::GameSession::Snapshot session;
        core::Random::Snapshot random;
        model::Level::Snapshot level;
        model::Water::Snapshot water;
        model::Rain::Snapshot rain;

        static State of(const model::Level& level);
        void restoreTo(model::Level& target) const;
    };

    /**
     * Every game field. The replay has only a checksum of the raindrops: the core keeps its own drops while they
     * match it, else it fills them with `unknownPattern`.
     */
    static void visit(FieldVisitor& v, State& s, int unknownPattern);
};

}  // namespace ugh::replay
