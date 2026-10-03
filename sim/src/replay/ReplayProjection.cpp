#include "replay/ReplayProjection.hpp"

#include <algorithm>
#include <optional>

#include "replay/BonusFields.hpp"
#include "replay/CopterFields.hpp"
#include "replay/EnemyFields.hpp"
#include "replay/FieldFiller.hpp"
#include "replay/FieldWriter.hpp"
#include "replay/GameFields.hpp"
#include "replay/PadFields.hpp"
#include "replay/PassengerFields.hpp"
#include "replay/ReplayText.hpp"

namespace ugh::replay {

namespace {

/** "copter.0." */
std::string prefix(const char* group, int index) { return std::string(group) + "." + std::to_string(index) + "."; }

}  // namespace

void ReplayProjection::reset(model::Level& level, int fill) {
    model::Rain::Snapshot rain = level.rain().snapshot();
    rain.drops.fill(fill);
    level.rain().restore(rain);
    forget(level, fill);
}

void ReplayProjection::forget(model::Level& level, int fill) {
    fill_ = fill;
    FieldFiller filler(fill);
    changeGame(filler, level);
    model::Level::Snapshot lists = level.snapshot();
    lists.padCount = lists.passengerCount = lists.enemyCount = 0;
    level.restore(lists);
    for (int i = 0; i < model::Level::COPTERS; i++) change(filler, level.copter(i));
    for (int i = 0; i < model::Level::PADS; i++) change(filler, level.pad(i));
    for (int i = 0; i < model::Level::PASSENGERS; i++) change(filler, level.passenger(i));
    for (int i = 0; i < model::Level::ENEMIES; i++) change(filler, level.enemy(i));
    for (int i = 0; i < model::BonusSlots::SLOTS; i++) {
        change(filler, level.bonuses()[i]);
        level.bonuses()[i].disappear();
    }
}

int ReplayProjection::set(model::Level& level, const std::string& field, const std::string& value) {
    size_t dot = field.find('.');
    if (dot == std::string::npos) return 0;
    std::string group = field.substr(0, dot), rest = field.substr(dot + 1);
    if (group == "game") {
        FieldReader reader(rest, value);
        changeGame(reader, level);
        return static_cast<int>(reader.result());
    }
    // group.index.name
    size_t dot2 = rest.find('.');
    if (dot2 == std::string::npos) return 0;
    std::optional<int> index = parseNumber(rest.substr(0, dot2));
    if (!index || *index < 0) return 0;
    FieldReader reader(rest.substr(dot2 + 1), value);
    return setInGroup(level, group, *index, reader);
}

int ReplayProjection::setInGroup(model::Level& level, const std::string& group, int i, FieldReader& reader) const {
    if (group == "copter") {
        if (i >= model::Level::COPTERS) return 0;
        change(reader, level.copter(i));
    } else if (group == "bonus") {
        if (i >= model::BonusSlots::SLOTS) return 0;
        change(reader, level.bonuses()[i]);
    } else if (group == "pad" || group == "passenger" || group == "object") {
        if (!lengthenList(level, group, i)) return 0;
        if (group == "pad") change(reader, level.pad(i));
        else if (group == "passenger") change(reader, level.passenger(i));
        else change(reader, level.enemy(i));
    } else {
        return 0;
    }
    return static_cast<int>(reader.result());
}

std::vector<std::pair<std::string, std::string>> ReplayProjection::fields(const model::Level& level) const {
    FieldWriter::Lines out;
    FieldWriter writer(out);

    GameFields::State game = GameFields::State::of(level);
    writer.startGroup("game.");
    GameFields::visit(writer, game, fill_);

    int copters = std::clamp<int>(level.session().players().bits(), 1, 2);
    for (int i = 0; i < copters; i++) {
        model::Copter::Snapshot copter = level.copter(i).snapshot();
        writer.startGroup(prefix("copter", i));
        CopterFields::visit(writer, copter);
    }
    for (int i = 0; i < level.padCount(); i++) {
        model::Pad::Snapshot pad = level.pad(i).snapshot();
        writer.startGroup(prefix("pad", i));
        PadFields::visit(writer, pad);
    }
    for (int i = 0; i < level.passengerCount(); i++) {
        model::Passenger::Snapshot passenger = level.passenger(i).snapshot();
        writer.startGroup(prefix("passenger", i));
        PassengerFields::visit(writer, passenger, data_);
    }
    for (int i = 0; i < level.enemyCount(); i++) {
        model::Enemy::Snapshot enemy = level.enemy(i).snapshot();
        writer.startGroup(prefix("object", i));
        EnemyFields::visit(writer, enemy, data_);
    }
    for (int i = 0; i < model::BonusSlots::SLOTS; i++) {
        const model::BonusItem& item = level.bonuses()[i];
        if (!item.inUse()) continue;
        model::BonusItem::Snapshot bonus = item.snapshot();
        writer.startGroup(prefix("bonus", i));
        BonusFields::visit(writer, bonus, data_);
    }
    return out;
}

void ReplayProjection::changeGame(FieldVisitor& v, model::Level& level) const {
    GameFields::State s = GameFields::State::of(level);
    GameFields::visit(v, s, fill_);
    if (v.changedValues()) s.restoreTo(level);
}

void ReplayProjection::change(FieldVisitor& v, model::Copter& copter) const {
    model::Copter::Snapshot s = copter.snapshot();
    CopterFields::visit(v, s);
    if (v.changedValues()) copter.restore(s);
}

void ReplayProjection::change(FieldVisitor& v, model::Pad& pad) const {
    model::Pad::Snapshot s = pad.snapshot();
    PadFields::visit(v, s);
    if (v.changedValues()) pad.restore(s);
}

void ReplayProjection::change(FieldVisitor& v, model::Passenger& passenger) const {
    model::Passenger::Snapshot s = passenger.snapshot();
    PassengerFields::visit(v, s, data_);
    if (v.changedValues()) passenger.restore(s);
}

void ReplayProjection::change(FieldVisitor& v, model::Enemy& enemy) const {
    model::Enemy::Snapshot s = enemy.snapshot();
    EnemyFields::visit(v, s, data_);
    if (v.changedValues()) enemy.restore(s);
}

void ReplayProjection::change(FieldVisitor& v, model::BonusItem& item) const {
    model::BonusItem::Snapshot s = item.snapshot();
    BonusFields::visit(v, s, data_);
    if (v.changedValues()) item.restore(s);
}

bool ReplayProjection::lengthenList(model::Level& level, const std::string& group, int index) {
    model::Level::Snapshot lists = level.snapshot();
    int* length = nullptr;
    int slots = 0;
    if (group == "pad") {
        length = &lists.padCount;
        slots = model::Level::PADS;
    } else if (group == "passenger") {
        length = &lists.passengerCount;
        slots = model::Level::PASSENGERS;
    } else if (group == "object") {
        length = &lists.enemyCount;
        slots = model::Level::ENEMIES;
    }
    if (!length || index >= slots) return false;
    *length = std::max(*length, index + 1);
    level.restore(lists);
    return true;
}

}  // namespace ugh::replay
