#include "model/Rain.hpp"

#include "data/CollisionMask.hpp"

namespace ugh::model {

namespace {

constexpr int ROW = data::CollisionMask::WIDTH;   // pixels per row of the background page
constexpr int SCREEN_WIDTH = 320;
constexpr int PREFALL_FRAMES = 0x241;             // the rain falls this long before a windy level starts
constexpr uint8_t WIND_TO_THE_LEFT = 1;
constexpr int EVEN_DROP_STEP = 3, ODD_DROP_STEP = 2;   // pixels per frame down and with the wind

/** -1 for wind to the left, 1 to the right. */
int direction(uint8_t wind) { return wind == WIND_TO_THE_LEFT ? -1 : 1; }

}  // namespace

void Rain::start(core::Word waterRow, uint8_t wind, core::Random& random, core::Diagnostics& diagnostics) {
    for (int i = DROPS - 1; i >= 0; i--) spawn(i, waterRow, wind, random);
    for (int i = 0; i < PREFALL_FRAMES; i++) move(waterRow, wind, random, diagnostics);
}

void Rain::move(core::Word waterRow, uint8_t wind, core::Random& random, core::Diagnostics& diagnostics) {
    int last = waterRow.value() + 1;
    if (last >= DROPS) {
        diagnostics.report("water below the screen: the original moves raindrops past the last one");
        last = DROPS - 1;
    }
    int floor = s_.floorRow * (ROW / 4);   // in the bytes of the VGA page, like the original compares
    for (int i = last; i >= 0; i--) {
        int step = i % 2 == 0 ? EVEN_DROP_STEP : ODD_DROP_STEP;
        int drop = s_.drops[i] + step * ROW + step * direction(wind);
        if (static_cast<uint16_t>(drop >> 2) >= floor) spawn(i, waterRow, wind, random);
        else s_.drops[i] = drop;
    }
}

void Rain::spawn(int i, core::Word waterRow, uint8_t wind, core::Random& random) {
    uint16_t place = random.next(static_cast<uint16_t>(SCREEN_WIDTH + waterRow.value()));
    int x = 0, y = 0;
    if (place < SCREEN_WIDTH) {
        x = place;
    } else {
        y = place - SCREEN_WIDTH;
        x = wind == WIND_TO_THE_LEFT ? SCREEN_WIDTH - 1 : 0;
    }
    s_.drops[i] = (y & 0xff) * ROW + x;   // the original keeps the row in a byte
}

}  // namespace ugh::model
