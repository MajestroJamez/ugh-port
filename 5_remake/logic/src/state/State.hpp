// A state of an entity.
#pragma once

namespace ugh::state {

/**
 * A state of an entity (State): waiting, flying, lying ... The states are stateless singletons, the entity holds the
 * data; `Context` is what an update gets (the level, the session, the events).
 */
template <class Entity, class Context>
class State {
public:
    virtual ~State() = default;

    /** The name of the state (the replays). */
    virtual const char* name() const = 0;
    /** What the entity does when it gets into the state. */
    virtual void enter(Entity&, const Context&) const {}
    /** One frame in the state. */
    virtual void update(Entity& entity, const Context& context) const = 0;
};

}  // namespace ugh::state
