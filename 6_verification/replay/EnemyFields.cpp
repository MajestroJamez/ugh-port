#include "replay/EnemyFields.hpp"

#include <string>

#include "enemies/blower/Blower.hpp"
#include "enemies/flyer/Flyer.hpp"
#include "enemies/tree/Tree.hpp"
#include "enemies/walker/Walker.hpp"
#include "replay/FieldRules.hpp"
#include "replay/FieldTable.hpp"

namespace ugh::replay {

namespace {

using enemies::Enemy;
using enemies::blower::Blower;
using enemies::flyer::Flyer;
using enemies::tree::Tree;
using enemies::walker::Walker;

// ------------------------------------------------------------ which fields an enemy has in which state

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

// ------------------------------------------------------------ the values of the fields

std::string side(world::Facing facing) { return facing == world::Facing::Left ? "left" : "right"; }

const FieldTable<Enemy>& commonValues() {
    static const FieldTable<Enemy> values{
        {"sprite", [](const Enemy& e) { return e.sprite() ? std::to_string(*e.sprite()) : "none"; }},
        {"x", [](const Enemy& e) { return std::to_string(e.x().raw()); }},
        {"y", [](const Enemy& e) { return std::to_string(e.y().raw()); }},
        {"animFrame", [](const Enemy& e) { return std::to_string(e.animator().frame()); }},
        {"animDelay", [](const Enemy& e) { return std::to_string(e.animator().delay()); }},
    };
    return values;
}

const FieldTable<Flyer>& flyerValues() {
    static const FieldTable<Flyer> values{
        {"kind", [](const Flyer&) { return std::string("flyer"); }},
        {"state", [](const Flyer& f) { return std::string(f.state().name()); }},
        {"vx", [](const Flyer& f) { return std::to_string(f.speedX().raw()); }},
        {"lastTarget", [](const Flyer& f) { return std::to_string(f.lastTarget()); }},
        {"flight", [](const Flyer& f) { return side(f.flight()); }},
        {"waitTime", [](const Flyer& f) { return std::to_string(f.waitTime()); }},
        {"screechTime", [](const Flyer& f) { return std::to_string(f.waitTime()); }},
        {"fallSpeed", [](const Flyer& f) { return std::to_string(f.fallSpeed().raw()); }},
    };
    return values;
}

const FieldTable<Walker>& walkerValues() {
    static const FieldTable<Walker> values{
        {"kind", [](const Walker&) { return std::string("walker"); }},
        {"state", [](const Walker& w) { return std::string(w.state().name()); }},
        {"vx", [](const Walker& w) { return std::to_string(w.speedX().raw()); }},
        {"facing", [](const Walker& w) { return side(w.facing()); }},
        {"watchTime", [](const Walker& w) { return std::to_string(w.watchTime()); }},
        {"chargeSpeed", [](const Walker& w) { return std::to_string(w.charge().speed().raw()); }},
        {"stunTime", [](const Walker& w) { return std::to_string(w.stun().time()); }},
    };
    return values;
}

const FieldTable<Blower>& blowerValues() {
    static const FieldTable<Blower> values{
        {"kind", [](const Blower&) { return std::string("blower"); }},
        {"state", [](const Blower& b) { return std::string(b.state().name()); }},
        {"stunTime", [](const Blower& b) { return std::to_string(b.stun().time()); }},
    };
    return values;
}

const FieldTable<Tree>& treeValues() {
    static const FieldTable<Tree> values{
        {"kind", [](const Tree&) { return std::string("tree"); }},
        {"state", [](const Tree& t) { return std::string(t.state().name()); }},
        {"nextDrop", [](const Tree& t) { return std::to_string(t.nextDrop()); }},
        {"restTime", [](const Tree& t) { return std::to_string(t.restTime()); }},
    };
    return values;
}

std::string prefix(const Enemy& enemy) { return "enemy." + std::to_string(enemy.index()) + "."; }

}  // namespace

void EnemyFields::visit(const Flyer& f) { writeFields(f, prefix(f), flyerRules(), flyerValues(), commonValues(), fields_); }

void EnemyFields::visit(const Walker& w) {
    writeFields(w, prefix(w), walkerRules(), walkerValues(), commonValues(), fields_);
}

void EnemyFields::visit(const Blower& b) {
    writeFields(b, prefix(b), blowerRules(), blowerValues(), commonValues(), fields_);
}

void EnemyFields::visit(const Tree& t) { writeFields(t, prefix(t), treeRules(), treeValues(), commonValues(), fields_); }

}  // namespace ugh::replay
