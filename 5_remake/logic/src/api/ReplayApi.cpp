// The C API (include/ugh_logic.h) of the replays of a level.
#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#include "api/Handles.hpp"
#include "record/ReplayCodec.hpp"
#include "record/ReplayText.hpp"
#include "ugh_logic.h"

namespace {

static_assert(ugh::record::Recording::LOGIC_VERSION == UGH_LOGIC_VERSION);
static_assert(ugh::record::Recording::MAX_TEXT + 1 == UGH_REPLAY_TEXT_SIZE);
static_assert(ugh::record::Recording::MAX_NAMES == 2);

ugh::game::AttemptStart fromApi(const ugh_logic_attempt_start& s) {
    ugh::game::AttemptStart start;
    start.players = s.players;
    start.difficulty = static_cast<ugh::data::Difficulty>(s.difficulty);
    start.level = s.level;
    start.lives = s.lives;
    start.points = s.score;
    start.multiplier = s.multiplier;
    std::copy(std::begin(s.random), std::end(s.random), start.random.begin());
    start.rainFloorRow = s.rain_floor_row;
    start.effort = {s.effort[0], s.effort[1]};
    start.lastMenuKey = static_cast<ugh::input::MenuKey>(s.menu_key);
    return start;
}

ugh_logic_attempt_start toApi(const ugh::game::AttemptStart& start) {
    ugh_logic_attempt_start s{};
    s.players = start.players;
    s.difficulty = static_cast<int>(start.difficulty);
    s.level = start.level;
    s.lives = start.lives;
    s.score = start.points;
    s.multiplier = start.multiplier;
    std::copy(start.random.begin(), start.random.end(), std::begin(s.random));
    s.rain_floor_row = start.rainFloorRow;
    s.effort[0] = start.effort[0];
    s.effort[1] = start.effort[1];
    s.menu_key = static_cast<int>(start.lastMenuKey);
    return s;
}

void copyText(const std::string& text, char (&out)[UGH_REPLAY_TEXT_SIZE]) {
    size_t n = std::min(text.size(), sizeof out - 1);
    std::memcpy(out, text.data(), n);
    out[n] = 0;
}

/** What `bytes` are into `out` if they fit; their size. */
size_t give(const std::vector<uint8_t>& bytes, void* out, size_t capacity) {
    if (out && capacity >= bytes.size()) std::memcpy(out, bytes.data(), bytes.size());
    return bytes.size();
}

}  // namespace

extern "C" {

uint32_t ugh_logic_data_hash(const ugh_logic* logic) { return logic->dataHash; }

int ugh_logic_get_attempt_start(const ugh_logic* logic, ugh_logic_attempt_start* start) {
    *start = ugh_logic_attempt_start{};
    if (!logic->started || !logic->game.attemptStart()) return 0;
    *start = toApi(*logic->game.attemptStart());
    return 1;
}

int ugh_logic_resume_game(ugh_logic* logic, const ugh_logic_attempt_start* start) {
    if (start->difficulty < 0 || start->difficulty > 2 || start->menu_key < UGH_LOGIC_MENU_ESCAPE ||
        start->menu_key > UGH_LOGIC_MENU_OTHER || !logic->game.resume(fromApi(*start)))
        return 0;
    logic->watched.reset();
    logic->startedGame(true);
    return 1;
}

ugh_replay* ugh_logic_get_replay(const ugh_logic* logic, int which) {
    const std::optional<ugh::record::Recording>& recording =
        which == UGH_REPLAY_LEVEL_ENDED ? logic->recorder.finished() : logic->recorder.current();
    return logic->started && recording ? new ugh_replay{*recording} : nullptr;
}

void ugh_replay_destroy(ugh_replay* replay) { delete replay; }

void ugh_replay_get_info(const ugh_replay* replay, ugh_replay_info* info) {
    const ugh::record::Recording& r = replay->recording;
    *info = ugh_replay_info{};
    info->logic_version = r.logicVersion;
    info->data_hash = r.dataHash;
    info->start = toApi(r.start);
    info->steps = r.steps;
    info->play_steps = r.playSteps;
    info->attempts = r.attempts;
    info->points = r.points;
    info->done = r.done ? 1 : 0;
    info->date = r.date;
    copyText(r.password, info->password);
    info->name_count = static_cast<int>(r.names.size());
    for (size_t i = 0; i < r.names.size() && i < 2; i++) copyText(r.names[i], info->names[i]);
    info->input_count = static_cast<int>(r.inputs.size());
}

void ugh_replay_set_label(ugh_replay* replay, const char* password, const char* name1, const char* name2, int64_t date) {
    ugh::record::Recording& r = replay->recording;
    r.password = password ? password : "";
    r.names.clear();
    if (name1) r.names.emplace_back(name1);
    if (name2) r.names.emplace_back(name2);
    r.date = date;
    // as the file keeps them (cut, nothing below a space): a replay read back is the same
    std::string ignored;
    if (std::optional<ugh::record::Recording> kept = ugh::record::ReplayCodec::read(ugh::record::ReplayCodec::write(r), ignored))
        r = *kept;
}

size_t ugh_replay_write(const ugh_replay* replay, void* out, size_t capacity) {
    return give(ugh::record::ReplayCodec::write(replay->recording), out, capacity);
}

size_t ugh_replay_write_text(const ugh_replay* replay, char* out, size_t capacity) {
    const std::string text = ugh::record::ReplayText::write(replay->recording);
    if (out && capacity > text.size()) {
        std::memcpy(out, text.data(), text.size());
        out[text.size()] = 0;
    }
    return text.size();
}

ugh_replay* ugh_replay_read(const void* bytes, size_t size, char* err, size_t err_size) {
    std::string error;
    std::optional<ugh::record::Recording> recording =
        ugh::record::ReplayText::read({static_cast<const uint8_t*>(bytes), size}, error);
    if (!recording) {
        if (err && err_size) {
            size_t n = std::min(error.size(), err_size - 1);
            std::memcpy(err, error.data(), n);
            err[n] = 0;
        }
        return nullptr;
    }
    return new ugh_replay{std::move(*recording)};
}

int ugh_replay_better(const ugh_replay* replay, const ugh_replay* than) {
    return replay->recording.betterThan(than ? &than->recording : nullptr) ? 1 : 0;
}

int ugh_replay_compare_logic(const ugh_replay* replay, const ugh_logic* logic) {
    return (replay->recording.logicVersion != UGH_LOGIC_VERSION ? UGH_REPLAY_OTHER_VERSION : 0) |
           (replay->recording.dataHash != logic->dataHash ? UGH_REPLAY_OTHER_DATA : 0);
}

int ugh_logic_watch(ugh_logic* logic, const ugh_replay* replay) {
    ugh::record::Playback playback(replay->recording);
    if (!playback.start(logic->game)) return 0;
    logic->watched.emplace(std::move(playback));
    logic->startedGame(true);
    return 1;
}

int ugh_logic_watched_steps(const ugh_logic* logic) { return logic->watched ? logic->watched->stepsMade() : -1; }

}
