// The PC keyboard of the replays.
#pragma once

#include <cstdint>
#include <vector>

#include "game/Game.hpp"
#include "keyboard/KeyBinding.hpp"

namespace ugh::keyboard {

/**
 * The PC keyboard of the original (Adapter): the scancodes of a replay become the inputs of the logic. Scancodes,
 * matched against the key bindings (an extended key comes as two scancodes, fake shifts are swallowed), become the
 * pilots' keys. The original's game loop also looks at the last scancode: Esc and P, and a caption waits until it
 * changes; so before a frame the logic gets a menu key when the last scancode changed since the frame before.
 */
class PcKeyboard {
public:
    explicit PcKeyboard(const std::vector<KeyBinding>& bindings);

    /** A scancode, between two frames: a completed pilot's key goes to the game at once. */
    void deliver(uint8_t scancode, game::Game& game);
    /** The scancodes before a frame are delivered: the game loop sees the last one if it changed. */
    void beforeFrame(game::Game& game);

    /** The action of a completed key binding, if any (`deliver` without a game). */
    const KeyBinding::Action* match(uint8_t scancode);

private:
    static constexpr uint8_t SCANCODE_ESC = 0x01, SCANCODE_P = 0x19;

    const std::vector<KeyBinding>* bindings_;
    std::vector<bool> failed_;   // the bindings that do not match the sequence so far
    int position_ = 0;           // scancodes of the current sequence so far
    uint8_t last_ = 0;           // the last scancode
    uint8_t seen_ = 0;           // the last scancode the game loop has seen
};

}  // namespace ugh::keyboard
