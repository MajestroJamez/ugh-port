// Makes the passengers of a level.
#pragma once

#include "data/PassengerPlacementVisitor.hpp"
#include "passengers/Passengers.hpp"

namespace ugh::passengers {

/** Makes the right passenger of each placement of a level (Factory, as a Visitor of the placements). */
class PassengerFactory : public data::PassengerPlacementVisitor {
public:
    explicit PassengerFactory(Passengers& passengers) : passengers_(passengers) {}

    void visit(const data::RoutePassengerPlacement& placement) override;
    void visit(const data::StandingPassengerPlacement& placement) override;

private:
    Passengers& passengers_;
};

}  // namespace ugh::passengers
