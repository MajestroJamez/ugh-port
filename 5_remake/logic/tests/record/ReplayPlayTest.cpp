#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "TestFramework.hpp"
#include "ugh_logic.h"

namespace {

/** A view as a number: the same view, the same number (FNV-1a of its bytes; it has no padding). */
uint64_t hashOf(const ugh_logic_view& view) {
    uint64_t hash = 14695981039346656037ull;
    const auto* bytes = reinterpret_cast<const uint8_t*>(&view);
    for (size_t i = 0; i < sizeof view; i++) hash = (hash ^ bytes[i]) * 1099511628211ull;
    return hash;
}

/** A pilot that holds random keys for a random while, as a frontend gives them (every key event to the game loop too). */
class RandomPilot {
public:
    explicit RandomPilot(uint32_t seed) : state_(seed) {}
    void fly(ugh_logic* logic, int players) {
        for (int player = 0; player < players; player++) {
            for (int key = UGH_LOGIC_KEY_UP; key <= UGH_LOGIC_KEY_FIRE; key++) {
                if (next() % (key == UGH_LOGIC_KEY_UP ? 12 : 30) != 0) continue;
                held_[player][key] = !held_[player][key];
                ugh_logic_key(logic, player, key, held_[player][key] ? 1 : 0);
                ugh_logic_menu_key(logic, UGH_LOGIC_MENU_OTHER);
            }
        }
    }

private:
    uint32_t state_;
    bool held_[2][5] = {};
    uint32_t next() { return state_ = state_ * 1664525u + 1013904223u, state_ >> 8; }
};

/** A game played by the random pilot: its views step by step, the replays of its levels and their first steps. */
struct Played {
    std::vector<uint64_t> views;
    std::vector<int> results;       // of ugh_logic_step
    std::vector<ugh_replay*> replays;
    std::map<int, int> firstStep;   // a level's: the step its first attempt started in
};

Played play(ugh_logic* logic, const ugh_logic_settings& settings, int maxSteps, uint32_t seed) {
    Played played;
    CHECK_EQUAL(1, ugh_logic_new_game(logic, &settings));
    RandomPilot pilot(seed);
    int level = -1, status = UGH_LOGIC_CONTINUE;
    for (int step = 0; step < maxSteps && status == UGH_LOGIC_CONTINUE; step++) {
        pilot.fly(logic, settings.players);
        status = ugh_logic_step(logic);
        ugh_logic_take_events(logic, [](void*, const ugh_logic_event*) {}, nullptr);
        ugh_logic_view view;
        ugh_logic_get_view(logic, &view);
        played.views.push_back(hashOf(view));
        played.results.push_back(status);
        ugh_logic_attempt_start start;
        if (ugh_logic_get_attempt_start(logic, &start) && start.level != level) {
            if (level >= 0) played.replays.push_back(ugh_logic_get_replay(logic, UGH_REPLAY_LEVEL_ENDED));
            level = start.level;
            played.firstStep[level] = step;
        }
    }
    played.replays.push_back(ugh_logic_get_replay(logic, status == UGH_LOGIC_CONTINUE ? UGH_REPLAY_LEVEL_PLAYED
                                                                                       : UGH_REPLAY_LEVEL_ENDED));
    return played;
}

/** The replay shared as text, read back, watched on another logic: the same views as the game it was recorded in. */
void watchAgain(ugh_logic* other, const Played& played, const ugh_replay* replay) {
    std::vector<char> text(ugh_replay_write_text(replay, nullptr, 0) + 1);
    ugh_replay_write_text(replay, text.data(), text.size());
    char err[200] = "";
    ugh_replay* read = ugh_replay_read(text.data(), std::strlen(text.data()), err, sizeof err);
    CHECK(read != nullptr);
    if (!read) return;
    ugh_replay_info info, readInfo;
    ugh_replay_get_info(replay, &info);
    ugh_replay_get_info(read, &readInfo);
    CHECK(std::memcmp(&info, &readInfo, sizeof info) == 0);
    CHECK_EQUAL(UGH_REPLAY_SAME_LOGIC, ugh_replay_compare_logic(read, other));
    CHECK_EQUAL(1, ugh_logic_watch(other, read));
    const int first = played.firstStep.at(info.start.level);
    int status = UGH_LOGIC_CONTINUE;
    for (int step = 0; step < info.steps; step++) {
        CHECK_EQUAL(step, ugh_logic_watched_steps(other));
        ugh_logic_key(other, 0, UGH_LOGIC_KEY_UP, 1);   // (ignored while watching)
        status = ugh_logic_step(other);
        ugh_logic_view view;
        ugh_logic_get_view(other, &view);
        if (hashOf(view) != played.views[first + step]) {
            CHECK_EQUAL(played.views[first + step], hashOf(view));
            break;
        }
        ugh_logic_attempt_start start;
        if (step == 0) CHECK(ugh_logic_get_attempt_start(other, &start) && std::memcmp(&start, &info.start, sizeof start) == 0);
    }
    CHECK_EQUAL(played.results[first + info.steps - 1], status);
    CHECK_EQUAL(static_cast<int>(UGH_LOGIC_REPLAY_OVER), ugh_logic_step(other));
    // what the logic recorded while watching is the replay again
    ugh_replay* again = ugh_logic_get_replay(other, info.done || status != UGH_LOGIC_CONTINUE ? UGH_REPLAY_LEVEL_ENDED
                                                                                               : UGH_REPLAY_LEVEL_PLAYED);
    CHECK(again != nullptr);
    if (again) {
        std::vector<uint8_t> a(ugh_replay_write(replay, nullptr, 0)), b(ugh_replay_write(again, nullptr, 0));
        ugh_replay_write(replay, a.data(), a.size());
        ugh_replay_write(again, b.data(), b.size());
        CHECK(a == b);
        ugh_replay_destroy(again);
    }
    ugh_replay_destroy(read);
}

}  // namespace

