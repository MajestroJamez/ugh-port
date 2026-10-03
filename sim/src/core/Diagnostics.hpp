// Situations the core meets and does not support.
#pragma once

#include <string>
#include <utility>
#include <vector>

namespace ugh::core {

/**
 * Collects what the core met and does not support: the pause, all bonus slots in use, a level that does not exist.
 * The original would go on in some way the core does not copy; the replay player reports every problem as an error.
 */
class Diagnostics {
public:
    void report(std::string problem) { problems_.push_back(std::move(problem)); }

    /** The problems since the last take, oldest first. */
    std::vector<std::string> take() { return std::exchange(problems_, {}); }

    void clear() { problems_.clear(); }

private:
    std::vector<std::string> problems_;
};

}  // namespace ugh::core
