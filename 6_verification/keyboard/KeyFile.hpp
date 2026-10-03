// The keys of the PC keyboard in the game data.
#pragma once

#include <string>
#include <vector>

#include "keyboard/KeyBinding.hpp"

namespace ugh::keyboard {

/** Reads the `key` records of the game data (assets/logic/ugh-data.ugd); the logic itself skips them. */
class KeyFile {
public:
    /** The key bindings in the order of the file; false (and `error`) when the file or a record is bad. */
    static bool read(const std::string& path, std::vector<KeyBinding>& out, std::string& error);
};

}  // namespace ugh::keyboard
