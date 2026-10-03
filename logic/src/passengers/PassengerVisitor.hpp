// The passengers by type.
#pragma once

namespace ugh::passengers {

namespace route {
class RoutePassenger;
}
namespace standing {
class StandingPassenger;
}

/** Visitor of the passengers by their type (the replay fields, a renderer). */
class PassengerVisitor {
public:
    virtual ~PassengerVisitor() = default;
    virtual void visit(const route::RoutePassenger& passenger) = 0;
    virtual void visit(const standing::StandingPassenger& passenger) = 0;
};

}  // namespace ugh::passengers
