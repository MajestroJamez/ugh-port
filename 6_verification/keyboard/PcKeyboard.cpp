#include "keyboard/PcKeyboard.hpp"

#include <algorithm>

namespace ugh::keyboard {

PcKeyboard::PcKeyboard(const std::vector<KeyBinding>& bindings) : bindings_(&bindings), failed_(bindings.size()) {}

void PcKeyboard::deliver(uint8_t scancode, game::Game& game) {
    const KeyBinding::Action* action = match(scancode);
    if (action) game.key(action->player, action->key, action->press);
}

void PcKeyboard::beforeFrame(game::Game& game) {
    std::optional<input::MenuKey> key = takeMenuKey();
    if (key) game.menuKey(*key);
}

std::optional<input::MenuKey> PcKeyboard::takeMenuKey() {
    if (last_ == seen_) return std::nullopt;
    seen_ = last_;
    if (last_ == SCANCODE_ESC) return input::MenuKey::Escape;
    if (last_ == SCANCODE_P) return input::MenuKey::Pause;
    return input::MenuKey::Other;
}

const KeyBinding::Action* PcKeyboard::match(uint8_t scancode) {
    last_ = scancode;
    bool partial = false;
    const KeyBinding* matched = nullptr;
    for (size_t i = 0; i < bindings_->size() && !matched; i++) {
        if (failed_[i]) continue;
        const KeyBinding& binding = (*bindings_)[i];
        int length = static_cast<int>(binding.scancodes.size());
        if (position_ >= length || binding.scancodes[position_] != scancode) failed_[i] = true;
        else if (position_ + 1 < length) partial = true;
        else matched = &binding;
    }
    if (matched || !partial) {
        position_ = 0;
        std::fill(failed_.begin(), failed_.end(), false);
    } else {
        position_++;
    }
    return matched && matched->action ? &*matched->action : nullptr;
}

}  // namespace ugh::keyboard
