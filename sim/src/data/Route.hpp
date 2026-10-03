// The route of a walking passenger.
#pragma once

#include <cstdint>
#include <vector>

#include "core/Word.hpp"
#include "data/DataImage.hpp"

namespace ugh::data {

/** The route of a walking passenger: pairs (pad, delay) up to the list end (level list B). */
struct Route {
    uint16_t origin = 0;               // the offset in the original's data
    std::vector<core::Word> words;     // pad, delay, pad, delay, ..., list end
};

/**
 * A position in a route: stop n goes from pad words[2n] (after the delay words[2n + 1]) to pad words[2n + 2];
 * the route is finished when there is no pad to go to.
 */
struct RouteCursor {
    const Route* route = nullptr;   // nullptr: no route (the standing passenger)
    int stop = 0;

    bool finished() const { return route->words[2 * stop + 2].bits() == DataImage::LIST_END; }
    int from() const { return route->words[2 * stop].value(); }
    core::Word delay() const { return route->words[2 * stop + 1]; }
    int to() const { return route->words[2 * stop + 2].value(); }
};

}  // namespace ugh::data
