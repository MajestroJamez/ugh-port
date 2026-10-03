#include "enemies/walker/WalkerInit.hpp"

#include "enemies/walker/Walking.hpp"

namespace ugh::enemies {

const WalkerInit WalkerInit::instance{};

/** 113b:25b1 */
void WalkerInit::update(model::Enemy& enemy, model::Level& level) const {
    enemy.changeState(Walking::instance, level);
}

}  // namespace ugh::enemies
