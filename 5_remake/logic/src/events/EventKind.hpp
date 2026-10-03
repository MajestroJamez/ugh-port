// What can happen in a frame.
#pragma once

namespace ugh::events {

/** What happened, for the sounds and effects of a frontend (the original played its sound effects there). */
enum class EventKind {
    LevelCaption,        // the caption of a level shows (its jingle)
    CopterCrashed,       // player: too hard a bounce, or caught by the flyer; the attempt ends
    LevelDone,           // the last passenger is delivered
    PassengerBoarded,    // player, entity = passenger
    PassengerPaid,       // player, entity = passenger, value = points
    QuickDelivery,       // player, entity = passenger: a bonus item drops
    PassengerDropped,    // player lets the hanging passenger go, entity = passenger
    PassengerInWater,    // entity = passenger
    FlyerScreech,        // entity = enemy, before it flies
    FlyerFlapStart,      // entity = enemy: the flapping until FlyerFlapStop
    FlyerFlapStop,       // entity = enemy
    BlowerBlow,          // entity = enemy
    EnemyStunned,        // entity = enemy, value = points: a passenger fell on it
    TreeDrop,            // entity = enemy: a bonus item drops out of the tree
    BonusCollected,      // player, entity = bonus slot, value = the effect (0 energy, 1 life, 2 multiplier)
};

}  // namespace ugh::events
