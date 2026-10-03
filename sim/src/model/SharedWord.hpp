// A word of the original an entity uses for several things.
#pragma once

#include "core/Word.hpp"

namespace ugh::model {

/**
 * A word of the original that an entity uses for different things in different states (PassengerCounter,
 * EnemyTimer ...). The subclass names each use with its own methods; the raw word is only for the snapshot of the
 * entity (the replay adapter reads and restores it).
 */
class SharedWord {
public:
    core::Word word() const { return word_; }
    void restoreWord(core::Word word) { word_ = word; }

protected:
    SharedWord() = default;
    explicit SharedWord(core::Word word) : word_(word) {}

    core::Word word_;
};

}  // namespace ugh::model
