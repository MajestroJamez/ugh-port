#include "check/ReplayReport.hpp"

#include <cstdio>

namespace ugh::check {

void ReplayReport::problem(long long tick, const std::string& text) {
    problems_++;
    if (texts_.size() < KEPT_PROBLEMS) texts_.push_back("tick " + std::to_string(tick) + ": " + text);
}

void ReplayReport::unreadable(const std::string& reason) {
    problems_++;
    if (texts_.size() < KEPT_PROBLEMS) texts_.push_back("cannot read: " + reason);
}

std::string ReplayReport::text(const std::string& path) const {
    std::string groups;
    for (const auto& [group, n] : compared_) groups += " " + group + " " + std::to_string(n);
    std::string result = path + ": " + std::to_string(ticks_) + " ticks, " +
                         (passed() ? "OK" : std::to_string(problems_) + " problems") + "; values compared:" + groups;
    for (const std::string& line : texts_) result += "\n  " + line;
    return result;
}

void ReplayReport::print(const std::string& path) const { std::printf("%s\n", text(path).c_str()); }

}  // namespace ugh::check
