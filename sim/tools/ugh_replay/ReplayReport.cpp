#include "ReplayReport.hpp"

#include <cstdio>

namespace ugh::tool {

void ReplayReport::problem(long long tick, const std::string& what, const std::string& text) {
    problems_++;
    if (problemTexts_.size() < KEPT_PROBLEMS)
        problemTexts_.push_back("tick " + std::to_string(tick) + " " + what + ": " + text);
}

void ReplayReport::takenOver(const std::string& general, long long tick) {
    takenOver_++;
    takenOverAt_.emplace(general, tick);
}

void ReplayReport::print(const std::string& path) const {
    std::string checked;
    for (const auto& [what, n] : checked_) checked += " " + what + " " + std::to_string(n) + ",";
    std::string compared;
    for (const auto& [group, n] : compared_) compared += " " + group + " " + std::to_string(n);
    std::printf("%s:%s skipped %lld, %lld mismatches, %lld taken over; values compared:%s\n", path.c_str(),
                checked.c_str(), skipped_, problems_, takenOver_, compared.c_str());
    for (const auto& line : problemTexts_) std::printf("  %s\n", line.c_str());
    if (!unknown_.empty()) {
        std::string names;
        for (const auto& [name, n] : unknown_) names += " " + name;
        std::printf("  not known after a level start:%s\n", names.c_str());
    }
    if (!takenOverAt_.empty()) {
        std::string names;
        for (const auto& [name, tick] : takenOverAt_) names += " " + name + "@" + std::to_string(tick);
        std::printf("  taken over (first tick):%s\n", names.c_str());
    }
}

}  // namespace ugh::tool
