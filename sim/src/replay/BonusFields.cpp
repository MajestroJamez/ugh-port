#include "replay/BonusFields.hpp"

#include "bonuses/BonusStates.hpp"
#include "replay/ReplayText.hpp"

namespace ugh::replay {

namespace {

/** The kind, by the offset of its descriptor in the original's data; not written before the item has one. */
class KindField : public Field {
public:
    KindField(const data::BonusKind*& kind, const data::GameData& data) : kind_(kind), data_(data) {}

    std::optional<std::string> text() const override {
        if (!kind_) return std::nullopt;
        return hex(kind_->origin);
    }

    bool read(const std::string& text) const override {
        std::optional<int> v = parseHex(text);
        const data::BonusKind* kind = v ? data_.bonusKind(static_cast<uint16_t>(*v)) : nullptr;
        if (kind) kind_ = kind;
        return kind != nullptr;
    }

    void fill(int) const override { kind_ = nullptr; }

private:
    const data::BonusKind*& kind_;
    const data::GameData& data_;
};

/** The state, by its name; not written before the item has one. */
class StateField : public Field {
public:
    explicit StateField(const bonuses::BonusState*& state) : state_(state) {}

    std::optional<std::string> text() const override {
        if (!state_) return std::nullopt;
        return std::string(state_->name());
    }

    bool read(const std::string& text) const override {
        for (const bonuses::BonusState* state : bonuses::BonusStates::all()) {
            if (text == state->name()) {
                state_ = state;
                return true;
            }
        }
        return false;
    }

    void fill(int) const override { state_ = nullptr; }

private:
    const bonuses::BonusState*& state_;
};

}  // namespace

void BonusFields::visit(FieldVisitor& v, model::BonusItem::Snapshot& s, const data::GameData& data) {
    v.visit("kind", KindField(s.kind, data));
    v.visit("state", StateField(s.state));
    v.signedWord("xf", s.x);
    v.signedWord("yf", s.y);
    v.signedWord("vx", s.timer);   // the original's x speed word; the item uses it as its timer too (BonusTimer)
    v.signedWord("vy", s.vy);
    v.hex("sprite", s.sprite);
}

}  // namespace ugh::replay
