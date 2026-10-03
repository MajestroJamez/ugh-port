// A passenger.
#pragma once

#include <optional>

#include "passengers/PassengerContext.hpp"
#include "passengers/PassengerVisitor.hpp"
#include "units/Fixed.hpp"
#include "world/Animator.hpp"

namespace ugh::passengers {

/**
 * A passenger: what all have - their place in the level's list, where they are, the sprite and the speech bubble
 * they show, their animation. A passenger with a route and the standing passenger are its two kinds, each with its
 * own state machine.
 */
class Passenger {
public:
    explicit Passenger(int index) : index_(index) {}
    virtual ~Passenger() = default;
    Passenger(const Passenger&) = delete;
    Passenger& operator=(const Passenger&) = delete;

    /** Its place in the level's list: pads, events and the replays name it so. */
    int index() const { return index_; }

    /** One frame in its state. */
    virtual void update(const PassengerContext& context) = 0;
    /** The end of a frame: the passenger was shown (or not). */
    virtual void frameShown() {}
    virtual void accept(PassengerVisitor& visitor) const = 0;

    units::Fixed x() const { return x_; }
    units::Fixed y() const { return y_; }
    std::optional<int> sprite() const { return sprite_; }
    std::optional<int> bubble() const { return bubble_; }
    const world::Animator& animator() const { return animator_; }

    /** Nothing of it is shown. */
    void hide() { sprite_.reset(); }

protected:
    units::Fixed x_, y_;   // top left corner
    std::optional<int> sprite_, bubble_;
    world::Animator animator_;

private:
    int index_;
};

}  // namespace ugh::passengers
