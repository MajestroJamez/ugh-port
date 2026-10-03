#include "replay/PadFields.hpp"

#include "replay/ReplayText.hpp"

namespace ugh::replay {

namespace {

/** The passenger waiting on the pad: the original keeps its offset in its arrays (passenger * 2), -1 for nobody. */
class WaitingField : public Field {
public:
    explicit WaitingField(int& passenger) : passenger_(passenger) {}

    std::optional<std::string> text() const override {
        return std::to_string(passenger_ == model::Pad::NOBODY ? -1 : core::Word(passenger_ * 2).value());
    }

    bool read(const std::string& text) const override {
        std::optional<int> v = parseNumber(text);
        if (!v || (*v != -1 && *v % 2 != 0)) return false;
        passenger_ = *v == -1 ? model::Pad::NOBODY : *v / 2;
        return true;
    }

    void fill(int pattern) const override { passenger_ = core::Word(pattern).value(); }

private:
    int& passenger_;
};

}  // namespace

void PadFields::visit(FieldVisitor& v, model::Pad::Snapshot& s) {
    v.signedWord("left", s.place.left);
    v.signedWord("right", s.place.right);
    v.signedWord("y", s.place.y);
    v.signedWord("number", s.place.number);
    v.signedWord("doorX", s.place.doorX);
    v.signedWord("waitX", s.place.waitX);
    v.signedWord("standX", s.place.standX);
    v.visit("waiting", WaitingField(s.waiting));
}

}  // namespace ugh::replay
