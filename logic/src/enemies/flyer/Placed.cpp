#include "enemies/flyer/Placed.hpp"

#include "enemies/flyer/Flyer.hpp"
#include "enemies/flyer/Hidden.hpp"

namespace ugh::enemies::flyer {

const Placed Placed::instance{};

void Placed::update(Flyer& flyer, const EnemyContext& context) const { flyer.changeState(Hidden::instance, context); }

}  // namespace ugh::enemies::flyer
