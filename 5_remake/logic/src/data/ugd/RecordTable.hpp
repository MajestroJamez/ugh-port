// The record types a reader reads.
#pragma once

#include <cstddef>
#include <string>
#include <utility>

#include "data/ugd/UgdRecord.hpp"

namespace ugh::data::ugd {

/**
 * The record types a reader reads, each with the method that reads one (like method references in Java):
 * `{"rules", &RulesReader::readRules}`. A new record type is one entry and one method.
 */
template <class Reader>
class RecordTable {
public:
    using Entry = std::pair<const char*, bool (Reader::*)(const UgdRecord&)>;

    /** `table` has a method for records of `type`. */
    template <size_t N>
    static bool has(const Entry (&table)[N], const std::string& type) {
        for (const Entry& entry : table)
            if (type == entry.first) return true;
        return false;
    }

    /** `reader` reads `r` with the method `table` has for its type; false when there is none or it failed. */
    template <size_t N>
    static bool read(Reader& reader, const Entry (&table)[N], const UgdRecord& r) {
        for (const auto& [type, method] : table)
            if (r.type == type) return (reader.*method)(r);
        return false;
    }
};

}  // namespace ugh::data::ugd
