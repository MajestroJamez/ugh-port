// The C API (include/ugh_logic.h) over the game.
#include "ugh_logic.h"

#include <algorithm>
#include <cstring>
#include <memory>
#include <optional>
#include <string>

#include "data/ugd/DataFileReader.hpp"
#include "events/EventQueue.hpp"
#include "game/Game.hpp"
#include "passengers/Passenger.hpp"

struct ugh_logic {
    std::unique_ptr<const ugh::data::GameData> data;
    ugh::game::Game game;
    ugh::events::EventQueue events;
    bool started = false;   // a new game was set up

    explicit ugh_logic(std::unique_ptr<const ugh::data::GameData> loaded) : data(std::move(loaded)), game(*data) {
        game.addListener(events);
    }
};

namespace {

// the keys of the C API are the logic's keys in the same order
static_assert(static_cast<int>(ugh::input::PlayerKey::Fire) == UGH_LOGIC_KEY_FIRE);
static_assert(static_cast<int>(ugh::input::MenuKey::Other) == UGH_LOGIC_MENU_OTHER);

int eventKind(ugh::events::EventKind kind) { return static_cast<int>(kind) + UGH_LOGIC_EVENT_LEVEL_CAPTION; }

int phase(ugh::game::GamePhase p) {
    switch (p) {
        case ugh::game::GamePhase::Start: return UGH_LOGIC_PHASE_START;
        case ugh::game::GamePhase::BetweenLevels: return UGH_LOGIC_PHASE_BETWEEN_LEVELS;
        case ugh::game::GamePhase::Caption: return UGH_LOGIC_PHASE_CAPTION;
        case ugh::game::GamePhase::Setup: return UGH_LOGIC_PHASE_SETUP;
        case ugh::game::GamePhase::Play: return UGH_LOGIC_PHASE_PLAY;
    }
    return UGH_LOGIC_PHASE_START;
}

void addEntity(ugh_logic_view& view, int kind, int index, ugh::units::Fixed x, ugh::units::Fixed y,
               std::optional<int> sprite, std::optional<int> bubble) {
    if (view.entity_count >= UGH_LOGIC_MAX_ENTITIES) return;
    view.entities[view.entity_count++] = {kind, index, x.raw(), y.raw(), sprite.value_or(-1), bubble.value_or(-1)};
}

/** The level and its sprites: copters, passengers, enemies, bonus items, rain. */
void viewLevel(const ugh::game::Game& game, ugh_logic_view& view) {
    const ugh::world::Level& level = game.level();
    view.level_id = level.definition()->id;
    view.energy = level.energy().value();
    view.fade = level.fade().position();
    view.water_level = level.water().level().raw();
    view.water_frame = level.water().surfaceFrame();
    view.copter_count = level.copters().count();
    for (const ugh::world::copter::Copter& c : level.copters().all()) {
        const auto& cargo = c.cabin().cargo();
        int destination = !cargo ? 0 : cargo->destination ? *cargo->destination : -1;
        view.copters[c.player()] = {c.motion().x().raw(), c.motion().y().raw(), c.rotor().sprite(), cargo ? cargo->look : 0,
                           destination, c.cabin().fare()};
    }
    for (int i = 0; i < game.passengers().count(); i++) {
        const ugh::passengers::Passenger& passenger = game.passengers()[i];
        addEntity(view, UGH_LOGIC_ENTITY_PASSENGER, i, passenger.x(), passenger.y(), passenger.sprite(), passenger.bubble());
    }
    for (int i = 0; i < game.enemies().count(); i++) {
        const ugh::enemies::Enemy& enemy = game.enemies()[i];
        addEntity(view, UGH_LOGIC_ENTITY_ENEMY, i, enemy.x(), enemy.y(), enemy.sprite(), std::nullopt);
    }
    for (int slot = 0; slot < ugh::bonuses::BonusSlots::SLOTS; slot++) {
        const auto& item = game.bonuses()[slot];
        if (item) addEntity(view, UGH_LOGIC_ENTITY_BONUS_ITEM, slot, item->x(), item->y(), item->sprite(), std::nullopt);
    }
    if (!level.windy()) return;
    for (const ugh::world::scenery::Raindrop& drop : level.rain().drops()) {
        if (!drop.onScreen()) continue;
        view.raindrops[view.raindrop_count][0] = drop.x;
        view.raindrops[view.raindrop_count][1] = drop.y;
        view.raindrop_count++;
    }
}

int result(ugh::game::GameResult r) {
    switch (r) {
        case ugh::game::GameResult::Continue: return UGH_LOGIC_CONTINUE;
        case ugh::game::GameResult::GameOver: return UGH_LOGIC_GAME_OVER;
        case ugh::game::GameResult::AllLevelsDone: return UGH_LOGIC_ALL_LEVELS_DONE;
    }
    return UGH_LOGIC_GAME_OVER;
}

}  // namespace

