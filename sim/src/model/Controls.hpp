// The keys a player holds down.
#pragma once

#include "data/KeyBinding.hpp"

namespace ugh::model {

/** The keys of a player that are held down (key states 278c .. 279e). */
struct Controls {
    bool up = false, down = false, left = false, right = false, fire = false;

    /** A key of the keyboard table pressed (true) or released. */
    void set(data::KeyBinding::Key key, bool held) {
        switch (key) {
            case data::KeyBinding::Key::Up: up = held; break;
            case data::KeyBinding::Key::Down: down = held; break;
            case data::KeyBinding::Key::Left: left = held; break;
            case data::KeyBinding::Key::Right: right = held; break;
            case data::KeyBinding::Key::Fire: fire = held; break;
            case data::KeyBinding::Key::None: break;
        }
    }
};

}  // namespace ugh::model
