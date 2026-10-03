// Passes the events on to every listener.
#pragma once

#include <vector>

#include "core/Event.hpp"
#include "core/EventListener.hpp"

namespace ugh::core {

/** The listener the game reports to; it passes every event on to the listeners added to it. */
class EventBroadcast : public EventListener {
public:
    void add(EventListener& listener) { listeners_.push_back(&listener); }

    void onEvent(const Event& event) override {
        for (EventListener* listener : listeners_) listener->onEvent(event);
    }

private:
    std::vector<EventListener*> listeners_;
};

}  // namespace ugh::core
