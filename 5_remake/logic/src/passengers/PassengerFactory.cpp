#include "passengers/PassengerFactory.hpp"

#include "data/levels/RoutePassengerPlacement.hpp"
#include "data/levels/StandingPassengerPlacement.hpp"
#include "passengers/route/RoutePassenger.hpp"

namespace ugh::passengers {

void PassengerFactory::visit(const data::levels::RoutePassengerPlacement& placement) {
    passengers_.all_.push_back(std::make_unique<route::RoutePassenger>(passengers_.count(), placement, level_));
}

void PassengerFactory::visit(const data::levels::StandingPassengerPlacement& placement) {
    auto passenger = std::make_unique<standing::StandingPassenger>(passengers_.count(), placement);
    passengers_.standing_.push_back(passenger.get());
    passengers_.all_.push_back(std::move(passenger));
}

}  // namespace ugh::passengers
