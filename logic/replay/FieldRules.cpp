#include "replay/FieldRules.hpp"

#include <sstream>

namespace ugh::replay {

std::set<std::string> FieldRules::fieldsOf(const std::string& state) const {
    std::set<std::string> fields;
    for (const Rule& rule : rules_) {
        std::istringstream states(rule.states);
        std::string s;
        bool applies = false;
        while (states >> s) applies = applies || s == state;
        if (!applies) continue;
        std::istringstream names(rule.fields);
        std::string f;
        while (names >> f) fields.insert(f);
    }
    return fields;
}

}  // namespace ugh::replay
