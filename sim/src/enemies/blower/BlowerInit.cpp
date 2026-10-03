#include "enemies/blower/BlowerInit.hpp"

#include "enemies/blower/Blowing.hpp"

namespace ugh::enemies {

const BlowerInit BlowerInit::instance{};

/** 113b:295b */
void BlowerInit::update(model::Enemy& enemy, model::Level& level) const {
    enemy.changeState(Blowing::instance, level);
}

}  // namespace ugh::enemies
