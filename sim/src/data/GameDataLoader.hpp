// The factory of the game data.
#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "data/GameData.hpp"

namespace ugh::data {

/**
 * Reads the extractor's export "UGHSIM01" (assets/sim/ugh-sim.bin, extractor/src/main/kotlin/ugh/extractor/Sim.kt,
 * re/notes/phase2-data.md) and turns the original's tables into the types of GameData, with the arithmetic of the
 * level loader 113b:3976 (Level.kt loadLevel).
 *
 * It is the boundary of the data: it checks everything the logic relies on (pad indices in range, the kinds of
 * passengers and enemies the state machines know, descriptors that hand their state handlers over the way the
 * state machines of the core assume) and refuses an export that does not fit. The logic then needs no checks.
 */
class GameDataLoader {
public:
    /** The data of the export at `path`; nullptr with the reason in `error`. */
    static std::unique_ptr<const GameData> load(const std::string& path, std::string& error);

private:
    using Masks = std::map<uint16_t, std::vector<uint8_t>>;

    explicit GameDataLoader(GameData& data) : d_(data) {}

    GameData& d_;

    void build(Masks& masks);

    [[noreturn]] static void fail(const std::string& what);
    uint16_t word(int offset) const { return d_.image_.word(offset); }
    core::Word signedWord(int offset) const { return d_.image_.word(offset); }
    uint8_t byte(int offset) const { return d_.image_.byte(offset); }

    void checkSlots(uint16_t descriptor, const std::vector<uint16_t>& slots, const char* what) const;
    const Animation* animation(uint16_t origin);
    AnimationPair pair(int offset);
    Box box(uint16_t descriptor) const;
    const PassengerKind& passengerKind(uint16_t origin);
    const BonusKind& bonusKind(uint16_t origin);
    const EnemyKind& enemyKind(uint16_t origin);
    const Route& route(uint16_t origin);
    const DropList& drops(uint16_t origin);
    const LevelDefinition& level(uint16_t record, Masks& masks);
    void loadPads(LevelDefinition& level, int list);
    void loadPassengers(LevelDefinition& level, int list);
    void loadEnemies(LevelDefinition& level, int list);
    KeyBinding key(int entry) const;
};

}  // namespace ugh::data
