// An index the logic uses as a replay field.
#pragma once

#include "replay/Field.hpp"
#include "replay/ReplayText.hpp"

namespace ugh::replay {

/**
 * An index the logic uses without a check (a pad slot, the difficulty, the wind): 0 .. count - 1, another value is
 * refused. The pattern is a valid index too (`pattern & fillMask`), so even a core that never got the value works.
 */
class IndexField : public Field {
public:
    IndexField(int& index, int count, int fillMask) : index_(index), count_(count), fillMask_(fillMask) {}

    std::optional<std::string> text() const override { return std::to_string(index_); }

    bool read(const std::string& text) const override {
        std::optional<int> v = parseNumber(text);
        if (!v || *v < 0 || *v >= count_) return false;
        index_ = *v;
        return true;
    }

    void fill(int pattern) const override { index_ = pattern & fillMask_; }

private:
    int& index_;
    int count_;
    int fillMask_;
};

}  // namespace ugh::replay
