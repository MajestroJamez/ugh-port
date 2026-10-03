#include "world/Rain.hpp"

namespace ugh::world {

namespace {

constexpr int EVEN_DROP_STEP = 3, ODD_DROP_STEP = 2;   // pixels per frame, down and with the wind

}  // namespace

void Rain::start(int waterRow, data::Wind wind, RandomNumbers& random, events::Diagnostics& diagnostics) {
    for (int i = DROPS - 1; i >= 0; i--) spawn(i, waterRow, wind, random);
    for (int frame = 0; frame < PREFALL_FRAMES; frame++) move(waterRow, wind, random, diagnostics);
}

void Rain::move(int waterRow, data::Wind wind, RandomNumbers& random, events::Diagnostics& diagnostics) {
    int last = waterRow + 1;   // the drops above the water: one per row
    if (last >= DROPS) {
        diagnostics.report("water below the screen: the original moves raindrops past the last one");
        last = DROPS - 1;
    }
    int direction = wind == data::Wind::Left ? -1 : 1;
    for (int i = last; i >= 0; i--) {
        Raindrop next = drops_[i].blown(i % 2 == 0 ? EVEN_DROP_STEP : ODD_DROP_STEP, direction);
        if (next.y >= floorRow_) spawn(i, waterRow, wind, random);
        else drops_[i] = next;
    }
}

void Rain::spawn(int i, int waterRow, data::Wind wind, RandomNumbers& random) {
    int place = random.next(static_cast<uint16_t>(Raindrop::SCREEN_WIDTH + waterRow));
    if (place < Raindrop::SCREEN_WIDTH) drops_[i] = {place, 0};
    else drops_[i] = {wind == data::Wind::Left ? Raindrop::SCREEN_WIDTH - 1 : 0, place - Raindrop::SCREEN_WIDTH};
}

}  // namespace ugh::world
