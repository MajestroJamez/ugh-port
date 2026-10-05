#include "api/LevelView.hpp"

#include <algorithm>
#include <optional>
#include <utility>

#include "enemies/EnemyVisitor.hpp"
#include "enemies/blower/Blower.hpp"
#include "enemies/blower/Stunned.hpp"
#include "enemies/flyer/Falling.hpp"
#include "enemies/flyer/Flyer.hpp"
#include "enemies/tree/Tree.hpp"
#include "enemies/walker/Stunned.hpp"
#include "enemies/walker/Walker.hpp"
#include "passengers/PassengerVisitor.hpp"
#include "passengers/route/RoutePassenger.hpp"
#include "passengers/standing/StandingPassenger.hpp"

namespace ugh::api {

namespace {

/** A passenger's look: who sits in a copter or hangs below it (`Cargo::look`). */
class Look : public passengers::PassengerVisitor {
public:
    int look = 0;
    void visit(const passengers::route::RoutePassenger& passenger) override { look = passenger.form().land().look; }
    void visit(const passengers::standing::StandingPassenger& passenger) override { look = passenger.kind().look; }
};

/** An enemy a passenger knocked out: a walker or a blower stunned, a flyer falling. */
class KnockedOut : public enemies::EnemyVisitor {
public:
    bool out = false;
    void visit(const enemies::flyer::Flyer& enemy) override {
        out = &enemy.state() == &enemies::flyer::Falling::instance;
    }
    void visit(const enemies::walker::Walker& enemy) override {
        out = &enemy.state() == &enemies::walker::Stunned::instance;
    }
    void visit(const enemies::blower::Blower& enemy) override {
        out = &enemy.state() == &enemies::blower::Stunned::instance;
    }
    void visit(const enemies::tree::Tree&) override { out = false; }
};

int lookOf(const passengers::Passenger& passenger) {
    Look look;
    passenger.accept(look);
    return look.look;
}

bool knockedOut(const enemies::Enemy& enemy) {
    KnockedOut knocked;
    enemy.accept(knocked);
    return knocked.out;
}

/** An entity of the view; `look` a passenger's, `stunned` an enemy's. */
void addEntity(ugh_logic_view& view, int kind, int index, const world::figure::Figure& figure,
               std::optional<int> bubble, int look, bool stunned) {
    if (view.entity_count >= UGH_LOGIC_MAX_ENTITIES) return;
    view.entities[view.entity_count++] = {kind, index, figure.x().raw(), figure.y().raw(), figure.sprite().value_or(-1),
                                          bubble.value_or(-1), look, stunned ? 1 : 0};
}

/** The wind as ugh_logic.h gives it. */
int windDirection(data::levels::Wind wind) {
    switch (wind) {
        case data::levels::Wind::None: return 0;
        case data::levels::Wind::Left: return -1;
        case data::levels::Wind::Right: return 1;
    }
    return 0;
}

}  // namespace

void viewLevel(const game::Game& game, ugh_logic_view& view) {
    const world::Level& level = game.level();
    view.level_id = level.definition()->id;
    view.energy = level.energy().value();
    view.fade = level.fade().position();
    view.water_level = level.water().level().raw();
    view.water_frame = level.water().surfaceFrame();
    view.wind = windDirection(level.wind());
    view.copter_count = level.copters().count();
    for (const world::copter::Copter& c : level.copters().all()) {
        const auto& cargo = c.cabin().cargo();
        int destination = !cargo ? 0 : cargo->destination ? *cargo->destination : -1;
        view.copters[c.player()] = {c.motion().x().raw(), c.motion().y().raw(), c.rotor().sprite(), cargo ? cargo->look : 0,
                           destination, c.cabin().fare()};
    }
    for (int i = 0; i < game.passengers().count(); i++) {
        const passengers::Passenger& passenger = game.passengers()[i];
        addEntity(view, UGH_LOGIC_ENTITY_PASSENGER, i, passenger, passenger.bubble(), lookOf(passenger), false);
    }
    for (int i = 0; i < game.enemies().count(); i++) {
        const enemies::Enemy& enemy = game.enemies()[i];
        addEntity(view, UGH_LOGIC_ENTITY_ENEMY, i, enemy, std::nullopt, 0, knockedOut(enemy));
    }
    for (int slot = 0; slot < bonuses::BonusSlots::SLOTS; slot++) {
        const auto& item = game.bonuses()[slot];
        if (item) addEntity(view, UGH_LOGIC_ENTITY_BONUS_ITEM, slot, *item, std::nullopt, 0, false);
    }
    if (!level.windy()) return;
    for (const world::scenery::Raindrop& drop : level.rain().drops()) {
        if (!drop.onScreen()) continue;
        view.raindrops[view.raindrop_count][0] = drop.x;
        view.raindrops[view.raindrop_count][1] = drop.y;
        view.raindrop_count++;
    }
}


bool nameSprite(const data::GameData& data, int sprite, SpriteName& out) {
    for (const auto& animation : data.animations()) {
        const auto& frames = animation->frames;
        auto at = std::find(frames.begin(), frames.end(), sprite);
        if (at == frames.end()) continue;
        out = {animation->name, static_cast<int>(at - frames.begin()), animation->length()};
        return true;
    }
    const data::SpriteIds& ids = data.sprites();
    const std::pair<int, const char*> single[] = {{ids.standingPassenger, "standingPassenger"},
                                                  {ids.droppedPassenger, "droppedPassenger"},
                                                  {ids.bouncedPassenger, "bouncedPassenger"},
                                                  {ids.shakenTree, "shakenTree"}};
    for (const auto& [id, name] : single) {
        if (id != sprite) continue;
        out = {name, 0, 1};
        return true;
    }
    for (const auto& kind : data.bonusKinds()) {
        if (kind->sprite != sprite) continue;
        out = {kind->name, 0, 1};
        return true;
    }
    if (sprite >= ids.firstDestinationBubble && sprite <= ids.lastDestinationBubble) {
        out = {"destinationBubble", sprite - ids.firstDestinationBubble,
               ids.lastDestinationBubble - ids.firstDestinationBubble + 1};
        return true;
    }
    if (sprite == ids.impatientBubble) {
        out = {"impatientBubble", 0, 1};
        return true;
    }
    return false;
}

}  // namespace ugh::api
