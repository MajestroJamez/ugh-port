// The C API (include/ugh_sim.h) over the game and the replay projection.
#include "ugh_sim.h"

#include <algorithm>
#include <cstring>
#include <memory>
#include <string>

#include "core/EventQueue.hpp"
#include "data/GameDataLoader.hpp"
#include "game/Game.hpp"
#include "replay/ReplayProjection.hpp"

struct ugh_sim {
    std::unique_ptr<const ugh::data::GameData> data;
    ugh::game::Game game;
    ugh::replay::ReplayProjection projection;
    ugh::core::EventQueue events;

    explicit ugh_sim(std::unique_ptr<const ugh::data::GameData> loaded)
        : data(std::move(loaded)), game(*data), projection(*data) {
        game.addListener(events);
    }
};

extern "C" {

ugh_sim* ugh_sim_create(const char* data_path, char* err, size_t err_size) {
    std::string error;
    auto data = ugh::data::GameDataLoader::load(data_path, error);
    if (!data) {
        if (err && err_size) {
            size_t n = std::min(error.size(), err_size - 1);
            std::memcpy(err, error.data(), n);
            err[n] = 0;
        }
        return nullptr;
    }
    auto* s = new ugh_sim(std::move(data));
    s->game.reset();   // the program start: one player, medium difficulty, level 1
    return s;
}

void ugh_sim_destroy(ugh_sim* sim) { delete sim; }

void ugh_sim_reset(ugh_sim* sim, int fill) {
    sim->game.reset();
    sim->events.clear();
    sim->projection.reset(sim->game.level(), fill);
}

void ugh_sim_clear(ugh_sim* sim, int fill) { sim->projection.forget(sim->game.level(), fill); }

int ugh_sim_has_stage(const char* stage) {
    return std::strcmp(stage, "passengers") == 0 || std::strcmp(stage, "objects") == 0 || std::strcmp(stage, "bonuses") == 0;
}

int ugh_sim_set(ugh_sim* sim, const char* field, const char* value) {
    return sim->projection.set(sim->game.level(), field, value);
}

void ugh_sim_take_problems(ugh_sim* sim, void (*callback)(void* ctx, const char* problem), void* ctx) {
    for (const auto& p : sim->game.diagnostics().take()) callback(ctx, p.c_str());
}

void ugh_sim_fields(const ugh_sim* sim, void (*callback)(void* ctx, const char* field, const char* value), void* ctx) {
    for (const auto& [name, value] : sim->projection.fields(sim->game.level())) callback(ctx, name.c_str(), value.c_str());
}

void ugh_sim_take_events(ugh_sim* sim, void (*callback)(void* ctx, const ugh_sim_event* event), void* ctx) {
    for (const auto& e : sim->events.take()) {
        ugh_sim_event event{static_cast<int>(e.kind), e.player, e.entity, e.value};
        callback(ctx, &event);
    }
}

void ugh_sim_key(ugh_sim* sim, int scancode) { sim->game.key(static_cast<uint8_t>(scancode)); }

void ugh_sim_new_game(ugh_sim* sim) { sim->game.newGame(); }

int ugh_sim_level_end(ugh_sim* sim) { return sim->game.levelEnd(); }

void ugh_sim_level_start(ugh_sim* sim) { sim->game.levelStart(); }

void ugh_sim_play_frame(ugh_sim* sim) { sim->game.playFrame(); }

int ugh_sim_step(ugh_sim* sim) { return sim->game.step(); }

}
