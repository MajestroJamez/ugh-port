// The values of the fields of an entity, by name.
#pragma once

#include <initializer_list>
#include <map>
#include <string>
#include <utility>

#include "replay/FieldRules.hpp"
#include "replay/Fields.hpp"

namespace ugh::replay {

/**
 * The value of each field of an entity of type `Entity`, by the name of the field: a table "field -> how to get its
 * value" (like method references in Java). A new field is one line in the table.
 */
template <class Entity>
class FieldTable {
public:
    using Value = std::string (*)(const Entity&);

    FieldTable(std::initializer_list<std::pair<const std::string, Value>> values) : values_(values) {}

    bool has(const std::string& field) const { return values_.count(field) != 0; }
    /** The value of `field` of `entity`; "?" when the table has no such field. */
    std::string valueOf(const std::string& field, const Entity& entity) const {
        auto it = values_.find(field);
        return it == values_.end() ? "?" : it->second(entity);
    }

private:
    std::map<std::string, Value> values_;
};

/**
 * The fields `rules` give `entity` in its state, under `prefix` ("passenger.3."): the value from its own table, or
 * from the table of what all entities of its base have (`common`).
 */
template <class Entity, class Base>
void writeFields(const Entity& entity, const std::string& prefix, const FieldRules& rules,
                 const FieldTable<Entity>& own, const FieldTable<Base>& common, Fields& fields) {
    for (const std::string& field : rules.fieldsOf(entity.state().name()))
        fields[prefix + field] = own.has(field) ? own.valueOf(field, entity) : common.valueOf(field, entity);
}

}  // namespace ugh::replay
