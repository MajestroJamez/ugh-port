// A passenger.
#pragma once

#include <optional>

#include "passengers/PassengerContext.hpp"
#include "passengers/PassengerVisitor.hpp"
#include "units/Fixed.hpp"
#include "world/Figure.hpp"

namespace ugh::passengers {

/**
 * A passenger: what all have - what they show (`world::Figure`, with their place in the level's list) and their speech
 * bubble. A passenger with a route and the standing passenger are its two kinds, each with its own state machine.
 */
class Passenger : public world::Figure {
public:
    explicit Passenger(int index) : Figure(index) {}
    Passenger(int index, units::Fixed x, units::Fixed y) : Figure(index, x, y) {}
    virtual ~Passenger() = default;
    Passenger(const Passenger&) = delete;
    Passenger& operator=(const Passenger&) = delete;

    /** One frame in its state. */
    virtual void update(const PassengerContext& context) = 0;
    /** The end of a frame: the passenger was shown (or not). */
    virtual void frameShown() {}
    virtual void accept(PassengerVisitor& visitor) const = 0;

    std::optional<int> bubble() const { return bubble_; }
    void showBubble(int bubble) { bubble_ = bubble; }
    void hideBubble() { bubble_.reset(); }

private:
    std::optional<int> bubble_;
};

}  // namespace ugh::passengers
