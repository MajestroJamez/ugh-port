// Reads a golden replay.
#pragma once

#include <fstream>
#include <string>
#include <vector>

#include "replay/Fields.hpp"

namespace ugh::tool {

/**
 * A golden replay "UGR 1" (4_test_data/verify/.../replay/ReplayWriter.kt), read tick by tick: a T line has the tick number, the
 * scancodes and the fields that changed (`~` removes one); an I line after it belongs to it.
 */
class ReplayFile {
public:
    /** One tick of a replay: the state after it, the scancodes delivered after it, and its I line. */
    struct Tick {
        long long number = -1;
        replay::Fields state;        // the whole state after the tick
        std::vector<int> scancodes;  // delivered after the tick: the input of the next one
        replay::Fields inject;       // what the test pilot set after the tick
    };

    explicit ReplayFile(const std::string& path);

    /** The file is open and starts with "UGR 1"; else error() says why. */
    bool open();

    /** The next tick with its I line; false at the end or at an error (error() is then not empty). */
    bool next(Tick& tick);

    const std::string& error() const { return error_; }

private:
    std::string path_;
    std::ifstream in_;
    std::string pending_;    // the T line of the next tick, already read
    replay::Fields state_;   // the state so far
    std::string error_;
    int line_ = 0;

    bool readTickLine(const std::string& line, Tick& tick);
    static replay::Fields pairs(const std::string& text);
};

}  // namespace ugh::tool
