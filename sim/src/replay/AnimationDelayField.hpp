// The countdown of an animation as a replay field.
#pragma once

#include "model/Animator.hpp"
#include "replay/Field.hpp"
#include "replay/ReplayText.hpp"

namespace ugh::replay {

/** The countdown of an animator to its next frame ("animDelay"). */
class AnimationDelayField : public Field {
public:
    explicit AnimationDelayField(model::Animator& animator) : animator_(animator) {}

    std::optional<std::string> text() const override { return formatWord(Format::Signed, animator_.delay()); }

    bool read(const std::string& text) const override {
        std::optional<int> v = parseNumber(text);
        if (v) animator_ = model::Animator(animator_.frame(), *v);
        return v.has_value();
    }

    void fill(int pattern) const override { animator_ = model::Animator(animator_.frame(), pattern); }

private:
    model::Animator& animator_;
};

}  // namespace ugh::replay