TEST(a_replay_of_a_level_plays_again_as_it_was_played) {
    char err[200] = "";
    ugh_logic* logic = ugh_logic_create(ugh::test::dataPath().c_str(), err, sizeof err);
    ugh_logic* other = ugh_logic_create(ugh::test::dataPath().c_str(), err, sizeof err);
    CHECK(logic && other);
    if (!logic || !other) return;
    CHECK_EQUAL(ugh_logic_data_hash(logic), ugh_logic_data_hash(other));
    // both modes, every difficulty, levels from the first to late ones, windy ones (rain, its random numbers)
    const int games[][4] = {{1, 1, 0, 11}, {1, 0, 24, 21}, {1, 2, 59, 22}, {2, 1, 0, 3}, {2, 2, 49, 4}, {1, 1, 10, 5},
                            {2, 0, 30, 6}, {1, 2, 68, 7}};
    int replays = 0, attempts = 0;
    for (const auto& g : games) {
        ugh_logic_settings settings;
        ugh_logic_default_settings(&settings);
        settings.players = g[0];
        settings.difficulty = g[1];
        settings.first_level = g[2];
        settings.random_seed[0] = static_cast<uint16_t>(g[3] * 977);
        Played played = play(logic, settings, 9000, static_cast<uint32_t>(g[3]));
        for (ugh_replay* replay : played.replays) {
            CHECK(replay != nullptr);
            if (!replay) continue;
            ugh_replay_info info;
            ugh_replay_get_info(replay, &info);
            CHECK_EQUAL(UGH_LOGIC_VERSION, info.logic_version);
            CHECK_EQUAL(ugh_logic_data_hash(logic), info.data_hash);
            CHECK_EQUAL(g[0], info.start.players);
            CHECK_EQUAL(g[1], info.start.difficulty);
            CHECK(info.play_steps > 0 && info.play_steps < info.steps);
            watchAgain(other, played, replay);
            replays++;
            attempts += info.attempts;
            ugh_replay_destroy(replay);
        }
    }
    CHECK(replays >= 8);
    CHECK(attempts > replays);   // crashes: attempts resumed with fewer lives, the random numbers on
    ugh_logic_destroy(logic);
    ugh_logic_destroy(other);
}

