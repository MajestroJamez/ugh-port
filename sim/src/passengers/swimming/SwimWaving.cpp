#include "passengers/swimming/SwimWaving.hpp"

#include "passengers/swimming/Sinking.hpp"
#include "passengers/walking/Impatient.hpp"

namespace ugh::passengers {

const SwimWaving SwimWaving::instance{};

/** 113b:17e6 - the same as waving impatiently on a pad. */
void SwimWaving::enter(model::Passenger& passenger, model::Level& level) const {
    Impatient::instance.enter(passenger, level);
}

/** 113b:2068 - waves for a while, then sinks. */
void SwimWaving::update(model::Passenger& passenger, model::Level& level) const {
    passenger.floatOnSurface(level.water().row());
    if (passenger.animate()) passenger.show(*passenger.kind().waving);
    if (passenger.counter().tick()) passenger.changeState(Sinking::instance, level);
}

}  // namespace ugh::passengers
