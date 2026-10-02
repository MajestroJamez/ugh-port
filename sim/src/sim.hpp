// UGH! game logic core - internal declarations.
//
// Values are kept the way the original keeps them: raw 16-bit words (0 .. 0xffff) and bytes, signed where the
// code compares them signed (s16). -1 marks a value the core does not know (not set from outside, not
// computed yet). Every function names the routine of the original (113b:xxxx) and its Kotlin port
// (core/src/main/kotlin/ugh/core/game).
#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace ugh {

constexpr int UNKNOWN = -1;

inline int s16(int v) { return static_cast<int16_t>(static_cast<uint16_t>(v)); }
inline int w16(int v) { return v & 0xffff; }
inline int b8(int v) { return v & 0xff; }

/** The data exported by the extractor (format UGHSIM01, re/notes/phase2-data.md). */
struct Data {
    std::vector<uint8_t> dgroup = std::vector<uint8_t>(0x10000);  // initialized DGROUP, zero beyond
    std::vector<uint8_t> maps;
    std::map<int, std::vector<uint8_t>> masks;                    // level record -> collision mask

    bool load(const std::string& path, std::string& error);

    int u8(int off) const { return dgroup[w16(off)]; }
    int u16(int off) const { return u8(off) | (u8(off + 1) << 8); }
    int s16w(int off) const { return s16(u16(off)); }
};

constexpr int MASK_WIDTH = 384, MASK_HEIGHT = 192, MASK_ROW_BYTES = MASK_WIDTH / 8;
constexpr int RAINDROPS = 193;
constexpr int PADS = 10;
constexpr int KEY_AREA = 0x278c;  // key states of both players (DGROUP:278c .. 27a1, one byte per slot)

struct Copter {
    int xf = UNKNOWN, yf = UNKNOWN;        // 27d0 / 27d4, 1/32 px
    int x = UNKNOWN, y = UNKNOWN;          // 27d8 / 27dc, px
    int vx = UNKNOWN, vy = UNKNOWN;        // 2810 / 2814
    int landed = UNKNOWN;                  // 27f8, pad index, 0xffff = flying
    int effort = UNKNOWN;                  // 27f4
    int impact = UNKNOWN;                  // 2818
    int carrying = UNKNOWN;                // 27fc
    int targetPad = UNKNOWN;               // 2804
    int fare = UNKNOWN, fareMin = UNKNOWN; // 2808 / 280c
    int sprite = UNKNOWN;                  // 27e8
    int animCounter = UNKNOWN;             // 27f0
    bool keysKnown = false;
};

struct Pad {
    int left = UNKNOWN, right = UNKNOWN, y = UNKNOWN;          // 290d / 2921 / 2935
    int doorX = UNKNOWN, waitX = UNKNOWN, standX = UNKNOWN;    // 2949 / 295d / 2971
    int number = UNKNOWN, waiting = UNKNOWN;                   // 2985 / 2999
};

struct State {
    int level = UNKNOWN, players = UNKNOWN, difficulty = UNKNOWN;  // 261c / 2634 / 2638
    int lives = UNKNOWN, multiplier = UNKNOWN;                     // bytes 263c / 263d
    int scoreLo = UNKNOWN, scoreHi = UNKNOWN;                      // 261e / 2620
    int energy = UNKNOWN;                                          // 2622
    int fade = UNKNOWN, fadeStep = UNKNOWN;                        // 27a8 / 27aa
    int levelDone = UNKNOWN;                                       // byte 27cf, bit 7
    int wind = UNKNOWN;                                            // byte 28f5 (level record +0c)
    int waterRow = UNKNOWN;                                        // 2903
    int rainFloor = UNKNOWN;                                       // 2907
    int passengersLeft = UNKNOWN;                                  // byte 28f1 (level record +08)
    int waterYf = UNKNOWN;                                         // 28fe (level record +15)
    int waterHold = UNKNOWN, waterToggle = UNKNOWN;                // bytes 27a2 / 27ce
    int waterAnim = UNKNOWN, waterAnimDelay = UNKNOWN;             // 27a4 / byte 27a3
    std::array<int, 4> rng{UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN};    // CS:4ef7, 4ef9, 4efb, 4efd

    bool rainKnown = false;
    std::array<int, RAINDROPS> rainOffset{};                       // 2e8b: row * 0x60 + x / 4
    std::array<int, RAINDROPS> rainPlane{};                        // 318f: x & 3

    std::array<int, 0x16> keys{};                                  // DGROUP:278c + slot
    std::array<Copter, 2> copters;
    std::array<Pad, PADS> pads;

    // not part of the replay state
    int lastScancode = 0;        // CS:4509 (only ESC and P matter)
    int keyPosition = 0;         // 2647: position in a key sequence
    std::vector<uint8_t> keyMatch; // 2821 + 6 * entry: entry ruled out (0xff) for the current sequence
    int savedWaterRow = UNKNOWN; // the caption keeps the level's water row on the stack
};

class Sim {
public:
    explicit Sim(Data data) : data_(std::move(data)) {}

    State state;
    const Data& data() const { return data_; }

    // fields.cpp: the state as named replay fields
    void reset();   // everything unknown
    void clear();   // the replay fields unknown, the state the replay does not hold kept
    int set(const std::string& field, const std::string& value);
    std::vector<std::pair<std::string, std::string>> fields() const;

    // logic.cpp
    void key(int scancode);
    void newGame();
    int levelEnd();
    void levelStart();
    void playFrame();

    /** Problems found while running (unsupported situations); the replay player reports them. */
    std::vector<std::string> problems;

private:
    Data data_;

    int random(int range);
    int levelRecord() const;
    void loadLevel();
    void spawnRaindrop(int bx);
    void moveRain();
    void updateWater();
    void drawWaterSurface();
    void copterUpdate(int p);
    void moveHorizontally(int p, int ch);
    void moveVertically(int p, int ch);
    void bounceVertically(int p, int bp);
    void drawCopter(int p);
    bool probe(int si, int plane) const;
    int key16(int off) const;
};

}  // namespace ugh