TEST(a_replay_tells_another_logic_and_a_level_the_data_has_not) {
    char err[200] = "";
    ugh_logic* logic = ugh_logic_create(ugh::test::dataPath().c_str(), err, sizeof err);
    CHECK(logic != nullptr);
    if (!logic) return;
    ugh_logic_settings settings;
    ugh_logic_default_settings(&settings);
    CHECK(ugh_logic_get_replay(logic, UGH_REPLAY_LEVEL_PLAYED) == nullptr);   // no game
    Played played = play(logic, settings, 400, 1);
    ugh_replay* replay = played.replays.back();
    CHECK(replay != nullptr);
    if (!replay) return;
    ugh_replay_set_label(replay, "", "Jan", nullptr, 1791000000);
    std::vector<uint8_t> file(ugh_replay_write(replay, nullptr, 0));
    ugh_replay_write(replay, file.data(), file.size());
    ugh_replay* read = ugh_replay_read(file.data(), file.size(), err, sizeof err);
    CHECK(read != nullptr);
    ugh_replay_info info;
    ugh_replay_get_info(read, &info);
    CHECK_EQUAL(1, info.name_count);
    CHECK_EQUAL(std::string("Jan"), std::string(info.names[0]));
    CHECK_EQUAL(int64_t{1791000000}, info.date);
    CHECK_EQUAL(0, info.done);
    // the not done is never the best; a resumed game takes only what the data has
    CHECK_EQUAL(0, ugh_replay_better(read, nullptr));
    ugh_logic_attempt_start start = info.start;
    start.level = 69;
    CHECK_EQUAL(0, ugh_logic_resume_game(logic, &start));
    start = info.start;
    start.multiplier = 0;
    CHECK_EQUAL(0, ugh_logic_resume_game(logic, &start));
    start = info.start;
    CHECK_EQUAL(1, ugh_logic_resume_game(logic, &start));
    // made with other data: the hash in the file (its checksum made again)
    std::vector<uint8_t> other = file;
    other[6] ^= 1;   // (the logic's version is one byte: the hash follows it)
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i + 4 < other.size(); i++) {
        crc ^= other[i];
        for (int k = 0; k < 8; k++) crc = crc & 1 ? 0xEDB88320u ^ (crc >> 1) : crc >> 1;
    }
    crc ^= 0xFFFFFFFFu;
    for (int k = 0; k < 4; k++) other[other.size() - 4 + k] = static_cast<uint8_t>(crc >> (8 * k));
    ugh_replay* foreign = ugh_replay_read(other.data(), other.size(), err, sizeof err);
    CHECK(foreign != nullptr);
    if (foreign) CHECK_EQUAL(static_cast<int>(UGH_REPLAY_OTHER_DATA), ugh_replay_compare_logic(foreign, logic));
    other[5] = 2;   // the logic's version 2, its hash as it was
    other[6] ^= 1;
    crc = 0xFFFFFFFFu;
    for (size_t i = 0; i + 4 < other.size(); i++) {
        crc ^= other[i];
        for (int k = 0; k < 8; k++) crc = crc & 1 ? 0xEDB88320u ^ (crc >> 1) : crc >> 1;
    }
    crc ^= 0xFFFFFFFFu;
    for (int k = 0; k < 4; k++) other[other.size() - 4 + k] = static_cast<uint8_t>(crc >> (8 * k));
    ugh_replay* newer = ugh_replay_read(other.data(), other.size(), err, sizeof err);
    CHECK(newer != nullptr);
    if (newer) CHECK_EQUAL(static_cast<int>(UGH_REPLAY_OTHER_VERSION), ugh_replay_compare_logic(newer, logic));
    ugh_replay_destroy(newer);
    ugh_replay_destroy(foreign);
    ugh_replay_destroy(read);
    ugh_replay_destroy(replay);
    ugh_logic_destroy(logic);
}
