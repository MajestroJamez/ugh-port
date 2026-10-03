// What the check found in a replay.
#pragma once

#include <map>
#include <string>
#include <vector>

namespace ugh::check {

/** The counts and the mismatches of one replay, and its summary line. */
class ReplayReport {
public:
    void tick() { ticks_++; }
    /** A value of the field group ("copter" ...) was compared. */
    void compared(const std::string& group) { compared_[group]++; }
    /** A mismatch or a problem at the tick, with its text (the first ones are kept). */
    void problem(long long tick, const std::string& text);

    /** The file cannot be read, the reason. */
    void unreadable(const std::string& reason);

    bool passed() const { return problems_ == 0; }

    /** The summary line, and the details of the first problems under it (one per line). */
    std::string text(const std::string& path) const;
    /** Prints `text` to stdout. */
    void print(const std::string& path) const;

private:
    static constexpr size_t KEPT_PROBLEMS = 5;

    long long ticks_ = 0;
    long long problems_ = 0;
    std::vector<std::string> texts_;
    std::map<std::string, long long> compared_;
};

}  // namespace ugh::check
