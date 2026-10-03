// C API (include/ugh_sim.h) over ugh::Game and the replay projection.
#include "ugh_sim.h"

#include <algorithm>
#include <cstring>
#include <memory>

#include "game.hpp"
#include "replay_projection.hpp"

struct ugh_sim {
    std::unique_ptr<const ugh::GameData> data;
    ugh::Game game;
    ugh::ReplayProjection projection;

    explicit ugh_sim(std::unique_ptr<const ugh::GameData> loaded)
        : data(std::move(loaded)), game(*data), projection(*data) {}
};

extern "C" {

ugh_sim* ugh_sim_create(const char* data_path, char* err, size_t err_size) {
    std::string error;
    auto data = ugh::GameData::load(data_path, error);
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
    sim->projection.reset(sim->game.world, fill);
}

void ugh_sim_clear(ugh_sim* sim, int fill) { sim->projection.forget(sim->game.world, fill); }

int ugh_sim_has_stage(const char* stage) {
    return std::strcmp(stage, "passengers") == 0 || std::strcmp(stage, "objects") == 0 || std::strcmp(stage, "bonuses") == 0;
}

int ugh_sim_set(ugh_sim* sim, const char* field, const char* value) { return sim->projection.set(sim->game.world, field, value); }

void ugh_sim_take_problems(ugh_sim* sim, void (*callback)(void* ctx, const char* problem), void* ctx) {
    for (const auto& p : sim->game.problems) callback(ctx, p.c_str());
    sim->game.problems.clear();
}

void ugh_sim_fields(const ugh_sim* sim, void (*callback)(void* ctx, const char* field, const char* value), void* ctx) {
    for (const auto& [name, value] : sim->projection.fields(sim->game.world)) callback(ctx, name.c_str(), value.c_str());
}

void ugh_sim_take_events(ugh_sim* sim, void (*callback)(void* ctx, const ugh_sim_event* event), void* ctx) {
    for (const auto& e : sim->game.events) {
        ugh_sim_event event{static_cast<int>(e.kind), e.player, e.entity, e.value};
        callback(ctx, &event);
    }
    sim->game.events.clear();
}

void ugh_sim_key(ugh_sim* sim, int scancode) { sim->game.key(static_cast<uint8_t>(scancode)); }

void ugh_sim_new_game(ugh_sim* sim) { sim->game.newGame(); }

int ugh_sim_level_end(ugh_sim* sim) { return sim->game.levelEnd(); }

void ugh_sim_level_start(ugh_sim* sim) { sim->game.levelStart(); }

void ugh_sim_play_frame(ugh_sim* sim) { sim->game.playFrame(); }

int ugh_sim_step(ugh_sim* sim) { return sim->game.step(); }

}
