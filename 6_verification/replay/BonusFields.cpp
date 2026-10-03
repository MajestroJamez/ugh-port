#include "replay/BonusFields.hpp"

#include <string>

#include "bonuses/BonusState.hpp"

namespace ugh::replay {

void BonusFields::write(const bonuses::BonusItem& item, Fields& f) {
    std::string c = "bonus." + std::to_string(item.slot()) + ".";
    std::string state = item.state().name();
    f[c + "kind"] = item.kind().name;
    f[c + "state"] = state;
    f[c + "x"] = std::to_string(item.x().raw());
    f[c + "y"] = std::to_string(item.y().raw());
    f[c + "sprite"] = std::to_string(item.kind().sprite);
    if (state == "Falling") {
        f[c + "vx"] = std::to_string(item.speedX().raw());
        f[c + "vy"] = std::to_string(item.fallSpeed());
    } else {
        f[c + "lyingTime"] = std::to_string(item.lyingTime().remaining());
    }
}

}  // namespace ugh::replay
