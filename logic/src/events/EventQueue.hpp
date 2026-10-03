// Events kept until someone takes them.
#pragma once

#include <utility>
#include <vector>

#include "events/EventListener.hpp"

namespace ugh::events {

/** Keeps the events until they are taken (the C API hands them over after each frame). */
class EventQueue : public EventListener {
public:
    void onEvent(const Event& event) override { events_.push_back(event); }

    /** The events since the last take, oldest first. */
    std::vector<Event> take() { return std::exchange(events_, {}); }

private:
    std::vector<Event> events_;
};

}  // namespace ugh::events
