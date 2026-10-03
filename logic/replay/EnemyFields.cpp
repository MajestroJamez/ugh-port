#include "replay/EnemyFields.hpp"

#include "enemies/blower/Blower.hpp"
#include "enemies/flyer/Flyer.hpp"
#include "enemies/tree/Tree.hpp"
#include "enemies/walker/Walker.hpp"
#include "replay/FieldRules.hpp"

namespace ugh::replay {

namespace {

const FieldRules& flyerRules() {
    static const FieldRules rules({
        {"Placed Hidden Screeching Flying Falling", "kind state vx lastTarget"},
        {"Hidden Screeching Flying Falling", "sprite"},
        {"Flying Falling", "x y"},
        {"Hidden Screeching Flying", "animFrame animDelay"},
        {"Flying", "flight"},
        {"Hidden", "waitTime"},
        {"Screeching", "screechTime"},
        {"Falling", "fallSpeed"},
    });
    return rules;
}

const FieldRules& walkerRules() {
    static const FieldRules rules({
        {"Placed Walking Watching Charging Recovering Stunned", "kind state x y vx facing"},
        {"Walking Watching Charging Recovering Stunned", "sprite animFrame animDelay"},
        {"Watching", "watchTime"},
        {"Charging", "chargeSpeed"},
        {"Stunned", "stunTime"},
    });
    return rules;
}

const FieldRules& blowerRules() {
    static const FieldRules rules({
        {"Placed Blowing Stunned", "kind state x y"},
        {"Blowing Stunned", "sprite"},
        {"Blowing", "animFrame animDelay"},
        {"Stunned", "stunTime"},
    });
    return rules;
}

const FieldRules& treeRules() {
    static const FieldRules rules({
        {"Placed Swaying Resting Bare", "kind state x y nextDrop"},
        {"Swaying Resting Bare", "sprite"},
        {"Swaying Resting", "animFrame animDelay"},
        {"Resting", "restTime"},
    });
    return rules;
}

const char* side(world::Facing facing) { return facing == world::Facing::Left ? "left" : "right"; }

std::string prefix(const enemies::Enemy& enemy) { return "enemy." + std::to_string(enemy.index()) + "."; }

}  // namespace

bool EnemyFields::common(const enemies::Enemy& e, const char* kind, const char* state, const std::string& field) {
    std::string& v = fields_[prefix(e) + field];
    if (field == "kind") v = kind;
    else if (field == "state") v = state;
    else if (field == "sprite") v = e.sprite() ? std::to_string(*e.sprite()) : "none";
    else if (field == "x") v = std::to_string(e.x().raw().value());
    else if (field == "y") v = std::to_string(e.y().raw().value());
    else if (field == "animFrame") v = std::to_string(e.animator().frame());
    else if (field == "animDelay") v = std::to_string(e.animator().delay().value());
    else return false;
    return true;
}

void EnemyFields::visit(const enemies::flyer::Flyer& f) {
    for (const std::string& field : flyerRules().fieldsOf(f.state().name())) {
        if (common(f, "flyer", f.state().name(), field)) continue;
        std::string& v = fields_[prefix(f) + field];
        if (field == "vx") v = std::to_string(f.speedX().raw().value());
        else if (field == "lastTarget") v = std::to_string(f.lastTarget());
        else if (field == "flight") v = side(f.flight());
        else if (field == "waitTime") v = std::to_string(f.waitTime().value());
        else if (field == "screechTime") v = std::to_string(f.screechTime().value());
        else if (field == "fallSpeed") v = std::to_string(f.fallSpeed().value());
    }
}

void EnemyFields::visit(const enemies::walker::Walker& w) {
    for (const std::string& field : walkerRules().fieldsOf(w.state().name())) {
        if (common(w, "walker", w.state().name(), field)) continue;
        std::string& v = fields_[prefix(w) + field];
        if (field == "vx") v = std::to_string(w.speedX().raw().value());
        else if (field == "facing") v = side(w.facing());
        else if (field == "watchTime") v = std::to_string(w.watchTime().value());
        else if (field == "chargeSpeed") v = std::to_string(w.chargeSpeed().value());
        else if (field == "stunTime") v = std::to_string(w.stunTime().value());
    }
}

void EnemyFields::visit(const enemies::blower::Blower& b) {
    for (const std::string& field : blowerRules().fieldsOf(b.state().name())) {
        if (common(b, "blower", b.state().name(), field)) continue;
        if (field == "stunTime") fields_[prefix(b) + field] = std::to_string(b.stunTime().value());
    }
}

void EnemyFields::visit(const enemies::tree::Tree& t) {
    for (const std::string& field : treeRules().fieldsOf(t.state().name())) {
        if (common(t, "tree", t.state().name(), field)) continue;
        std::string& v = fields_[prefix(t) + field];
        if (field == "nextDrop") v = std::to_string(t.nextDrop());
        else if (field == "restTime") v = std::to_string(t.restTime().value());
    }
}

}  // namespace ugh::replay
