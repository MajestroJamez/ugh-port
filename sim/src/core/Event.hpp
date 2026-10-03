// What happened in a frame, for the frontend (sounds, effects): the core reports events instead of playing the
// original's sound effects (ADLX blocks) or redrawing the status line. The kinds match the C API (ugh_sim.h).
#pragma once

#include "ugh_sim.h"

namespace ugh::core {

enum class EventKind {
    LevelCaption = UGH_SIM_EVENT_LEVEL_CAPTION,             // the caption's jingle (ADLX 4629)
    CopterCrashed = UGH_SIM_EVENT_COPTER_CRASHED,           // player
    LevelDone = UGH_SIM_EVENT_LEVEL_DONE,                   // the last passenger is delivered
    PassengerBoarded = UGH_SIM_EVENT_PASSENGER_BOARDED,     // player, entity = passenger
    PassengerPaid = UGH_SIM_EVENT_PASSENGER_PAID,           // player, entity = passenger, value = score
    QuickDelivery = UGH_SIM_EVENT_QUICK_DELIVERY,           // player; a bonus item drops (ADLX 4274)
    PassengerDropped = UGH_SIM_EVENT_PASSENGER_DROPPED,     // player lets the hanging passenger go (ADLX 4632)
    PassengerInWater = UGH_SIM_EVENT_PASSENGER_IN_WATER,    // entity = passenger
    FlyerScreech = UGH_SIM_EVENT_FLYER_SCREECH,             // entity = enemy (ADLX 425b)
    FlyerFlapStart = UGH_SIM_EVENT_FLYER_FLAP_START,        // the looping flap sound (ADLX 4260)
    FlyerFlapStop = UGH_SIM_EVENT_FLYER_FLAP_STOP,
    BlowerBlow = UGH_SIM_EVENT_BLOWER_BLOW,                 // entity = enemy (ADLX 4265)
    EnemyStunned = UGH_SIM_EVENT_ENEMY_STUNNED,             // entity = enemy, value = score
    TreeDrop = UGH_SIM_EVENT_TREE_DROP,                     // entity = enemy; a bonus item drops (ADLX 4274)
    BonusCollected = UGH_SIM_EVENT_BONUS_COLLECTED,         // player, entity = bonus slot, value = effect
};

/** An event: what happened, to which player and entity (slot index), and a value (points, effect). */
struct Event {
    static constexpr int NONE = -1;

    EventKind kind;
    int player = NONE;
    int entity = NONE;
    int value = 0;
};

}  // namespace ugh::core
