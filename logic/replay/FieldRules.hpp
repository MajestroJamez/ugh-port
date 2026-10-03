// Which fields an entity has in which state.
#pragma once

#include <set>
#include <string>
#include <vector>

namespace ugh::replay {

/**
 * The rules of the replay state for one type of entity: the fields each state defines, as the same tables as the
 * Kotlin projection (SemanticProjection.kt). A rule is "states" -> "fields", both space separated.
 */
class FieldRules {
public:
    struct Rule {
        const char* states;
        const char* fields;
    };

    explicit FieldRules(std::vector<Rule> rules) : rules_(std::move(rules)) {}

    /** The fields of an entity in `state`. */
    std::set<std::string> fieldsOf(const std::string& state) const;

private:
    std::vector<Rule> rules_;
};

}  // namespace ugh::replay
