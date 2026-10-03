// The level being played.
#pragma once

#include <array>
#include <cstdint>

#include "core/Diagnostics.hpp"
#include "core/Event.hpp"
#include "core/EventListener.hpp"
#include "core/Fixed.hpp"
#include "data/Box.hpp"
#include "data/GameData.hpp"
#include "model/BonusSlots.hpp"
#include "model/Copter.hpp"
#include "model/Enemy.hpp"
#include "model/Energy.hpp"
#include "model/Fade.hpp"
#include "model/GameSession.hpp"
#include "model/Pad.hpp"
#include "model/Passenger.hpp"
#include "model/Rain.hpp"
#include "model/Water.hpp"

namespace ugh::model {

/**
 * The running level attempt: the copters, pads, passengers, enemies and bonus items, the water and the rain, the
 * energy and the fade; and the questions the states of the entities ask about each other (which copter landed on
 * a pad, which one touches a sprite ...). The states get the Level, so it also leads to the game session, the data,
 * the events and the diagnostics.
 *
 * The entities live in fixed slots like in the original (2 copters, 10 pads, 16 passengers, 5 enemies, 12 bonus
 * items); the level lists use the first slots, and a load sets only what the original sets.
 */
class Level {
public:
    static constexpr int COPTERS = 2, PADS = 10, PASSENGERS = 16, ENEMIES = 5;
    static constexpr int NONE = -1;   // no copter, no passenger

    struct Snapshot {
        Energy energy;
        Fade fade;
        bool done = false;            // all passengers delivered
        uint8_t passengersLeft = 0;   // to deliver
        uint8_t wind = 0;             // 0 none, 1 to the left, 2 to the right; rain with it
        int padCount = 0, passengerCount = 0, enemyCount = 0;   // the lengths of the level lists
    };

    Level(const data::GameData& data, GameSession& session, core::EventListener& events, core::Diagnostics& diagnostics);
    Level(const Level&) = delete;
    Level& operator=(const Level&) = delete;

    /** The program start: every entity and value as before the first game. */
    void reset();

    // ------------------------------------------------------------ the attempt

    /** 113b:3d66 - a new attempt: no key held, the water's counters, full energy, fading in, not done. */
    void startAttempt();

    /** 113b:3976 - the values of the level and its lists: the pads, how many passengers and enemies there are. */
    void loadLists(const data::LevelDefinition& definition);

    /** The level as the data defines it; nullptr when there is no such level. */
    const data::LevelDefinition* definition() const { return session_.levelDefinition(); }

    /** 113b:149c - a passenger finished its route; the last one ends the level. */
    void passengerFinished();

    /** How much the water rises (negative) or sinks every second frame; nothing without a level. */
    core::Word waterSpeed() const;

    bool done() const { return s_.done; }
    uint8_t wind() const { return s_.wind; }
    bool windy() const { return s_.wind != 0; }

    /** 113b:0b4f resetDrawnSprites: nothing of the level lists is drawn yet. */
    void hideSprites();

    // ------------------------------------------------------------ the systems of a frame

    /** 113b:1486 - Passengers.kt passengersUpdate: every passenger's state. */
    void updatePassengers();
    /** 113b:2363 - Objects.kt objectsUpdate: every enemy's state. */
    void updateEnemies();
    /** Frame.kt frameAfterKeys (the drawing of the passengers): the pixel positions the states read next frame. */
    void updatePassengerPixels();

    // ------------------------------------------------------------ the parts

    Copter& copter(int player) { return copters_[player]; }
    const Copter& copter(int player) const { return copters_[player]; }
    int copterCount() const { return session_.copterCount(); }
    Pad& pad(int i) { return pads_[i]; }
    const Pad& pad(int i) const { return pads_[i]; }
    int padCount() const { return s_.padCount; }
    Passenger& passenger(int i) { return passengers_[i]; }
    const Passenger& passenger(int i) const { return passengers_[i]; }
    int passengerCount() const { return s_.passengerCount; }
    Enemy& enemy(int i) { return enemies_[i]; }
    const Enemy& enemy(int i) const { return enemies_[i]; }
    int enemyCount() const { return s_.enemyCount; }
    BonusSlots& bonuses() { return bonuses_; }
    const BonusSlots& bonuses() const { return bonuses_; }
    Water& water() { return water_; }
    const Water& water() const { return water_; }
    Rain& rain() { return rain_; }
    const Rain& rain() const { return rain_; }
    Energy& energy() { return s_.energy; }
    Fade& fade() { return s_.fade; }
    const Fade& fade() const { return s_.fade; }

    GameSession& session() { return session_; }
    const GameSession& session() const { return session_; }
    const data::GameData& data() const { return data_; }
    void report(const core::Event& event) { events_.onEvent(event); }
    core::Diagnostics& diagnostics() { return diagnostics_; }

    // ------------------------------------------------------------ what the states ask

    /** The background is solid at the pixel index (the collision mask; nothing without a level). */
    bool solid(int index) const;

    /** The first copter landed on the pad; NONE when there is none. */
    int copterLandedOn(int pad) const;

    /** A copter with room for a passenger landed on the pad. */
    bool emptyCopterLandedOn(int pad) const;

    /** A copter floating on the water; `withRoom`: with room for a passenger, `still`: not moving up or down. */
    int copterOnWater(bool withRoom, bool still) const;

    /** 113b:2276, 22f1, 2207 - the first copter touching the box of a sprite at x, y; NONE when there is none. */
    int copterTouching(const data::Box& box, core::Fixed x, core::Fixed y) const;

    /** 113b:2196 - a standing passenger falling from a copter close to the enemy; NONE when there is none. */
    int fallingPassengerNear(const Enemy& enemy) const;

    const Snapshot& snapshot() const { return s_; }
    void restore(const Snapshot& snapshot) { s_ = snapshot; }

private:
    const data::GameData& data_;
    GameSession& session_;
    core::EventListener& events_;
    core::Diagnostics& diagnostics_;

    Snapshot s_;
    std::array<Copter, COPTERS> copters_;
    std::array<Pad, PADS> pads_;
    std::array<Passenger, PASSENGERS> passengers_;
    std::array<Enemy, ENEMIES> enemies_;
    BonusSlots bonuses_;
    Water water_;
    Rain rain_;
};

}  // namespace ugh::model
