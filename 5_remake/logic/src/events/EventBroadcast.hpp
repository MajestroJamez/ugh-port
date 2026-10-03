// Events for several listeners.
#pragma once

#include <vector>

#include "events/EventListener.hpp"

namespace ugh::events {

/** Passes every event on to all its listeners. */
class EventBroadcast : public EventListener {
public:
    void add(EventListener& listener) { listeners_.push_back(&listener); }

    void onEvent(const Event& event) override {
        for (EventListener* listener : listeners_) listener->onEvent(event);
    }

private:
    std::vector<EventListener*> listeners_;
};

}  // namespace ugh::events
