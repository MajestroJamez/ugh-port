// Situations the logic does not support.
#pragma once

#include <string>
#include <utility>
#include <vector>

namespace ugh::events {

/**
 * What the logic met and does not support (the pause, a 13th bonus item): the original would go on in a way the
 * logic does not copy. The replay check reports each as an error.
 */
class Diagnostics {
public:
    void report(std::string problem) { problems_.push_back(std::move(problem)); }

    /** The problems since the last take, oldest first. */
    std::vector<std::string> take() { return std::exchange(problems_, {}); }

private:
    std::vector<std::string> problems_;
};

}  // namespace ugh::events
