// The placements of passengers by type.
#pragma once

namespace ugh::data::levels {

struct RoutePassengerPlacement;
struct StandingPassengerPlacement;

/** Visitor of the passenger placements of a level (a factory makes the right passenger of each). */
class PassengerPlacementVisitor {
public:
    virtual ~PassengerPlacementVisitor() = default;
    virtual void visit(const RoutePassengerPlacement& placement) = 0;
    virtual void visit(const StandingPassengerPlacement& placement) = 0;
};

}  // namespace ugh::data::levels
