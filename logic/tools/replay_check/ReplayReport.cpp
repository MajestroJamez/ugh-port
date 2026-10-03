#include "ReplayReport.hpp"

#include <cstdio>

namespace ugh::tool {

void ReplayReport::problem(long long tick, const std::string& text) {
    problems_++;
    if (texts_.size() < KEPT_PROBLEMS) texts_.push_back("tick " + std::to_string(tick) + ": " + text);
}

void ReplayReport::print(const std::string& path) const {
    std::string groups;
    for (const auto& [group, n] : compared_) groups += " " + group + " " + std::to_string(n);
    std::printf("%s: %lld ticks, %s; values compared:%s\n", path.c_str(), ticks_,
                passed() ? "OK" : (std::to_string(problems_) + " problems").c_str(), groups.c_str());
    for (const std::string& text : texts_) std::printf("  %s\n", text.c_str());
}

}  // namespace ugh::tool
