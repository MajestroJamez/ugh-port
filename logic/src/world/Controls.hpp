// The keys a pilot holds.
#pragma once

#include "data/PlayerKey.hpp"

namespace ugh::world {

/** The keys a pilot holds down. */
struct Controls {
    bool up = false, down = false, left = false, right = false, fire = false;

    /** A key pressed (true) or released. */
    void set(data::PlayerKey key, bool held) {
        switch (key) {
            case data::PlayerKey::Up: up = held; break;
            case data::PlayerKey::Down: down = held; break;
            case data::PlayerKey::Left: left = held; break;
            case data::PlayerKey::Right: right = held; break;
            case data::PlayerKey::Fire: fire = held; break;
        }
    }
};

}  // namespace ugh::world
