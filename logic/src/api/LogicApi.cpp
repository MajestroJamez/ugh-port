// The C API (include/ugh_logic.h) over the game.
#include "ugh_logic.h"

#include <algorithm>
#include <cstring>
#include <memory>
#include <string>

#include "data/DataFileReader.hpp"
#include "events/EventQueue.hpp"
#include "game/Game.hpp"

struct ugh_logic {
    std::unique_ptr<const ugh::data::GameData> data;
    ugh::game::Game game;
    ugh::events::EventQueue events;

    explicit ugh_logic(std::unique_ptr<const ugh::data::GameData> loaded) : data(std::move(loaded)), game(*data) {
        game.addListener(events);
    }
};

namespace {

int eventKind(ugh::events::EventKind kind) { return static_cast<int>(kind) + UGH_LOGIC_EVENT_LEVEL_CAPTION; }

int result(ugh::game::GameResult r) {
    switch (r) {
        case ugh::game::GameResult::Continue: return UGH_LOGIC_CONTINUE;
        case ugh::game::GameResult::GameOver: return UGH_LOGIC_GAME_OVER;
        case ugh::game::GameResult::AllLevelsDone: return UGH_LOGIC_ALL_LEVELS_DONE;
    }
    return UGH_LOGIC_GAME_OVER;
}

}  // namespace

extern "C" {

ugh_logic* ugh_logic_create(const char* data_path, char* err, size_t err_size) {
    std::string error;
    auto data = ugh::data::DataFileReader::read(data_path, error);
    if (!data) {
        if (err && err_size) {
            size_t n = std::min(error.size(), err_size - 1);
            std::memcpy(err, error.data(), n);
            err[n] = 0;
        }
        return nullptr;
    }
    return new ugh_logic(std::move(data));
}

void ugh_logic_destroy(ugh_logic* logic) { delete logic; }

int ugh_logic_new_game(ugh_logic* logic, const ugh_logic_settings* settings) {
    if (settings->players < 1 || settings->players > 2 || settings->difficulty < 0 || settings->difficulty > 2 ||
        settings->first_level < 0 || settings->first_level >= logic->data->levelCount(settings->players) ||
        settings->rain_floor_row < 0 || settings->rain_floor_row > 255)
        return 0;
    ugh::game::NewGameSettings s;
    s.players = settings->players;
    s.difficulty = static_cast<ugh::data::Difficulty>(settings->difficulty);
    s.firstLevel = settings->first_level;
    for (int i = 0; i < 4; i++) s.randomSeed[i] = settings->random_seed[i];
    s.rainFloorRow = settings->rain_floor_row;
    logic->game.newGame(s);
    logic->events.take();
    return 1;
}

void ugh_logic_scancode(ugh_logic* logic, int scancode) { logic->game.scancode(static_cast<uint8_t>(scancode)); }

int ugh_logic_step(ugh_logic* logic) { return result(logic->game.step()); }

void ugh_logic_take_events(ugh_logic* logic, void (*callback)(void* ctx, const ugh_logic_event* event), void* ctx) {
    for (const ugh::events::Event& e : logic->events.take()) {
        ugh_logic_event event{eventKind(e.kind), e.player.value_or(-1), e.entity.value_or(-1), e.value};
        callback(ctx, &event);
    }
}

}