extern "C" {

ugh_logic* ugh_logic_create(const char* data_path, char* err, size_t err_size) {
    std::string error;
    auto data = ugh::data::ugd::DataFileReader::read(data_path, error);
    if (!data) {
        if (err && err_size) {
            size_t n = std::min(error.size(), err_size - 1);
            std::memcpy(err, error.data(), n);
            err[n] = 0;
        }
        return nullptr;
    }
    return new ugh_logic(std::move(data));
}

void ugh_logic_destroy(ugh_logic* logic) { delete logic; }

void ugh_logic_default_settings(ugh_logic_settings* settings) {
    const ugh::game::NewGameSettings defaults;
    *settings = ugh_logic_settings{};
    settings->players = defaults.players;
    settings->difficulty = static_cast<int>(defaults.difficulty);
    settings->first_level = defaults.firstLevel;
    for (int i = 0; i < 4; i++) settings->random_seed[i] = defaults.randomSeed[i];
    settings->rain_floor_row = defaults.rainFloorRow;
}

int ugh_logic_new_game(ugh_logic* logic, const ugh_logic_settings* settings) {
    if (settings->difficulty < 0 || settings->difficulty > 2) return 0;
    ugh::game::NewGameSettings s;
    s.players = settings->players;
    s.difficulty = static_cast<ugh::data::Difficulty>(settings->difficulty);
    s.firstLevel = settings->first_level;
    for (int i = 0; i < 4; i++) s.randomSeed[i] = settings->random_seed[i];
    s.rainFloorRow = settings->rain_floor_row;
    if (!logic->game.newGame(s)) return 0;
    logic->events.take();
    logic->started = true;
    return 1;
}

void ugh_logic_key(ugh_logic* logic, int player, int key, int pressed) {
    if (!logic->started || player < 0 || player > 1 || key < UGH_LOGIC_KEY_UP || key > UGH_LOGIC_KEY_FIRE) return;
    logic->game.key(player, static_cast<ugh::input::PlayerKey>(key), pressed != 0);
}

void ugh_logic_menu_key(ugh_logic* logic, int key) {
    if (!logic->started || key < UGH_LOGIC_MENU_ESCAPE || key > UGH_LOGIC_MENU_OTHER) return;
    logic->game.menuKey(static_cast<ugh::input::MenuKey>(key));
}

int ugh_logic_step(ugh_logic* logic) { return logic->started ? result(logic->game.step()) : UGH_LOGIC_GAME_OVER; }

void ugh_logic_take_events(ugh_logic* logic, void (*callback)(void* ctx, const ugh_logic_event* event), void* ctx) {
    for (const ugh::events::Event& e : logic->events.take()) {
        ugh_logic_event event{eventKind(e.kind), e.player.value_or(-1), e.entity.value_or(-1), e.value};
        callback(ctx, &event);
    }
}

void ugh_logic_get_view(const ugh_logic* logic, ugh_logic_view* view) {
    *view = ugh_logic_view{};
    view->level_id = -1;
    if (!logic->started) return;
    const ugh::game::Game& game = logic->game;
    view->phase = phase(game.phase());
    const ugh::world::session::Session& session = game.session();
    view->level = session.levelNumber();
    view->lives = session.lives().count();
    view->multiplier = session.score().multiplier();
    view->score = session.score().points();
    if (game.levelLoaded()) viewLevel(game, *view);
}

}
