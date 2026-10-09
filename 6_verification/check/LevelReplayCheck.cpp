#include "check/LevelReplayCheck.hpp"

#include <memory>
#include <span>

#include "check/ReplayFile.hpp"
#include "record/Playback.hpp"
#include "record/ReplayText.hpp"
#include "replay/StateWriter.hpp"

namespace ugh::check {

namespace {

/** A cut replay being played along the golden one. */
struct Playing {
    long long from;   // the golden tick of its first step
    std::unique_ptr<game::Game> game;
    record::Playback playback;
};

/** The bonus items of the attempt before stay in a game until its play (a resumed one has none). */
bool comparable(const std::string& field, bool play) { return play || field.rfind("bonus.", 0) != 0; }

}  // namespace

LevelReplayCheck::LevelReplayCheck(const data::GameData& data, const std::vector<keyboard::KeyBinding>& keys,
                                   ReplayReport& report)
    : data_(data), keys_(keys), report_(report), recorder_(0) {}

void LevelReplayCheck::run(const std::string& path) {
    ReplayCheck::Options options;
    options.observer = this;
    ReplayReport golden;   // (the golden check itself is replay_check's)
    ReplayCheck(data_, keys_, options, golden).run(path);
    if (!golden.passed()) {
        report_.problem(0, "the golden replay does not pass: " + golden.text(path));
        return;
    }
    if (recorder_.current()) keep(*recorder_.current());
    cutAttempts();
    playCut(path);
}

void LevelReplayCheck::started(game::Game& game) {
    game.addListener(recorder_);
    game.addListener(*this);
}

void LevelReplayCheck::key(int player, input::PlayerKey key, bool pressed) { recorder_.key(player, key, pressed); }

void LevelReplayCheck::menuKey(input::MenuKey key) { recorder_.menuKey(key); }

void LevelReplayCheck::onEvent(const events::Event& event) {
    if (event.kind == events::EventKind::LevelCaption) attemptStarted_ = true;
}

void LevelReplayCheck::stepped(const game::Game& game, game::GameResult result, long long tick) {
    recorder_.stepped(game, result);
    if (attemptStarted_ && game.attemptStart()) attempts_.push_back({tick, *game.attemptStart()});
    attemptStarted_ = false;
    if (recorder_.finished()) keep(*recorder_.finished());
}

void LevelReplayCheck::keep(const record::Recording& recording) {
    if (!levels_.empty() && levels_.back().recording.start == recording.start) return;
    for (const Attempt& attempt : attempts_) {
        if (attempt.start == recording.start) {
            levels_.push_back({attempt.tick, recording});
            return;
        }
    }
    report_.problem(0, "a recording of level " + std::to_string(recording.start.level + 1) + " from no attempt");
}

/** An attempt of a level's recording from its step on: its start, the inputs after it, the steps left. */
void LevelReplayCheck::cutAttempts() {
    for (const Level& level : levels_) {
        const record::Recording& whole = level.recording;
        int before = 0;   // the level's attempts before this one
        for (const Attempt& attempt : attempts_) {
            const long long offset = attempt.tick - level.tick;
            if (offset < 0 || offset >= whole.steps || attempt.start.level != whole.start.level) continue;
            before++;
            if (attempt.tick <= lastIntervention_) continue;
            record::Recording cut = whole;
            cut.start = attempt.start;
            cut.inputs.clear();
            for (record::Input input : whole.inputs) {
                if (input.after < offset) continue;
                input.after -= static_cast<int>(offset);
                cut.inputs.push_back(input);
            }
            cut.steps = whole.steps - static_cast<int>(offset);
            cut.attempts = whole.attempts - (before - 1);
            cut.points = whole.points - (attempt.start.points - whole.start.points);
            cut.playSteps = 0;   // (not played again)
            // as a player shares it: a line of text, read back the same
            std::string text = record::ReplayText::write(cut), error;
            std::optional<record::Recording> read = record::ReplayText::read(
                std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(text.data()), text.size()), error);
            if (!read || !(*read == cut)) {
                report_.problem(attempt.tick, "the replay of the attempt does not read back the same: " + error);
                continue;
            }
            cut_.push_back({attempt.tick, *read});
        }
    }
}

void LevelReplayCheck::playCut(const std::string& path) {
    ReplayFile file(path);
    if (!file.open()) {
        report_.unreadable(file.error());
        return;
    }
    std::vector<Playing> playing;
    size_t next = 0;
    ReplayFile::Tick tick;
    while (file.next(tick)) {
        for (; next < cut_.size() && cut_[next].tick == tick.number; next++) {
            Playing p{tick.number, std::make_unique<game::Game>(data_), record::Playback(cut_[next].recording)};
            if (!p.playback.start(*p.game)) report_.problem(tick.number, "the game refuses the replay's start");
            else playing.push_back(std::move(p));
        }
        auto phase = tick.state.find("game.phase");
        const bool play = phase != tick.state.end() && phase->second == "play";
        for (Playing& p : playing) {
            if (p.playback.over()) continue;
            for (const record::Input& input : p.playback.due()) {
                if (input.kind == record::Input::Kind::PilotKey) p.game->key(input.player, input.key, input.pressed);
                else p.game->menuKey(input.menuKey);
            }
            p.game->step();
            p.playback.stepped();
            report_.tick();
            replay::Fields actual = replay::StateWriter::write(*p.game);
            std::string diffs;
            for (const auto& [name, expected] : tick.state) {
                if (!comparable(name, play)) continue;
                auto it = actual.find(name);
                report_.compared(name.substr(0, name.find('.')));
                if (it == actual.end() || it->second != expected)
                    diffs += " " + name + " expected " + expected + ", got " + (it == actual.end() ? "none" : it->second);
            }
            for (const auto& [name, value] : actual)
                if (comparable(name, play) && !tick.state.count(name)) diffs += " " + name + " not expected, got " + value;
            if (!diffs.empty()) {
                report_.problem(tick.number, "the replay from tick " + std::to_string(p.from) + ":" + diffs);
                p.playback = record::Playback(record::Recording());   // (over: one report a replay)
            }
        }
    }
    if (!file.error().empty()) report_.unreadable(file.error());
}

}  // namespace ugh::check
