// A record of the game data file.
#pragma once

#include <map>
#include <string>

namespace ugh::data {

/** One line of the game data: `<type> [<name>] <key>=<value> ...`. */
struct UgdRecord {
    int line = 0;
    std::string type, name;
    std::map<std::string, std::string> fields;
};

}  // namespace ugh::data
