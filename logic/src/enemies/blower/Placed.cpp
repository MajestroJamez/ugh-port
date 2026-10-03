#include "enemies/blower/Placed.hpp"

#include "enemies/blower/Blower.hpp"
#include "enemies/blower/Blowing.hpp"

namespace ugh::enemies::blower {

const Placed Placed::instance{};

void Placed::update(Blower& blower, const EnemyContext& context) const { blower.changeState(Blowing::instance, context); }

}  // namespace ugh::enemies::blower
