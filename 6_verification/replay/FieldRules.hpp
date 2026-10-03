// Which fields an entity has in which state.
#pragma once

#include <map>
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

    explicit FieldRules(const std::vector<Rule>& rules);

    /** The fields of an entity in `state`. */
    const std::set<std::string>& fieldsOf(const std::string& state) const;

private:
    std::map<std::string, std::set<std::string>> fields_;   // by state
    std::set<std::string> none_;
};

}  // namespace ugh::replay
