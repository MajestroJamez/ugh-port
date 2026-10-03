// The keys the game loop looks at.
#pragma once

#include "input/MenuKey.hpp"

namespace ugh::input {

/**
 * The keys the game loop looks at (not the pilots' keys): the last key event stays until the next one - Esc gives
 * the game up in every frame of the play while it is the last - and a caption waits for a new one.
 */
class MenuInput {
public:
    /** A key event, between two frames. */
    void receive(MenuKey key) {
        last_ = key;
        arrived_ = true;
    }
    MenuKey last() const { return last_; }
    /** A key event came since the last call. */
    bool takeArrived() {
        bool arrived = arrived_;
        arrived_ = false;
        return arrived;
    }

private:
    MenuKey last_ = MenuKey::Other;
    bool arrived_ = false;
};

}  // namespace ugh::input
