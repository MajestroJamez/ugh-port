// The state machine of an entity.
#pragma once

#include <type_traits>

#include "state/State.hpp"

namespace ugh::state {

/**
 * The state machine of an entity: the entity derives from it (`class Flyer : public StateMachine<Flyer, ...>`) and
 * gets its current state and the two ways its states change it. `StateType` is the base of the entity's states when
 * they share more than `State` (default: `State<Entity, Context>`).
 */
template <class Entity, class Context, class StateType = State<Entity, Context>>
class StateMachine {
    static_assert(std::is_base_of_v<State<Entity, Context>, StateType>);

public:
    const StateType& state() const { return *state_; }

    /** Into `next` from the next frame on: its entry action now, its update in the next frame. */
    void changeState(const StateType& next, const Context& context) {
        state_ = &next;
        next.enter(entity(), context);
    }

    /** Into `next` and on in it in this frame: its entry action and its update now. */
    void continueIn(const StateType& next, const Context& context) {
        changeState(next, context);
        next.update(entity(), context);
    }

protected:
    explicit StateMachine(const StateType& initial) : state_(&initial) {}

    /** One frame in the current state. */
    void updateState(const Context& context) { state_->update(entity(), context); }

private:
    Entity& entity() { return static_cast<Entity&>(*this); }

    const StateType* state_;
};

}  // namespace ugh::state
