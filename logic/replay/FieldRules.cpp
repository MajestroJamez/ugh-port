#include "replay/FieldRules.hpp"

#include <sstream>

namespace ugh::replay {

FieldRules::FieldRules(const std::vector<Rule>& rules) {
    for (const Rule& rule : rules) {
        std::istringstream states(rule.states);
        std::string state;
        while (states >> state) {
            std::istringstream names(rule.fields);
            std::string field;
            while (names >> field) fields_[state].insert(field);
        }
    }
}

const std::set<std::string>& FieldRules::fieldsOf(const std::string& state) const {
    auto it = fields_.find(state);
    return it == fields_.end() ? none_ : it->second;
}

}  // namespace ugh::replay
