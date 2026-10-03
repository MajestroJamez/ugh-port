// The fields of the passengers.
#pragma once

#include "passengers/PassengerVisitor.hpp"
#include "replay/Fields.hpp"

namespace ugh::replay {

/** The passenger.N.* fields of a passenger, by its type and state (Visitor of the passengers). */
class PassengerFields : public passengers::PassengerVisitor {
public:
    explicit PassengerFields(Fields& fields) : fields_(fields) {}

    void visit(const passengers::route::RoutePassenger& passenger) override;
    void visit(const passengers::standing::StandingPassenger& passenger) override;

private:
    Fields& fields_;
};

}  // namespace ugh::replay
