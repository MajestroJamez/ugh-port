// An input of the logic between two steps, as a replay keeps it.
#pragma once

#include "input/MenuKey.hpp"
#include "input/PlayerKey.hpp"

namespace ugh::record {

/**
 * An input of the logic between two steps (`game::Game::key`, `menuKey`): after which step of the recording it came
 * (from its first, 0) and what it was - a pilot's key pressed or released, or a key event of the game loop.
 */
struct Input {
    enum class Kind { PilotKey, MenuKey };

    int after = 0;
    Kind kind = Kind::PilotKey;
    int player = 0;   // a pilot's key: 0 or 1
    input::PlayerKey key = input::PlayerKey::Up;
    bool pressed = false;
    input::MenuKey menuKey = input::MenuKey::Other;   // a key of the game loop

    static Input pilot(int after, int player, input::PlayerKey key, bool pressed) {
        Input input;
        input.after = after;
        input.player = player;
        input.key = key;
        input.pressed = pressed;
        return input;
    }
    static Input menu(int after, input::MenuKey key) {
        Input input;
        input.after = after;
        input.kind = Kind::MenuKey;
        input.menuKey = key;
        return input;
    }

    bool operator==(const Input&) const = default;
};

}  // namespace ugh::record
