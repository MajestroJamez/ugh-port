// The C API (include/ugh_logic.h) over the game.
#include "ugh_logic.h"

#include <algorithm>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>

#include "api/LevelView.hpp"
#include "data/ugd/DataFileReader.hpp"
#include "events/EventQueue.hpp"
#include "game/Game.hpp"
#include "physics/CopterDanger.hpp"
#include "physics/CopterPhysics.hpp"
#include "world/copter/CopterShape.hpp"

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

// the events of the C API are the logic's events in the same order, from UGH_LOGIC_EVENT_LEVEL_CAPTION on
constexpr int eventKind(ugh::events::EventKind kind) { return static_cast<int>(kind) + UGH_LOGIC_EVENT_LEVEL_CAPTION; }
static_assert(eventKind(ugh::events::EventKind::BonusCollected) == UGH_LOGIC_EVENT_BONUS_COLLECTED);

// a full tank, a fully shown level; every raindrop fits into the view
static_assert(ugh::world::Energy::FULL == UGH_LOGIC_FULL_ENERGY && ugh::world::Fade::FULL == UGH_LOGIC_FADE_SHOWN);
static_assert(ugh::world::scenery::Rain::DROPS <= UGH_LOGIC_RAINDROPS);

// the values ugh_logic.h documents: the effect of a collected bonus item, the difficulty
static_assert(static_cast<int>(ugh::data::kinds::BonusEffect::Energy) == 0 &&
              static_cast<int>(ugh::data::kinds::BonusEffect::Life) == 1 &&
              static_cast<int>(ugh::data::kinds::BonusEffect::Multiplier) == 2);
static_assert(static_cast<int>(ugh::data::Difficulty::Easy) == 0 &&
              static_cast<int>(ugh::data::Difficulty::Medium) == 1 &&
              static_cast<int>(ugh::data::Difficulty::Hard) == 2);

// the screen, the positions and the copter's body of ugh_logic.h
static_assert(ugh::data::levels::ScreenSize::WIDTH == UGH_LOGIC_SCREEN_WIDTH &&
              ugh::data::levels::ScreenSize::HEIGHT == UGH_LOGIC_SCREEN_HEIGHT);
static_assert(ugh::units::Fixed::fromPixels(1).raw() == UGH_LOGIC_SUBPIXELS);
static_assert(ugh::world::copter::CopterShape::BODY_LEFT == UGH_LOGIC_COPTER_BODY_LEFT &&
              ugh::world::copter::CopterShape::BODY_RIGHT == UGH_LOGIC_COPTER_BODY_RIGHT &&
              ugh::world::copter::CopterShape::BODY_HEIGHT == UGH_LOGIC_COPTER_BODY_HEIGHT);

// a copter's top speed
static_assert(ugh::physics::CopterPhysics::TOP_SPEED.raw() == UGH_LOGIC_COPTER_TOP_SPEED);

// the words of the random numbers
constexpr size_t SEED_WORDS = std::tuple_size_v<ugh::world::session::RandomNumbers::Words>;
static_assert(SEED_WORDS == std::extent_v<decltype(ugh_logic_settings::random_seed)>);

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

/** The level being played, nullptr before the first one is loaded. */
const ugh::data::levels::LevelDefinition* levelPlayed(const ugh_logic* logic) {
    return logic->started && logic->game.levelLoaded() ? logic->game.level().definition() : nullptr;
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
    for (size_t i = 0; i < SEED_WORDS; i++) settings->random_seed[i] = defaults.randomSeed[i];
    settings->rain_floor_row = defaults.rainFloorRow;
}

int ugh_logic_new_game(ugh_logic* logic, const ugh_logic_settings* settings) {
    ugh::game::NewGameSettings s;
    s.players = settings->players;
    // any int is a value of the enum (its type is int); newGame refuses one outside the difficulties
    s.difficulty = static_cast<ugh::data::Difficulty>(settings->difficulty);
    s.firstLevel = settings->first_level;
    for (size_t i = 0; i < SEED_WORDS; i++) s.randomSeed[i] = settings->random_seed[i];
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

int ugh_logic_step(ugh_logic* logic) {
    if (!logic->started) return UGH_LOGIC_GAME_OVER;
    const int status = result(logic->game.step());
    logic->game.diagnostics().take();   // what the logic does not support: only the replay check reports it
    return status;
}

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
    if (game.levelLoaded()) ugh::api::viewLevel(game, *view);
}

int ugh_logic_get_copter_danger(const ugh_logic* logic, int player, ugh_logic_copter_danger* danger) {
    *danger = ugh_logic_copter_danger{};
    if (!logic->started || !logic->game.levelLoaded()) return 0;
    const ugh::world::Level& level = logic->game.level();
    if (player < 0 || player >= level.copters().count()) return 0;
    const ugh::physics::CopterDanger d =
        ugh::physics::CopterDanger::of(level, logic->game.session().crashLimit(), level.copters()[player]);
    *danger = ugh_logic_copter_danger{d.across.speed.raw(), d.upDown.speed.raw(), d.crashLimit, d.across.impact,
                                      d.upDown.impact, d.across.rock ? 1 : 0, d.upDown.rock ? 1 : 0};
    return 1;
}

int ugh_logic_pad_count(const ugh_logic* logic) {
    const ugh::data::levels::LevelDefinition* level = levelPlayed(logic);
    return level ? static_cast<int>(level->pads.size()) : 0;
}

int ugh_logic_get_pad(const ugh_logic* logic, int index, ugh_logic_pad* pad) {
    if (index < 0 || index >= ugh_logic_pad_count(logic)) return 0;
    const ugh::data::levels::PadDefinition& p = levelPlayed(logic)->pads[index];
    *pad = ugh_logic_pad{p.left, p.right, p.y, p.number};
    return 1;
}

int ugh_logic_solid(const ugh_logic* logic, int x, int y) {
    const ugh::data::levels::LevelDefinition* level = levelPlayed(logic);
    return level && level->mask.solid(x, y) ? 1 : 0;
}

int ugh_logic_get_sprite(const ugh_logic* logic, int sprite, ugh_logic_sprite* info) {
    ugh::api::SpriteName name;
    if (!ugh::api::nameSprite(*logic->data, sprite, name)) return 0;
    *info = ugh_logic_sprite{};
    name.name.copy(info->name, sizeof info->name - 1);
    info->frame = name.frame;
    info->frames = name.frames;
    return 1;
}

}
