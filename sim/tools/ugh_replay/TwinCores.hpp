// Two cores side by side.
#pragma once

#include <string>

#include "ReplayFile.hpp"
#include "ugh_sim.h"

namespace ugh::tool {

/**
 * Two cores fed the same, the memory they were never given filled with different patterns (ugh_sim_reset,
 * ugh_sim_clear). A field counts as known when both have it with the same value: a value the core computed only
 * from what it was given does not depend on the pattern. This is how the player tells the core's own values from
 * memory the original had but the core never got (e.g. what the attract mode left), without the core having to
 * track what it knows.
 */
class TwinCores {
public:
    /** The cores of `dataPath`; ok() is false when the data cannot be loaded (error() says why). */
    explicit TwinCores(const char* dataPath);
    ~TwinCores();
    TwinCores(const TwinCores&) = delete;
    TwinCores& operator=(const TwinCores&) = delete;

    bool ok() const { return b_ != nullptr; }
    const std::string& error() const { return error_; }

    void reset();
    void clear();
    /** As ugh_sim_set: -1 when either core refuses the value. */
    int set(const std::string& name, const std::string& value);
    /** Different values for the two cores. */
    int setEach(const std::string& name, const std::string& a, const std::string& b) {
        int ra = ugh_sim_set(a_, name.c_str(), a.c_str()), rb = ugh_sim_set(b_, name.c_str(), b.c_str());
        return ra < 0 || rb < 0 ? -1 : ra;
    }
    void key(int scancode);
    /** How the cores end the game; -1 when they disagree (the end depends on memory they were never given). */
    int step();
    void newGame();
    int levelEnd();
    void levelStart();
    void playFrame();

    /** The fields both cores have with the same value. */
    Fields known() const;

    /** The problems of the first core (the second meets the same); the events are for a frontend: dropped. */
    void takeProblems(void (*callback)(void* ctx, const char* problem), void* ctx);

private:
    static constexpr int PATTERN_A = 0x0000, PATTERN_B = 0xffff;

    ugh_sim* a_ = nullptr;
    ugh_sim* b_ = nullptr;
    std::string error_;

    static int agreed(int a, int b) { return a == b ? a : -1; }
    static Fields fields(const ugh_sim* sim);
};

}  // namespace ugh::tool
