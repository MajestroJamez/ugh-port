// The standing passenger's state Gone.
#pragma once

#include "passengers/standing/StandingState.hpp"

namespace ugh::passengers::standing {

/** Off the screen. */
class Gone : public StandingState {
public:
    static const Gone instance;

    const char* name() const override { return "Gone"; }
    void enter(StandingPassenger& passenger, const PassengerContext& context) const override;
    void update(StandingPassenger&, const PassengerContext&) const override {}
};

}  // namespace ugh::passengers::standing
