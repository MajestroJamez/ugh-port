#include "enemies/walker/Placed.hpp"

#include "enemies/walker/Walker.hpp"
#include "enemies/walker/Walking.hpp"

namespace ugh::enemies::walker {

const Placed Placed::instance{};

void Placed::update(Walker& walker, const EnemyContext& context) const { walker.changeState(Walking::instance, context); }

}  // namespace ugh::enemies::walker
