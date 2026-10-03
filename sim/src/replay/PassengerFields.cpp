#include "replay/PassengerFields.hpp"

#include "model/Level.hpp"
#include "passengers/PassengerStates.hpp"
#include "replay/AnimationDelayField.hpp"
#include "replay/AnimationFrameField.hpp"
#include "replay/ReplayText.hpp"

namespace ugh::replay {

namespace {

/** The kind, by the offset of its descriptor in the original's data; not written before the passenger has one. */
class KindField : public Field {
public:
    KindField(const data::PassengerKind*& kind, const data::GameData& data) : kind_(kind), data_(data) {}

    std::optional<std::string> text() const override {
        if (!kind_) return std::nullopt;
        return hex(kind_->origin);
    }

    bool read(const std::string& text) const override {
        std::optional<int> v = parseHex(text);
        const data::PassengerKind* kind = v ? data_.passengerKind(static_cast<uint16_t>(*v)) : nullptr;
        if (kind) kind_ = kind;
        return kind != nullptr;
    }

    void fill(int) const override { kind_ = nullptr; }

private:
    const data::PassengerKind*& kind_;
    const data::GameData& data_;
};

/** The state, by its name; not written before the passenger has one. */
class StateField : public Field {
public:
    explicit StateField(const passengers::PassengerState*& state) : state_(state) {}

    std::optional<std::string> text() const override {
        if (!state_) return std::nullopt;
        return std::string(state_->name());
    }

    bool read(const std::string& text) const override {
        for (const passengers::PassengerState* state : passengers::PassengerStates::all()) {
            if (text == state->name()) {
                state_ = state;
                return true;
            }
        }
        return false;
    }

    void fill(int) const override { state_ = nullptr; }

private:
    const passengers::PassengerState*& state_;
};

/** Where the passenger is on its route: the original keeps a pointer that moves 4 bytes per stop; 0 for none. */
class RouteField : public Field {
public:
    RouteField(data::RouteCursor& route, const data::GameData& data) : route_(route), data_(data) {}

    std::optional<std::string> text() const override {
        return hex(route_.route ? route_.route->origin + BYTES_PER_STOP * route_.stop : 0);
    }

    bool read(const std::string& text) const override {
        std::optional<int> v = parseHex(text);
        if (!v) return false;
        if (*v == 0) {
            route_ = {};
            return true;
        }
        return data_.routeAt(static_cast<uint16_t>(*v), route_);
    }

    void fill(int) const override { route_ = {}; }

private:
    static constexpr int BYTES_PER_STOP = 4;
    data::RouteCursor& route_;
    const data::GameData& data_;
};

}  // namespace

void PassengerFields::visit(FieldVisitor& v, model::Passenger::Snapshot& s, const data::GameData& data) {
    v.visit("kind", KindField(s.kind, data));
    v.visit("state", StateField(s.state));
    v.signedWord("xf", s.x);
    v.signedWord("yf", s.y);
    v.signedWord("vy", s.vy);
    v.signedWord("timer", s.timer);
    v.signedWord("counter", s.counter);
    v.index("pickupPad", s.pickupPad, model::Level::PADS, 1);
    v.index("targetPad", s.targetPad, model::Level::PADS, 1);
    v.signedWord("bonusTimer", s.bonusTimer);
    v.hexOrNone("sprite", s.sprite);
    v.hexOrNone("bubble", s.bubble);
    v.signedWord("startPad", s.startPad);
    v.signedWord("x", s.pixelX);
    v.signedWord("y", s.pixelY);
    v.visit("anim", AnimationFrameField(s.animator));
    v.visit("animDelay", AnimationDelayField(s.animator));
    v.visit("route", RouteField(s.route, data));
}

}  // namespace ugh::replay
