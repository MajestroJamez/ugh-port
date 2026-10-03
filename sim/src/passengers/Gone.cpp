#include "passengers/Gone.hpp"

namespace ugh::passengers {

const Gone Gone::instance{};

void Gone::enter(model::Passenger& passenger, model::Level&) const { passenger.hide(); }

void Gone::update(model::Passenger&, model::Level&) const {}

}  // namespace ugh::passengers
