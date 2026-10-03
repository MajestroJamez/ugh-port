#include "world/Copters.hpp"

namespace ugh::world {

void Copters::placeAtStart(const data::LevelDefinition& definition, const data::SpriteIds& sprites) {
    for (int player = 0; player < 2; player++) {
        copters_[player].controls() = Controls{};
        copters_[player].placeAtStart(definition.startX[player], definition.startY[player], sprites.firstRotor[player]);
    }
}

std::optional<int> Copters::landedOn(int pad) const {
    for (int c = 0; c < count_; c++)
        if (copters_[c].landedOn(pad)) return c;
    return std::nullopt;
}

bool Copters::emptyLandedOn(int pad) const {
    for (int c = 0; c < count_; c++)
        if (copters_[c].landedOn(pad) && copters_[c].cabin().hasRoom()) return true;
    return false;
}

std::optional<int> Copters::firstOnWater(const Water& water, bool withRoom, bool still) const {
    for (int c = 0; c < count_; c++) {
        const Copter& copter = copters_[c];
        if ((still && copter.speedY() != units::Speed()) || (withRoom && !copter.cabin().hasRoom())) continue;
        if (copter.depthIn(water.row()) == 0) return c;
    }
    return std::nullopt;
}

}  // namespace ugh::world
