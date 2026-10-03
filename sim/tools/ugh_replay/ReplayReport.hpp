// What the player found in a replay.
#pragma once

#include <map>
#include <string>
#include <vector>

namespace ugh::tool {

/** The counts and the first problems of one replay, and its summary as the CTest log shows it. */
class ReplayReport {
public:
    /** A transition ("frame", "new game" ...) was checked. */
    void checked(const std::string& transition) { checked_[transition]++; }
    void skipped() { skipped_++; }
    /** A mismatch or a problem; the first ten are kept with their text. */
    void problem(long long tick, const std::string& what, const std::string& text);
    /** A value of the field group ("copter" ...) was compared. */
    void compared(const std::string& group) { compared_[group]++; }
    /** A field the core did not know after a level start. */
    void unknown(const std::string& field) { unknown_[field]++; }
    /** The core took over a recorded value it never had; `general` is the field with N for the index. */
    void takenOver(const std::string& general, long long tick);

    bool passed() const { return problems_ == 0; }

    /** The summary line and the details under it. */
    void print(const std::string& path) const;

private:
    static constexpr size_t KEPT_PROBLEMS = 10;

    std::map<std::string, long long> checked_;      // transition -> ticks
    long long skipped_ = 0;
    long long problems_ = 0;
    std::vector<std::string> problemTexts_;
    std::map<std::string, long long> compared_;     // field group -> values compared
    std::map<std::string, long long> unknown_;      // field -> ticks where the core did not know it
    long long takenOver_ = 0;
    std::map<std::string, long long> takenOverAt_;  // field (index as N) -> first tick
};

}  // namespace ugh::tool
