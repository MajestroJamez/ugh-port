#include "replay/PadFields.hpp"

#include <string>

namespace ugh::replay {

void PadFields::write(const world::Pad& pad, int index, Fields& f) {
    std::string c = "pad." + std::to_string(index) + ".";
    const data::PadDefinition& place = pad.place();
    f[c + "left"] = std::to_string(place.left);
    f[c + "right"] = std::to_string(place.right);
    f[c + "y"] = std::to_string(place.y);
    f[c + "door"] = std::to_string(place.door);
    f[c + "wait"] = std::to_string(place.wait);
    f[c + "stand"] = std::to_string(place.stand);
    f[c + "number"] = std::to_string(place.number);
    f[c + "waiting"] = pad.waiting() ? std::to_string(*pad.waiting()) : "none";
}

}  // namespace ugh::replay
