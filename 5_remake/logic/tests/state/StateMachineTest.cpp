#include <string>

#include "TestFramework.hpp"
#include "state/StateMachine.hpp"

namespace {

struct Log {
    std::string text;
};

class Lamp;
using LampState = ugh::state::State<Lamp, Log>;

class Lamp : public ugh::state::StateMachine<Lamp, Log> {
public:
    Lamp();
    void update(const Log& log) { updateState(log); }
    int frames = 0;
};

class On : public LampState {
public:
    static const On instance;
    const char* name() const override { return "On"; }
    void update(Lamp& lamp, const Log&) const override { lamp.frames += 10; }
};

class Off : public LampState {
public:
    static const Off instance;
    const char* name() const override { return "Off"; }
    void enter(Lamp& lamp, const Log&) const override { lamp.frames = 0; }
    void update(Lamp& lamp, const Log&) const override { lamp.frames += 1; }
};

const On On::instance{};
const Off Off::instance{};

Lamp::Lamp() : StateMachine(Off::instance) {}

}  // namespace

TEST(state_machine_starts_in_its_first_state_and_updates_it) {
    Lamp lamp;
    Log log;
    CHECK_EQUAL(std::string("Off"), std::string(lamp.state().name()));
    lamp.update(log);
    CHECK_EQUAL(1, lamp.frames);
}

TEST(state_machine_change_state_enters_now_and_updates_from_the_next_frame) {
    Lamp lamp;
    Log log;
    lamp.update(log);
    lamp.changeState(Off::instance, log);
    CHECK_EQUAL(0, lamp.frames);   // the entry action ran, the update did not
    lamp.changeState(On::instance, log);
    CHECK_EQUAL(std::string("On"), std::string(lamp.state().name()));
    lamp.update(log);
    CHECK_EQUAL(10, lamp.frames);
}

TEST(state_machine_continue_in_enters_and_updates_now) {
    Lamp lamp;
    Log log;
    lamp.update(log);
    lamp.update(log);
    lamp.continueIn(Off::instance, log);
    CHECK_EQUAL(1, lamp.frames);   // reset by the entry action, then one update
}
