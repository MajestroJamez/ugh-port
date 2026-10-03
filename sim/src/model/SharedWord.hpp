// A word of the original an entity uses for several things.
#pragma once

#include <string>

#include "core/Audit.hpp"
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
    void restoreWord(core::Word word) {
        if (!(word == word_)) { meaning_ = "replay"; writtenIn_ = core::audit::context(); }
        word_ = word;
    }

protected:
    SharedWord() = default;
    explicit SharedWord(core::Word word) : word_(word) {}

    core::Word word_;
    const char* meaning_ = "unset";
    const char* writtenIn_ = "-";

    void wrote(const char* meaning) { meaning_ = meaning; writtenIn_ = core::audit::context(); }
    void reading(const char* word, const char* meaning) const {
        if (std::string(meaning_) != meaning)
            core::audit::count(std::string("Q2 ") + word + " read-as=" + meaning + " written-as=" + meaning_ +
                               " written@" + writtenIn_);
        else
            core::audit::count(std::string("Q2ok ") + word + " " + meaning);
    }
};

}  // namespace ugh::model
