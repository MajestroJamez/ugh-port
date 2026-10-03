#include "input/PcKeyboard.hpp"

#include <algorithm>

namespace ugh::input {

PcKeyboard::PcKeyboard(const std::vector<data::KeyBinding>& bindings) : bindings_(&bindings), failed_(bindings.size()) {}

const data::KeyBinding::Action* PcKeyboard::deliver(uint8_t scancode) {
    lastScancode_ = scancode;
    bool partial = false;
    const data::KeyBinding* matched = nullptr;
    for (size_t i = 0; i < bindings_->size() && !matched; i++) {
        if (failed_[i]) continue;
        const data::KeyBinding& binding = (*bindings_)[i];
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

PcKeyboard::Reading PcKeyboard::readLastScancode() {
    uint8_t previous = lastRead_;
    lastRead_ = lastScancode_;
    return {lastRead_, lastRead_ != previous};
}

}  // namespace ugh::input
