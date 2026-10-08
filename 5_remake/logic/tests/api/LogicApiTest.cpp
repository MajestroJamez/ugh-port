#include <string>
#include <vector>

#include "TestFramework.hpp"
#include "ugh_logic.h"
#include "world/session/Session.hpp"

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
    CHECK_EQUAL(ugh::world::session::Lives::START, view.lives);
    CHECK_EQUAL(1, view.copter_count);
    CHECK_EQUAL(4608, view.copters[0].x);
    CHECK_EQUAL(4, view.entity_count);   // two passengers with a route, the standing one, the tree
    CHECK_EQUAL(static_cast<int>(UGH_LOGIC_ENTITY_ENEMY), view.entities[3].kind);
    CHECK_EQUAL(0, view.raindrop_count);
    CHECK_EQUAL(0, view.wind);
    ugh_logic_destroy(logic);
}

TEST(the_c_api_shows_the_background_of_the_level) {
    char err[200] = "";
    ugh_logic* logic = ugh_logic_create(ugh::test::dataPath().c_str(), err, sizeof err);
    CHECK(logic != nullptr);
    if (!logic) return;
    CHECK_EQUAL(0, ugh_logic_pad_count(logic));   // no level yet
    ugh_logic_settings settings;
    ugh_logic_default_settings(&settings);
    CHECK_EQUAL(1, ugh_logic_new_game(logic, &settings));
    for (int frame = 0; frame < 9; frame++) ugh_logic_step(logic);   // the caption loads the first level
    CHECK_EQUAL(3, ugh_logic_pad_count(logic));
    ugh_logic_pad pad{};
    CHECK_EQUAL(1, ugh_logic_get_pad(logic, 0, &pad));
    CHECK_EQUAL(208, pad.left);
    CHECK_EQUAL(320, pad.right);
    CHECK_EQUAL(36, pad.y);
    CHECK_EQUAL(1, pad.number);
    CHECK_EQUAL(0, ugh_logic_get_pad(logic, 3, &pad));
    int solid = 0;
    for (int y = 0; y < UGH_LOGIC_SCREEN_HEIGHT; y++)
        for (int x = 0; x < UGH_LOGIC_SCREEN_WIDTH; x++) solid += ugh_logic_solid(logic, x, y);
    CHECK(solid > 0 && solid < UGH_LOGIC_SCREEN_WIDTH * UGH_LOGIC_SCREEN_HEIGHT);
    CHECK_EQUAL(0, ugh_logic_solid(logic, -1, 0));
    ugh_logic_destroy(logic);
}

TEST(the_c_api_shows_the_wind_and_its_rain) {
    char err[200] = "";
    ugh_logic* logic = ugh_logic_create(ugh::test::dataPath().c_str(), err, sizeof err);
    CHECK(logic != nullptr);
    if (!logic) return;
    ugh_logic_settings settings;
    ugh_logic_default_settings(&settings);
    settings.first_level = 42;   // level 43 of one player: the wind blows to the left
    CHECK_EQUAL(1, ugh_logic_new_game(logic, &settings));
    for (int frame = 0; frame < 9; frame++) ugh_logic_step(logic);
    ugh_logic_view view;
    ugh_logic_get_view(logic, &view);
    CHECK_EQUAL(44, view.level_id);
    CHECK_EQUAL(-1, view.wind);
    CHECK(view.raindrop_count > 0);
    ugh_logic_destroy(logic);
}

TEST(the_c_api_shows_who_a_passenger_is_and_whether_an_enemy_is_knocked_out) {
    char err[200] = "";
    ugh_logic* logic = ugh_logic_create(ugh::test::dataPath().c_str(), err, sizeof err);
    CHECK(logic != nullptr);
    if (!logic) return;
    ugh_logic_settings settings;
    ugh_logic_default_settings(&settings);
    CHECK_EQUAL(1, ugh_logic_new_game(logic, &settings));
    for (int frame = 0; frame < 9; frame++) ugh_logic_step(logic);
    ugh_logic_view view;
    ugh_logic_get_view(logic, &view);
    CHECK_EQUAL(4, view.entity_count);   // two passengers of kind1 (look 1), the standing one (look 4), the tree
    CHECK_EQUAL(1, view.entities[0].look);
    CHECK_EQUAL(1, view.entities[1].look);
    CHECK_EQUAL(4, view.entities[2].look);
    CHECK_EQUAL(0, view.entities[3].look);
    for (int i = 0; i < view.entity_count; i++) CHECK_EQUAL(0, view.entities[i].stunned);
    ugh_logic_destroy(logic);
}

