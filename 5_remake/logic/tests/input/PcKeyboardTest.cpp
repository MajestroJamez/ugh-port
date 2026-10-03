#include "TestData.hpp"
#include "TestFramework.hpp"
#include "input/PcKeyboard.hpp"

using namespace ugh;

TEST(an_extended_key_is_two_scancodes) {
    input::PcKeyboard keyboard(test::gameData().keys());
    CHECK(keyboard.deliver(0xe0) == nullptr);
    const data::KeyBinding::Action* up = keyboard.deliver(0x48);   // the arrow key up
    CHECK(up != nullptr);
    CHECK_EQUAL(0, up->player);
    CHECK(up->key == data::PlayerKey::Up);
    CHECK(up->press);
    const data::KeyBinding::Action* numpadUp = keyboard.deliver(0x48);   // the numeric keypad's 8
    CHECK(numpadUp != nullptr);
    CHECK_EQUAL(1, numpadUp->player);
}

TEST(the_game_loop_sees_a_change_of_the_last_scancode) {
    input::PcKeyboard keyboard(test::gameData().keys());
    keyboard.deliver(0x39);
    CHECK(keyboard.readLastScancode().changed);
    CHECK(!keyboard.readLastScancode().changed);
    keyboard.deliver(0xb9);
    CHECK_EQUAL(0xb9, static_cast<int>(keyboard.readLastScancode().scancode));
}
