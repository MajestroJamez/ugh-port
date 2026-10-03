#include "passengers/PassengerFactory.hpp"

#include "data/RoutePassengerPlacement.hpp"
#include "data/StandingPassengerPlacement.hpp"
#include "passengers/route/RoutePassenger.hpp"

namespace ugh::passengers {

void PassengerFactory::visit(const data::RoutePassengerPlacement& placement) {
    passengers_.all_.push_back(std::make_unique<route::RoutePassenger>(passengers_.count(), placement));
}

void PassengerFactory::visit(const data::StandingPassengerPlacement& placement) {
    auto passenger = std::make_unique<standing::StandingPassenger>(passengers_.count(), placement);
    passengers_.standing_.push_back(passenger.get());
    passengers_.all_.push_back(std::move(passenger));
}

}  // namespace ugh::passengers
