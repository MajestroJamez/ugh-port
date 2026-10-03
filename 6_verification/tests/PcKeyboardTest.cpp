#include <string>
#include <vector>

#include "TestFramework.hpp"
#include "keyboard/KeyFile.hpp"
#include "keyboard/PcKeyboard.hpp"

using namespace ugh;

namespace {

const std::vector<keyboard::KeyBinding>& keys() {
    static std::vector<keyboard::KeyBinding> bindings;
    if (bindings.empty()) {
        std::string error;
        if (!keyboard::KeyFile::read(ugh::test::dataPath(), bindings, error)) ugh::test::Registry::instance().fail(error);
    }
    return bindings;
}

}  // namespace

TEST(an_extended_key_is_two_scancodes) {
    keyboard::PcKeyboard keyboard(keys());
    CHECK(keyboard.match(0xe0) == nullptr);
    const keyboard::KeyBinding::Action* up = keyboard.match(0x48);   // the arrow key up
    CHECK(up != nullptr);
    CHECK_EQUAL(0, up->player);
    CHECK(up->key == data::PlayerKey::Up);
    CHECK(up->press);
    const keyboard::KeyBinding::Action* numpadUp = keyboard.match(0x48);   // the numeric keypad's 8
    CHECK(numpadUp != nullptr);
    CHECK_EQUAL(1, numpadUp->player);
}

TEST(a_fake_shift_is_swallowed) {
    keyboard::PcKeyboard keyboard(keys());
    CHECK(keyboard.match(0xe0) == nullptr);
    CHECK(keyboard.match(0x2a) == nullptr);
}
