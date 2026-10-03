// Makes the passengers of a level.
#pragma once

#include "data/levels/PassengerPlacementVisitor.hpp"
#include "passengers/Passengers.hpp"
#include "world/Level.hpp"

namespace ugh::passengers {

/** Makes the right passenger of each placement of a level (Factory, as a Visitor of the placements). */
class PassengerFactory : public data::levels::PassengerPlacementVisitor {
public:
    PassengerFactory(Passengers& passengers, world::Level& level) : passengers_(passengers), level_(level) {}

    void visit(const data::levels::RoutePassengerPlacement& placement) override;
    void visit(const data::levels::StandingPassengerPlacement& placement) override;

private:
    Passengers& passengers_;
    world::Level& level_;
};

}  // namespace ugh::passengers
