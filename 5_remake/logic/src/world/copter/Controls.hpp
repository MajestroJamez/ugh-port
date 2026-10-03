// The keys a pilot holds.
#pragma once

#include "input/PlayerKey.hpp"

namespace ugh::world::copter {

/** The keys a pilot holds down. */
struct Controls {
    bool up = false, down = false, left = false, right = false, fire = false;

    /** A key pressed (true) or released. */
    void set(input::PlayerKey key, bool held) {
        switch (key) {
            case input::PlayerKey::Up: up = held; break;
            case input::PlayerKey::Down: down = held; break;
            case input::PlayerKey::Left: left = held; break;
            case input::PlayerKey::Right: right = held; break;
            case input::PlayerKey::Fire: fire = held; break;
        }
    }
};

}  // namespace ugh::world::copter
