// Writes the fields of the replay state.
#pragma once

#include <string>
#include <utility>
#include <vector>

#include "replay/FieldVisitor.hpp"

namespace ugh::replay {

/** Collects the visited fields as the replays write them: the full name ("copter.0.xf") and the text. */
class FieldWriter : public FieldVisitor {
public:
    using Lines = std::vector<std::pair<std::string, std::string>>;

    explicit FieldWriter(Lines& out) : out_(out) {}

    /** The fields visited from now on belong to `prefix` ("game.", "copter.0." ...). */
    void startGroup(std::string prefix) { prefix_ = std::move(prefix); }

    void visit(const char* name, const Field& field) override;
    bool changedValues() const override { return false; }

private:
    Lines& out_;
    std::string prefix_;
};

}  // namespace ugh::replay
