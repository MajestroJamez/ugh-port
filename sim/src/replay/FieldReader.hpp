// Sets one field of the replay state.
#pragma once

#include <string>
#include <utility>

#include "replay/FieldVisitor.hpp"

namespace ugh::replay {

/** Sets the one field named `name` (without its group: "xf") from its replay text. */
class FieldReader : public FieldVisitor {
public:
    /** As the C API returns it (ugh_sim_set). */
    enum class Result { BadValue = -1, NotAField = 0, Set = 1 };

    FieldReader(std::string name, std::string text) : name_(std::move(name)), text_(std::move(text)) {}

    void visit(const char* name, const Field& field) override;
    bool changedValues() const override { return result_ == Result::Set; }

    Result result() const { return result_; }

private:
    std::string name_, text_;
    Result result_ = Result::NotAField;
};

}  // namespace ugh::replay
