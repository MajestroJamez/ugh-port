// C API (include/ugh_sim.h) over ugh::Sim.
#include "ugh_sim.h"

#include <algorithm>
#include <cstring>

#include "sim.hpp"

struct ugh_sim {
    ugh::Sim sim;
};

extern "C" {

ugh_sim* ugh_sim_create(const char* data_path, char* err, size_t err_size) {
    ugh::Data data;
    std::string error;
    if (!data.load(data_path, error)) {
        if (err && err_size) {
            size_t n = std::min(error.size(), err_size - 1);
            std::memcpy(err, error.data(), n);
            err[n] = 0;
        }
        return nullptr;
    }
    auto* s = new ugh_sim{ugh::Sim(std::move(data))};
    s->sim.reset();
    return s;
}

void ugh_sim_destroy(ugh_sim* sim) { delete sim; }

void ugh_sim_reset(ugh_sim* sim) { sim->sim.reset(); }

void ugh_sim_clear(ugh_sim* sim) { sim->sim.clear(); }

int ugh_sim_has_stage(const char* stage) {
    return std::strcmp(stage, "passengers") == 0 || std::strcmp(stage, "objects") == 0 || std::strcmp(stage, "bonuses") == 0;
}

void ugh_sim_take_problems(ugh_sim* sim, void (*callback)(void* ctx, const char* problem), void* ctx) {
    for (const auto& p : sim->sim.problems) callback(ctx, p.c_str());
    sim->sim.problems.clear();
}

int ugh_sim_set(ugh_sim* sim, const char* field, const char* value) { return sim->sim.set(field, value); }

void ugh_sim_fields(const ugh_sim* sim, void (*callback)(void* ctx, const char* field, const char* value), void* ctx) {
    for (const auto& [name, value] : sim->sim.fields()) callback(ctx, name.c_str(), value.c_str());
}

void ugh_sim_key(ugh_sim* sim, int scancode) { sim->sim.key(scancode); }

void ugh_sim_new_game(ugh_sim* sim) { sim->sim.newGame(); }

int ugh_sim_level_end(ugh_sim* sim) { return sim->sim.levelEnd(); }

void ugh_sim_level_start(ugh_sim* sim) { sim->sim.levelStart(); }

void ugh_sim_play_frame(ugh_sim* sim) { sim->sim.playFrame(); }

int ugh_sim_step(ugh_sim* sim) { return sim->sim.step(); }

}
