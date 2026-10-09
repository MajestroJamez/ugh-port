// What the handles of the C API (include/ugh_logic.h) hold.
#pragma once

#include <cstdint>
#include <memory>
#include <optional>

#include "data/GameData.hpp"
#include "events/EventQueue.hpp"
#include "game/Game.hpp"
#include "record/Playback.hpp"
#include "record/Recorder.hpp"
#include "record/Recording.hpp"

/**
 * The logic behind the C API: the game with its data, the events for the frontend, the recorder of the levels'
 * replays, the replay being watched. The keys reach the game and the recorder together.
 */
struct ugh_logic {
    std::unique_ptr<const ugh::data::GameData> data;
    ugh::game::Game game;
    ugh::events::EventQueue events;
    ugh::record::Recorder recorder;
    std::optional<ugh::record::Playback> watched;
    uint32_t dataHash;
    bool started = false;   // a new game was set up

    ugh_logic(std::unique_ptr<const ugh::data::GameData> loaded, uint32_t hash)
        : data(std::move(loaded)), game(*data), recorder(hash), dataHash(hash) {
        game.addListener(events);
        game.addListener(recorder);
    }

    void key(int player, ugh::input::PlayerKey key, bool pressed) {
        game.key(player, key, pressed);
        recorder.key(player, key, pressed);
    }
    void menuKey(ugh::input::MenuKey key) {
        game.menuKey(key);
        recorder.menuKey(key);
    }
    /** A new or resumed game is set up (or not): nothing of the last one stays. */
    void startedGame(bool ok) {
        events.take();
        recorder.reset();
        started = ok;
    }
};

/** A replay of the C API. */
struct ugh_replay {
    ugh::record::Recording recording;
};
