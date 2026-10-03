// The passengers of a level.
#pragma once

#include <memory>
#include <vector>

#include "data/levels/LevelDefinition.hpp"
#include "passengers/Passenger.hpp"
#include "passengers/standing/StandingPassenger.hpp"

namespace ugh::passengers {

/** The passengers of the level, in the order of its definition (the order of their updates). */
class Passengers {
public:
    /** The passengers of a new attempt at `definition`. */
    void load(const data::levels::LevelDefinition& definition);

    /** Every passenger's state, in order. */
    void update(const PassengerContext& context);
    /** The end of a frame: the passengers were shown. */
    void frameShown();
    /** Nothing of them is shown (before the play of an attempt). */
    void hideAll();

    int count() const { return static_cast<int>(all_.size()); }
    const Passenger& operator[](int i) const { return *all_[i]; }

    /** A standing passenger falling (down, not up) whose hit point is in the box at x, y (an enemy); nullptr if none. */
    standing::StandingPassenger* fallingOnto(units::Fixed x, units::Fixed y) const;

private:
    friend class PassengerFactory;

    std::vector<std::unique_ptr<Passenger>> all_;
    std::vector<standing::StandingPassenger*> standing_;   // the standing ones of all_, for the enemies
};

}  // namespace ugh::passengers
