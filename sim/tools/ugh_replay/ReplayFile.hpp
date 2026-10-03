// Reads a golden replay.
#pragma once

#include <fstream>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace ugh::tool {

/** The text starts with `prefix`. */
inline bool startsWith(const std::string& text, const char* prefix) { return text.rfind(prefix, 0) == 0; }

/** Field name -> value, as the replays write them. */
using Fields = std::map<std::string, std::string>;

/** One tick of a replay: the state after it, the keys delivered after it, and its B and I lines. */
struct Tick {
    long long number = -1;
    Fields state;                                         // the full state after the tick
    std::vector<int> keys;                                // scancodes delivered after it
    std::vector<std::pair<std::string, Fields>> before;   // B lines: what each stage changed, in stage order
    Fields inject;                                        // I line: values set from outside after the tick
};

/**
 * A replay file in the format "UGR 0" (verify/src/test/kotlin/ugh/verify/replay/ReplayWriter.kt), read tick by
 * tick: a T line has the tick number, the keys and the fields that changed (`~` removes one); the B and I lines
 * after it belong to it.
 */
class ReplayFile {
public:
    explicit ReplayFile(const std::string& path);

    /** The file is open and starts with "UGR 0"; else error() says why. */
    bool open();

    /** The next tick with its B and I lines; false at the end or at an error (error() is then not empty). */
    bool next(Tick& tick);

    const std::string& error() const { return error_; }

private:
    std::string path_;
    std::ifstream in_;
    std::string pending_;   // the T line of the next tick, already read
    Fields state_;          // the state so far (the T lines have only what changed)
    std::string error_;

    void readHead(const std::string& line, Tick& tick);
    static Fields pairs(const std::string& text);
};

}  // namespace ugh::tool
