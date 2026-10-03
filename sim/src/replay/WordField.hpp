// A plain word of the original as a replay field.
#pragma once

#include "core/Word.hpp"
#include "replay/Field.hpp"
#include "replay/ReplayText.hpp"

namespace ugh::replay {

/** A word of the original, written in a format; the pattern fills it as it is. */
class WordField : public Field {
public:
    WordField(core::Word& value, Format format) : value_(value), format_(format) {}

    std::optional<std::string> text() const override { return formatWord(format_, value_); }

    bool read(const std::string& text) const override {
        std::optional<int> v = parseWord(format_, text);
        if (v) value_ = core::Word(*v);
        return v.has_value();
    }

    void fill(int pattern) const override { value_ = core::Word(pattern); }

private:
    core::Word& value_;
    Format format_;
};

}  // namespace ugh::replay