TEST(the_c_api_names_the_sprites_of_the_entities) {
    char err[200] = "";
    ugh_logic* logic = ugh_logic_create(ugh::test::dataPath().c_str(), err, sizeof err);
    CHECK(logic != nullptr);
    if (!logic) return;
    ugh_logic_sprite info;
    CHECK_EQUAL(1, ugh_logic_get_sprite(logic, 395, &info));   // works before a game too
    CHECK_EQUAL(std::string("kind1.walkRight"), std::string(info.name));
    CHECK_EQUAL(2, info.frame);
    CHECK_EQUAL(9, info.frames);
    CHECK_EQUAL(1, ugh_logic_get_sprite(logic, 378, &info));   // kind1.standing: 377, 377, 378, 378
    CHECK_EQUAL(std::string("kind1.standing"), std::string(info.name));
    CHECK_EQUAL(2, info.frame);
    CHECK_EQUAL(1, ugh_logic_get_sprite(logic, 234, &info));   // the flyer's first flight frame, also its fall
    CHECK_EQUAL(std::string("flyer.left"), std::string(info.name));
    CHECK_EQUAL(1, ugh_logic_get_sprite(logic, 544, &info));
    CHECK_EQUAL(std::string("standingPassenger"), std::string(info.name));
    CHECK_EQUAL(1, info.frames);
    CHECK_EQUAL(1, ugh_logic_get_sprite(logic, 297, &info));
    CHECK_EQUAL(std::string("multiplier"), std::string(info.name));
    CHECK_EQUAL(1, ugh_logic_get_sprite(logic, 270, &info));   // a speech bubble: to the third pad
    CHECK_EQUAL(std::string("destinationBubble"), std::string(info.name));
    CHECK_EQUAL(2, info.frame);
    CHECK_EQUAL(6, info.frames);
    CHECK_EQUAL(1, ugh_logic_get_sprite(logic, 274, &info));
    CHECK_EQUAL(std::string("impatientBubble"), std::string(info.name));
    CHECK_EQUAL(0, ugh_logic_get_sprite(logic, 218, &info));   // a rotor
    ugh_logic_destroy(logic);
}

TEST(the_c_api_shows_a_copters_danger_at_the_crash_limit_of_the_difficulty) {
    char err[200] = "";
    ugh_logic* logic = ugh_logic_create(ugh::test::dataPath().c_str(), err, sizeof err);
    CHECK(logic != nullptr);
    if (!logic) return;
    ugh_logic_copter_danger danger;
    CHECK_EQUAL(0, ugh_logic_get_copter_danger(logic, 0, &danger));   // no game yet
    const int limits[] = {3100, 2300, 1380};   // easy, medium, hard: the original's
    for (int difficulty = 0; difficulty < 3; difficulty++) {
        ugh_logic_settings settings;
        ugh_logic_default_settings(&settings);
        settings.difficulty = difficulty;
        CHECK_EQUAL(1, ugh_logic_new_game(logic, &settings));
        for (int frame = 0; frame < 9; frame++) ugh_logic_step(logic);   // level 1 is loaded
        CHECK_EQUAL(1, ugh_logic_get_copter_danger(logic, 0, &danger));
        CHECK_EQUAL(0, ugh_logic_get_copter_danger(logic, 1, &danger));   // one player
        CHECK_EQUAL(1, ugh_logic_get_copter_danger(logic, 0, &danger));
        CHECK_EQUAL(limits[difficulty], danger.crash_limit);
        CHECK_EQUAL(0, danger.speed_x);   // at its start
        CHECK_EQUAL(0, danger.rock_x);
    }
    ugh_logic_destroy(logic);
}
