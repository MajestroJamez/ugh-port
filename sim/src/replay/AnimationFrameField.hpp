// The frame of an animation as a replay field.
#pragma once

#include "model/Animator.hpp"
#include "replay/Field.hpp"
#include "replay/ReplayText.hpp"

namespace ugh::replay {

/** The frame of an animator ("anim"): the original counts it in bytes, two per frame. */
class AnimationFrameField : public Field {
public:
    explicit AnimationFrameField(model::Animator& animator) : animator_(animator) {}

    std::optional<std::string> text() const override {
        return formatWord(Format::Signed, animator_.frame() << 1);
    }

    bool read(const std::string& text) const override {
        std::optional<int> v = parseNumber(text);
        if (!v || *v % 2 != 0) return false;
        animator_ = model::Animator(*v / 2, animator_.delay());
        return true;
    }

    void fill(int pattern) const override { animator_ = model::Animator(pattern, animator_.delay()); }

private:
    model::Animator& animator_;
};

}  // namespace ugh::replay
