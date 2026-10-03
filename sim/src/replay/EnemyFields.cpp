#include "replay/EnemyFields.hpp"

#include <array>

#include "enemies/EnemyStates.hpp"
#include "model/Level.hpp"
#include "replay/AnimationDelayField.hpp"
#include "replay/AnimationFrameField.hpp"
#include "replay/ReplayText.hpp"

namespace ugh::replay {

namespace {

using Type = data::EnemyKind::Type;

constexpr std::array<Type, 4> TYPES = {Type::Flyer, Type::Walker, Type::Blower, Type::Tree};

const char* nameOf(Type type) {
    switch (type) {
        case Type::Flyer: return "flyer";
        case Type::Walker: return "walker";
        case Type::Blower: return "blower";
        case Type::Tree: return "tree";
    }
    return "";
}

/** The kind, by the name of its type; not written before the enemy has one. */
class KindField : public Field {
public:
    KindField(const data::EnemyKind*& kind, const data::GameData& data) : kind_(kind), data_(data) {}

    std::optional<std::string> text() const override {
        if (!kind_) return std::nullopt;
        return std::string(nameOf(kind_->type));
    }

    bool read(const std::string& text) const override {
        for (Type type : TYPES) {
            const data::EnemyKind* kind = data_.enemyKind(type);
            if (kind && text == nameOf(type)) {
                kind_ = kind;
                return true;
            }
        }
        return false;
    }

    void fill(int) const override { kind_ = nullptr; }

private:
    const data::EnemyKind*& kind_;
    const data::GameData& data_;
};

/** The state, by its name; not written before the enemy has one. */
class StateField : public Field {
public:
    explicit StateField(const enemies::EnemyState*& state) : state_(state) {}

    std::optional<std::string> text() const override {
        if (!state_) return std::nullopt;
        return std::string(state_->name());
    }

    bool read(const std::string& text) const override {
        for (const enemies::EnemyState* state : enemies::EnemyStates::all()) {
            if (text == state->name()) {
                state_ = state;
                return true;
            }
        }
        return false;
    }

    void fill(int) const override { state_ = nullptr; }

private:
    const enemies::EnemyState*& state_;
};

/**
 * The enemy's table word: the original keeps a pointer there - the flyer's animation, the tree's next bonus item
 * (a drop list and 2 bytes per item), or whatever an earlier enemy in the slot left.
 */
class TableField : public Field {
public:
    TableField(model::EnemyTable& table, const data::GameData& data) : table_(table), data_(data) {}

    std::optional<std::string> text() const override {
        if (table_.isFlightAnimation()) {
            const data::Animation* animation = table_.flightAnimation();
            return hex(animation ? animation->origin() : 0);
        }
        if (const data::DropCursor* drops = table_.dropCursor())
            return hex(drops->list->origin + BYTES_PER_DROP * drops->index);
        return hex(*table_.leftoverWord());
    }

    bool read(const std::string& text) const override {
        std::optional<int> v = parseHex(text);
        if (!v) return false;
        auto address = static_cast<uint16_t>(*v);
        data::DropCursor drops;
        if (const data::Animation* animation = data_.animation(address)) table_ = model::EnemyTable::flight(animation);
        else if (data_.dropsAt(address, drops)) table_ = model::EnemyTable::drops(drops);
        else table_ = model::EnemyTable::leftover(address);
        return true;
    }

    void fill(int pattern) const override { table_ = model::EnemyTable::leftover(static_cast<uint16_t>(pattern)); }

private:
    static constexpr int BYTES_PER_DROP = 2;
    model::EnemyTable& table_;
    const data::GameData& data_;
};

}  // namespace

void EnemyFields::visit(FieldVisitor& v, model::Enemy::Snapshot& s, const data::GameData& data) {
    v.visit("kind", KindField(s.kind, data));
    v.visit("state", StateField(s.state));
    v.signedWord("xf", s.x);
    v.signedWord("yf", s.y);
    v.signedWord("vx", s.vx);
    v.signedWord("timer", s.timer);
    v.signedWord("facing", s.facing);
    v.hexOrNone("sprite", s.sprite);
    v.index("pad", s.pad, model::Level::PADS, 1);
    v.signedWord("startDelay", s.startDelay);
    v.visit("anim", AnimationFrameField(s.animator));
    v.visit("animDelay", AnimationDelayField(s.animator));
    v.visit("table", TableField(s.table, data));
}

}  // namespace ugh::replay
