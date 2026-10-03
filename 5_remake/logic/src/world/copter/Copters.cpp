#include "world/copter/Copters.hpp"

namespace ugh::world::copter {

void Copters::placeAtStart(const data::levels::LevelDefinition& definition, const data::SpriteIds& sprites) {
    for (Copter& copter : copters_) {
        int player = copter.player();
        copter.controls() = Controls{};
        copter.placeAtStart(definition.startX[player], definition.startY[player], sprites.firstRotor[player]);
    }
}

Copter* Copters::landedOn(const scenery::Pad& pad) {
    for (Copter& copter : all())
        if (copter.landedOn(pad)) return &copter;
    return nullptr;
}

bool Copters::landedOnWithRoom(const scenery::Pad& pad) const {
    for (const Copter& copter : all())
        if (copter.landedOn(pad) && copter.cabin().hasRoom()) return true;
    return false;
}

Copter* Copters::firstOnWater(const scenery::Water& water, Wanted wanted) {
    for (Copter& copter : all()) {
        if (!fits(copter, wanted)) continue;
        if (copter.depthIn(water.row()) == 0) return &copter;
    }
    return nullptr;
}

bool Copters::fits(const Copter& copter, Wanted wanted) {
    bool still = copter.motion().speedY() == units::Speed();
    if (wanted == Wanted::StillWithRoom && !still) return false;
    return wanted == Wanted::Any || copter.cabin().hasRoom();
}

}  // namespace ugh::world::copter
