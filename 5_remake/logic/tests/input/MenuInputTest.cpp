#include "TestFramework.hpp"
#include "input/MenuInput.hpp"

using namespace ugh;

TEST(the_last_menu_key_stays_until_the_next) {
    input::MenuInput menu;
    CHECK(menu.last() == input::MenuKey::Other);
    menu.receive(input::MenuKey::Escape);
    CHECK(menu.last() == input::MenuKey::Escape);
    CHECK(menu.last() == input::MenuKey::Escape);
    menu.receive(input::MenuKey::Other);
    CHECK(menu.last() == input::MenuKey::Other);
}

TEST(a_caption_sees_a_menu_key_once) {
    input::MenuInput menu;
    CHECK(!menu.takeArrived());
    menu.receive(input::MenuKey::Other);
    CHECK(menu.takeArrived());
    CHECK(!menu.takeArrived());
}
