#include "TestFramework.hpp"
#include "model/Water.hpp"

using ugh::core::Fixed;

TEST(waterMovesEverySecondFrame) {
    ugh::model::Water w;
    w.fillTo(Fixed(0x100));
    w.move(-0x40);   // toggle 1: no movement
    CHECK(w.level() == Fixed(0x100));
    w.move(-0x40);   // toggle 0: moves by the speed
    CHECK(w.level() == Fixed(0xc0));
    CHECK(w.snapshot().hold == 0xff);   // the row changed: the next frame holds
    w.move(-0x40);
    CHECK(w.level() == Fixed(0xc0) && w.snapshot().hold == 0);
}
