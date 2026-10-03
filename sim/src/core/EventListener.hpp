// Who wants to know what happened in the game (Observer).
#pragma once

#include "core/Event.hpp"

namespace ugh::core {

/** Gets the events of the game as they happen: the C API keeps them for the frontend, a test checks them. */
class EventListener {
public:
    virtual ~EventListener() = default;
    virtual void onEvent(const Event& event) = 0;
};

}  // namespace ugh::core
