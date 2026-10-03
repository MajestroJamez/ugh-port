#include "replay/CopterFields.hpp"

#include "model/Level.hpp"
#include "replay/ReplayText.hpp"

namespace ugh::replay {

namespace {

/** The pad the copter stands on: a pad slot, or -1 in the air. */
class LandedPadField : public Field {
public:
    explicit LandedPadField(int& pad) : pad_(pad) {}

    std::optional<std::string> text() const override { return std::to_string(pad_); }

    bool read(const std::string& text) const override {
        std::optional<int> v = parseNumber(text);
        if (!v || *v < model::Copter::IN_THE_AIR || *v >= model::Level::PADS) return false;
        pad_ = *v;
        return true;
    }

    void fill(int pattern) const override { pad_ = core::Word(pattern).value(); }

private:
    int& pad_;
};

/** The keys the pilot holds: the letters U, D, L, R, F, or "-" for none. */
class KeysField : public Field {
public:
    explicit KeysField(model::Controls& keys) : keys_(keys) {}

    std::optional<std::string> text() const override {
        std::string held;
        if (keys_.up) held += 'U';
        if (keys_.down) held += 'D';
        if (keys_.left) held += 'L';
        if (keys_.right) held += 'R';
        if (keys_.fire) held += 'F';
        return held.empty() ? "-" : held;
    }

    bool read(const std::string& text) const override {
        keys_ = {holds(text, 'U'), holds(text, 'D'), holds(text, 'L'), holds(text, 'R'), holds(text, 'F')};
        return true;
    }

    void fill(int pattern) const override {
        bool held = pattern != 0;
        keys_ = {held, held, held, held, held};
    }

private:
    model::Controls& keys_;

    static bool holds(const std::string& text, char key) { return text.find(key) != std::string::npos; }
};

}  // namespace

void CopterFields::visit(FieldVisitor& v, model::Copter::Snapshot& s) {
    v.signedWord("xf", s.x);
    v.signedWord("yf", s.y);
    v.signedWord("vx", s.vx);
    v.signedWord("vy", s.vy);
    v.visit("landedPad", LandedPadField(s.landedPad));
    v.signedWord("effort", s.effort);
    v.signedWord("impact", s.impact);
    v.hexOrNoneIfZero("carrying", s.carrying);
    v.signedWord("targetPad", s.targetPad);
    v.signedWord("fare", s.fare);
    v.signedWord("fareMin", s.fareMin);
    v.signedWord("x", s.pixelX);
    v.signedWord("y", s.pixelY);
    v.hex("sprite", s.rotor);
    v.signedWord("animCounter", s.rotorCounter);
    v.visit("keys", KeysField(s.keys));
}

}  // namespace ugh::replay
