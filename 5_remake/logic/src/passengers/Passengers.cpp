#include "passengers/Passengers.hpp"

#include "passengers/PassengerFactory.hpp"

namespace ugh::passengers {

void Passengers::load(const data::LevelDefinition& definition) {
    all_.clear();
    standing_.clear();
    PassengerFactory factory(*this);
    for (const auto& placement : definition.passengers) placement->accept(factory);
}

void Passengers::update(const PassengerContext& context) {
    for (auto& passenger : all_) passenger->update(context);
}

void Passengers::frameShown() {
    for (auto& passenger : all_) passenger->frameShown();
}

void Passengers::hideAll() {
    for (auto& passenger : all_) passenger->hide();
}

standing::StandingPassenger* Passengers::fallingOnto(units::Fixed x, units::Fixed y) const {
    for (standing::StandingPassenger* passenger : standing_)
        if (passenger->fallsOnto(x, y)) return passenger;
    return nullptr;
}

}  // namespace ugh::passengers
