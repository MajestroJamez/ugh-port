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
    CHECK(up->key == input::PlayerKey::Up);
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

TEST(the_game_loop_sees_the_last_scancode_when_it_changed_since_the_frame_before) {
    keyboard::PcKeyboard keyboard(keys());
    CHECK(!keyboard.takeMenuKey().has_value());   // nothing yet
    keyboard.match(0x39);                         // Space pressed
    CHECK(keyboard.takeMenuKey() == input::MenuKey::Other);
    keyboard.match(0x39);                         // held: the same scancode again
    CHECK(!keyboard.takeMenuKey().has_value());
    keyboard.match(0xb9);                         // released and pressed again in one frame
    keyboard.match(0x39);
    CHECK(!keyboard.takeMenuKey().has_value());
    keyboard.match(0x01);                         // Esc
    CHECK(keyboard.takeMenuKey() == input::MenuKey::Escape);
    keyboard.match(0x19);                         // P
    CHECK(keyboard.takeMenuKey() == input::MenuKey::Pause);
}
