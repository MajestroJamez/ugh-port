#include "world/Copters.hpp"

namespace ugh::world {

void Copters::placeAtStart(const data::levels::LevelDefinition& definition, const data::SpriteIds& sprites) {
    for (Copter& copter : copters_) {
        int player = copter.player();
        copter.controls() = Controls{};
        copter.placeAtStart(definition.startX[player], definition.startY[player], sprites.firstRotor[player]);
    }
}

Copter* Copters::landedOn(const Pad& pad) {
    for (Copter& copter : all())
        if (copter.landedOn(pad)) return &copter;
    return nullptr;
}

bool Copters::emptyLandedOn(const Pad& pad) const {
    for (const Copter& copter : all())
        if (copter.landedOn(pad) && copter.cabin().hasRoom()) return true;
    return false;
}

Copter* Copters::firstOnWater(const Water& water, bool withRoom, bool still) {
    for (Copter& copter : all()) {
        if ((still && copter.motion().speedY() != units::Speed()) || (withRoom && !copter.cabin().hasRoom())) continue;
        if (copter.depthIn(water.row()) == 0) return &copter;
    }
    return nullptr;
}

}  // namespace ugh::world
