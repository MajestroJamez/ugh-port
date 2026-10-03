// A listener that keeps the events until somebody takes them.
#pragma once

#include <utility>
#include <vector>

#include "core/Event.hpp"
#include "core/EventListener.hpp"

namespace ugh::core {

class EventQueue : public EventListener {
public:
    void onEvent(const Event& event) override { events_.push_back(event); }

    /** The events since the last take, oldest first; the queue is empty afterwards. */
    std::vector<Event> take() { return std::exchange(events_, {}); }

    const std::vector<Event>& events() const { return events_; }
    void clear() { events_.clear(); }

private:
    std::vector<Event> events_;
};

}  // namespace ugh::core
