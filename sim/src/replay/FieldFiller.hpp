// Fills the fields of the replay state with a pattern.
#pragma once

#include "replay/FieldVisitor.hpp"

namespace ugh::replay {

/** Fills every visited field with a pattern (ReplayProjection::forget). */
class FieldFiller : public FieldVisitor {
public:
    explicit FieldFiller(int pattern) : pattern_(pattern) {}

    void visit(const char*, const Field& field) override { field.fill(pattern_); }
    bool changedValues() const override { return true; }

private:
    int pattern_;
};

}  // namespace ugh::replay
