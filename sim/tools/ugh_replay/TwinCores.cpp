#include "TwinCores.hpp"

extern "C" void ugh_sim_audit_enable(int on);

namespace ugh::tool {

TwinCores::TwinCores(const char* dataPath) {
    char err[256];
    a_ = ugh_sim_create(dataPath, err, sizeof err);
    b_ = a_ ? ugh_sim_create(dataPath, err, sizeof err) : nullptr;
    if (!b_) error_ = err;
}

TwinCores::~TwinCores() {
    if (a_) ugh_sim_destroy(a_);
    if (b_) ugh_sim_destroy(b_);
}

void TwinCores::reset() {
    ugh_sim_reset(a_, PATTERN_A);
    ugh_sim_reset(b_, PATTERN_B);
}

void TwinCores::clear() {
    ugh_sim_clear(a_, PATTERN_A);
    ugh_sim_clear(b_, PATTERN_B);
}

int TwinCores::set(const std::string& name, const std::string& value) {
    int a = ugh_sim_set(a_, name.c_str(), value.c_str()), b = ugh_sim_set(b_, name.c_str(), value.c_str());
    return a < 0 || b < 0 ? -1 : a;
}

void TwinCores::key(int scancode) {
    ugh_sim_key(a_, scancode);
    ugh_sim_key(b_, scancode);
}

int TwinCores::step() {
    int a = ugh_sim_step(a_);
    ugh_sim_audit_enable(0);
    int b = ugh_sim_step(b_);
    ugh_sim_audit_enable(1);
    return agreed(a, b);
}

void TwinCores::newGame() {
    ugh_sim_new_game(a_);
    ugh_sim_new_game(b_);
}

int TwinCores::levelEnd() { return agreed(ugh_sim_level_end(a_), ugh_sim_level_end(b_)); }

void TwinCores::levelStart() {
    ugh_sim_level_start(a_);
    ugh_sim_level_start(b_);
}

void TwinCores::playFrame() {
    ugh_sim_play_frame(a_);
    ugh_sim_play_frame(b_);
}

Fields TwinCores::known() const {
    Fields a = fields(a_), b = fields(b_), both;
    for (const auto& [name, value] : a) {
        auto it = b.find(name);
        if (it != b.end() && it->second == value) both.emplace(name, value);
    }
    return both;
}

void TwinCores::takeProblems(void (*callback)(void* ctx, const char* problem), void* ctx) {
    ugh_sim_take_problems(a_, callback, ctx);
    ugh_sim_take_problems(b_, [](void*, const char*) {}, nullptr);
    ugh_sim_take_events(a_, [](void*, const ugh_sim_event*) {}, nullptr);
    ugh_sim_take_events(b_, [](void*, const ugh_sim_event*) {}, nullptr);
}

Fields TwinCores::fields(const ugh_sim* sim) {
    Fields f;
    ugh_sim_fields(
        sim, [](void* ctx, const char* name, const char* value) { (*static_cast<Fields*>(ctx))[name] = value; }, &f);
    return f;
}

}  // namespace ugh::tool
