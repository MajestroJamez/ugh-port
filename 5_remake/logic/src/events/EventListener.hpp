// Who wants to know what happened.
#pragma once

#include "events/Event.hpp"

namespace ugh::events {

/** Gets the events of the logic as they happen (Observer). */
class EventListener {
public:
    virtual ~EventListener() = default;
    virtual void onEvent(const Event& event) = 0;
};

}  // namespace ugh::events
