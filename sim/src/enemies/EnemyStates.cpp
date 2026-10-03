#include "enemies/EnemyStates.hpp"

#include "enemies/blower/BlowerInit.hpp"
#include "enemies/blower/BlowerWait.hpp"
#include "enemies/blower/Blowing.hpp"
#include "enemies/flyer/FlyerFalling.hpp"
#include "enemies/flyer/FlyerInit.hpp"
#include "enemies/flyer/FlyerWait.hpp"
#include "enemies/flyer/FlyerWait2.hpp"
#include "enemies/flyer/Flying.hpp"
#include "enemies/tree/Inactive.hpp"
#include "enemies/tree/TreeInit.hpp"
#include "enemies/tree/TreeSwaying.hpp"
#include "enemies/tree/TreeWait.hpp"
#include "enemies/walker/Charging.hpp"
#include "enemies/walker/Recovering.hpp"
#include "enemies/walker/Stunned.hpp"
#include "enemies/walker/WalkerInit.hpp"
#include "enemies/walker/Walking.hpp"
#include "enemies/walker/Watching.hpp"

namespace ugh::enemies {

const std::vector<const EnemyState*>& EnemyStates::all() {
    static const std::vector<const EnemyState*> states = {
        &FlyerInit::instance,  &FlyerWait::instance,   &FlyerWait2::instance, &Flying::instance,
        &FlyerFalling::instance, &WalkerInit::instance, &Walking::instance,   &Watching::instance,
        &Charging::instance,   &Recovering::instance,  &Stunned::instance,    &BlowerInit::instance,
        &Blowing::instance,    &BlowerWait::instance,  &TreeInit::instance,   &TreeSwaying::instance,
        &TreeWait::instance,   &Inactive::instance};
    return states;
}

}  // namespace ugh::enemies
