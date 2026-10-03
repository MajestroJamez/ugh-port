#include <string>
#include <vector>

#include "TestFramework.hpp"
#include "ugh_logic.h"
#include "world/Session.hpp"

TEST(the_c_api_refuses_a_missing_data_file) {
    char err[200] = "";
    CHECK(ugh_logic_create("no-such-file.ugd", err, sizeof err) == nullptr);
    CHECK(std::string(err).find("cannot open") != std::string::npos);
}

TEST(the_default_settings_start_a_one_player_game) {
    ugh_logic_settings settings;
    ugh_logic_default_settings(&settings);
    CHECK_EQUAL(1, settings.players);
    CHECK_EQUAL(1, settings.difficulty);
    CHECK_EQUAL(0, settings.first_level);
    CHECK_EQUAL(180, settings.rain_floor_row);
    char err[200] = "";
    ugh_logic* logic = ugh_logic_create(ugh::test::dataPath().c_str(), err, sizeof err);
    CHECK(logic != nullptr);
    if (!logic) return;
    CHECK_EQUAL(1, ugh_logic_new_game(logic, &settings));
    CHECK_EQUAL(static_cast<int>(UGH_LOGIC_CONTINUE), ugh_logic_step(logic));
    ugh_logic_destroy(logic);
}

TEST(the_c_api_plays_a_game_and_shows_it) {
    char err[200] = "";
    ugh_logic* logic = ugh_logic_create(ugh::test::dataPath().c_str(), err, sizeof err);
    CHECK(logic != nullptr);
    if (!logic) return;
    CHECK_EQUAL(static_cast<int>(UGH_LOGIC_GAME_OVER), ugh_logic_step(logic));   // no game yet
    ugh_logic_settings settings{1, 1, 0, {0x0003, 0x8134, 0x48bc, 0x2347}, 180};
    ugh_logic_settings bad = settings;
    bad.first_level = 69;
    CHECK_EQUAL(0, ugh_logic_new_game(logic, &bad));
    CHECK_EQUAL(1, ugh_logic_new_game(logic, &settings));
    for (int frame = 0; frame < 9; frame++) CHECK_EQUAL(static_cast<int>(UGH_LOGIC_CONTINUE), ugh_logic_step(logic));
    std::vector<int> events;
    ugh_logic_take_events(
        logic, [](void* ctx, const ugh_logic_event* e) { static_cast<std::vector<int>*>(ctx)->push_back(e->kind); }, &events);
    CHECK_EQUAL(1, static_cast<int>(events.size()));
    if (!events.empty()) CHECK_EQUAL(static_cast<int>(UGH_LOGIC_EVENT_LEVEL_CAPTION), events[0]);
    ugh_logic_view view;
    ugh_logic_get_view(logic, &view);
    CHECK_EQUAL(static_cast<int>(UGH_LOGIC_PHASE_CAPTION), view.phase);
    CHECK_EQUAL(0, view.level_id);
    CHECK_EQUAL(ugh::world::Lives::START, view.lives);
    CHECK_EQUAL(1, view.copter_count);
    CHECK_EQUAL(4608, view.copters[0].x);
    CHECK_EQUAL(4, view.entity_count);   // two passengers with a route, the standing one, the tree
    CHECK_EQUAL(static_cast<int>(UGH_LOGIC_ENTITY_ENEMY), view.entities[3].kind);
    CHECK_EQUAL(0, view.raindrop_count);
    ugh_logic_destroy(logic);
}
