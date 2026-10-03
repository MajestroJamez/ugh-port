// Something that happened in a frame.
#pragma once

#include <optional>

#include "events/EventKind.hpp"

namespace ugh::events {

/** An event: what happened, to which player and entity (its index in its list), and a value (points, effect). */
struct Event {
    EventKind kind;
    std::optional<int> player;
    std::optional<int> entity;
    int value = 0;
};

}  // namespace ugh::events
