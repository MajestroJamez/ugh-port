#include "replay/CopterFields.hpp"

#include <string>

namespace ugh::replay {

void CopterFields::write(const world::Copter& copter, int player, Fields& f) {
    std::string c = "copter." + std::to_string(player) + ".";
    f[c + "x"] = std::to_string(copter.x().raw());
    f[c + "y"] = std::to_string(copter.y().raw());
    f[c + "pixelX"] = std::to_string(copter.pixelX());
    f[c + "pixelY"] = std::to_string(copter.pixelY());
    f[c + "vx"] = std::to_string(copter.speedX().raw());
    f[c + "vy"] = std::to_string(copter.speedY().raw());
    f[c + "landedPad"] = copter.landedPad() ? std::to_string(*copter.landedPad()) : "none";
    f[c + "rotorSprite"] = std::to_string(copter.rotor().sprite());
    f[c + "rotorCounter"] = std::to_string(copter.rotor().counter());
    const world::Controls& keys = copter.controls();
    std::string held;
    if (keys.up) held += 'U';
    if (keys.down) held += 'D';
    if (keys.left) held += 'L';
    if (keys.right) held += 'R';
    if (keys.fire) held += 'F';
    f[c + "keys"] = held.empty() ? "-" : held;
    const auto& cargo = copter.cabin().cargo();
    f[c + "cargoLook"] = cargo ? std::to_string(cargo->look) : "none";
    f[c + "destination"] = !cargo ? "none" : cargo->destination ? std::to_string(*cargo->destination) : "hanging";
    f[c + "fare"] = std::to_string(copter.cabin().fare());
    f[c + "effort"] = std::to_string(copter.rotor().effort());
    if (cargo && cargo->destination) f[c + "fareMin"] = std::to_string(cargo->fareMin);
}

}  // namespace ugh::replay
