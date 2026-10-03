// The lives of a game.
#pragma once

namespace ugh::world::session {

/** The lives of a game: a failed attempt costs one, a life bonus item brings more, Esc takes them all. */
class Lives {
public:
    static constexpr int START = 3;
    static constexpr int MAX = 99;

    Lives() = default;
    explicit Lives(int count) : count_(count) {}

    int count() const { return count_; }

    /** A life less; false when none is left. */
    bool lose() { return --count_ > 0; }
    /** More lives (a bonus item), at most MAX. */
    void add(int amount) { count_ = count_ + amount > MAX ? MAX : count_ + amount; }
    /** Esc gives the game up: the attempt that ends is the last one. */
    void giveUp() { count_ = 0; }

private:
    int count_ = 0;
};

}  // namespace ugh::world::session
