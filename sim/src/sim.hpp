// UGH! game logic core - internal declarations.
//
// The state is kept the way the original keeps it: a 64 KB DGROUP with the variables at their original offsets
// (plus the few CS variables: random numbers, last scancode). The game logic is ported from the Kotlin port
// (core/src/main/kotlin/ugh/core/game) routine by routine with the same helpers (u, d, setD ...) and the same
// register passing (Regs), so every function names the routine of the original (113b:xxxx) and its Kotlin port.
// The replay fields (fields.cpp) are a view of this memory, like verify/.../replay/StateProjection.kt; a field
// is known once it was set from outside or written by the logic.
#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "flow.hpp"

namespace ugh {

inline int s16(int v) { return static_cast<int16_t>(static_cast<uint16_t>(v)); }
inline int w16(int v) { return v & 0xffff; }
inline int b8(int v) { return v & 0xff; }

/** The data exported by the extractor (format UGHSIM01, re/notes/phase2-data.md). */
struct Data {
    std::vector<uint8_t> dgroup = std::vector<uint8_t>(0x10000);  // initialized DGROUP, zero beyond
    std::vector<uint8_t> maps;
    std::map<int, std::vector<uint8_t>> masks;                    // level record -> collision mask

    bool load(const std::string& path, std::string& error);
};

constexpr int MASK_WIDTH = 384, MASK_HEIGHT = 192, MASK_ROW_BYTES = MASK_WIDTH / 8;

/** Registers handed from routine to routine, as in the original (Kotlin: Regs). */
struct Regs {
    int ax = 0, bx = 0, cx = 0, dx = 0, si = 0, di = 0, bp = 0;
};

// DGROUP variables (Game.kt companion)
constexpr int V_ENERGY = 0x2622, V_DIFFICULTY = 0x2638, V_FADE = 0x27a8, V_FADE_STEP = 0x27aa;
constexpr int V_WIND = 0x28f5, V_WATER_ROW = 0x2903, V_ROW_BYTES = 0x00c3, CRASH_LIMITS = 0x262e;
constexpr int KEY_UP = 0x278c, KEY_DOWN = 0x2790, KEY_LEFT = 0x2794, KEY_RIGHT = 0x2798, KEY_FIRE = 0x279c;
constexpr int P_XF = 0x27d0, P_YF = 0x27d4, P_X = 0x27d8, P_Y = 0x27dc, P_EFFORT = 0x27f4, P_LANDED = 0x27f8;
constexpr int P_VX = 0x2810, P_VY = 0x2814, P_IMPACT = 0x2818;
constexpr int PAD_LEFT = 0x290d, PAD_RIGHT = 0x2921, PAD_Y = 0x2935;
constexpr int PLAYERS = 0x2634, PLAYERS2 = 0x2636;

class Sim {
public:
    explicit Sim(Data data);
    Sim(const Sim&) = delete;
    Sim& operator=(const Sim&) = delete;

    // fields.cpp: the replay fields
    void reset();   // initial memory, everything unknown
    void clear();   // the replay fields unknown; what the replay does not hold keeps its value
    int set(const std::string& field, const std::string& value);
    std::vector<std::pair<std::string, std::string>> fields() const;

    // game.cpp, passengers.cpp, bonuses.cpp
    void key(int scancode);
    void newGame();
    int levelEnd();
    void levelStart();
    void playFrame();

    /**
     * The whole game from the start of 113b:0c61 (playGame): runs to the next retrace wait. 0 = waiting there,
     * 1 = game over, 2 = all levels done (the game returned without another retrace wait).
     */
    int step();

    /** Problems found while running (situations the core does not support); the replay player reports them. */
    std::vector<std::string> problems;

    // memory of the original (Game.kt: u, d, d8, setD, setD8, addD)
    int u(int off) const { return mem_[w16(off)] | (mem_[w16(off + 1)] << 8); }
    int d(int off) const { return s16(u(off)); }
    int d8(int off) const { return mem_[w16(off)]; }
    void setD(int off, int v) { setD8(off, v); setD8(off + 1, v >> 8); }
    void setD8(int off, int v) { mem_[w16(off)] = static_cast<uint8_t>(v); known_[w16(off)] = 1; }
    void addD(int off, int v) { setD(off, u(off) + v); }
    bool known(int off, int length) const;

private:
    Data data_;
    std::vector<uint8_t> mem_ = std::vector<uint8_t>(0x10000);
    std::vector<uint8_t> known_ = std::vector<uint8_t>(0x10000);
    std::array<int, 4> rng_{};   // CS:4ef7, 4ef9, 4efb, 4efd
    bool rngKnown_ = false;
    bool rainKnown_ = false;     // the replay holds only a checksum of the raindrops
    int lastScancode_ = 0;       // CS:4509: the last scancode of the keyboard handler
    int savedWaterRow_ = -1;     // the caption keeps the level's water row on the stack

    // the game flow (game.cpp): coroutines that wait for the retrace like the original
    Task game_;
    std::coroutine_handle<> waiting_;
    int result_ = 0;
    Retrace vsync() { return Retrace{&waiting_}; }
    Task playGame();
    Task levelSetup();
    Task levelCaption();
    Task playLevel();
    Task blackPalette();
    Task fadeIn();
    Task fadeOut();
    Task waitKey();
    void levelSetupStart();
    void frameBody();
    int readScancode();
    void resetDrawnSprites();

    friend struct Fields;

    void derive();
    void prepare();
    int random(int range);
    int levelRecord() const;
    int times3quarter(int y);
    int times3half(int x);

    // game.cpp
    void loadLevel();
    void spawnRaindrop(int bx);
    void moveRain();
    void updateWater();
    void drawWaterSurface();
    void copterUpdate(int bx);
    void moveHorizontally(int bx, int ch);
    void moveVertically(int bx, int ch);
    void bounceVertically(int bx, int bp);
    void drawCopter(int bx);
    void drawPassengers();
    bool probe(int si, int plane) const;

    // passengers.cpp
    void passengersUpdate(Regs& r);
    void passengerState(int addr, Regs& r);
    void jumpVia(Regs& r, int off) { passengerState(u(r.si + off), r); }
    void nextFrame(Regs& r, int table);
    bool decZero(int off);
    bool animTick(Regs& r);
    void animReset(int bx);
    bool fellIntoWater(Regs& r);
    void switchToWaterSet(Regs& r);
    bool hitByCopter(Regs& r);
    bool touchesPlayer(Regs& r);
    int playerOnPad(Regs& r, int pad);
    void walkToCopter(Regs& r);
    void floatOnSurface(Regs& r);
    void gone(Regs& r);
    void p149cNextStop(Regs& r);
    void p1509Arriving(Regs& r);
    void p153bAppear(Regs& r);
    void p1582Appearing(Regs& r);
    void p15b4StartWaiting(Regs& r);
    void p15d7Waiting(Regs& r);
    void p16f6StartCalling(Regs& r);
    void p172aCalling(Regs& r);
    void p17e6StartImpatient(Regs& r);
    void p180aImpatient(Regs& r);
    void p18c8StartBoarding(Regs& r);
    void p18e6Boarding(Regs& r);
    void p19e0Board(Regs& r, bool switchDescriptor);
    void p1a42Riding(Regs& r);
    void p1a7ePaid(Regs& r);
    void p1b29WalkingAway(Regs& r);
    void p1bbeStartEntering(Regs& r);
    void p1bd6Entering(Regs& r);
    void p1c0fStartStanding(Regs& r);
    void p1c27Standing(Regs& r);
    void p1c48Grabbed(Regs& r);
    void p1c6bHanging(Regs& r);
    void p1c81Dropped(Regs& r);
    void p1ceeFalling(Regs& r);
    void p1da8StartSplash(Regs& r);
    void p1dd5Splash(Regs& r, bool advance);
    void p1e9cStartSinking(Regs& r);
    void p1ec0Sinking(Regs& r);
    void p1f24StartSwimming(Regs& r);
    void p1f43Swimming(Regs& r);
    void p1fe2SwimCalling(Regs& r);
    void p2068SwimWaving(Regs& r);
    void p20c1SwimBoarding(Regs& r);

    // objects.cpp
    void objectsUpdate(Regs& r);
    void objectState(int addr, Regs& r);
    void jumpObj(Regs& r, int off) { objectState(u(r.si + off), r); }
    void addScore(int v);
    void objFrame(Regs& r, int table);
    int facingTable(Regs& r, int off);
    bool passengerHitsObject(Regs& r, bool setSprite = true);
    bool playerOnObjectPad(Regs& r);
    void faceCopter(Regs& r);
    bool fallingPassengerNear(Regs& r);
    bool copterTouchesObject(Regs& r);
    void o2379FlyerInit(Regs& r);
    void o239fFlyerWait(Regs& r);
    void o23b0FlyerScreech(Regs& r);
    void o23d9FlyerWait2(Regs& r);
    void o23eaFlyerStart(Regs& r);
    void o2493Flying(Regs& r);
    void o252bFlyerHit(Regs& r);
    void o255eFlyerFalling(Regs& r);
    void o25b1WalkerInit(Regs& r);
    void o25c9Walking(Regs& r);
    void o2667StartWatching(Regs& r);
    void o2681Watching(Regs& r);
    void o272eStartCharging(Regs& r);
    void o2748Charging(Regs& r);
    void o2830StartRecovering(Regs& r);
    void o2844Recovering(Regs& r, int table);
    void o28eeWalkerStunned(Regs& r);
    void o2914Stunned(Regs& r);
    void o295bBlowerInit(Regs& r);
    void o2973Blowing(Regs& r);
    void o2a53BlowerStunned(Regs& r);
    void o2a76BlowerWait(Regs& r);
    void o2a87TreeInit(Regs& r);
    void o2ab5Tree(Regs& r);
    void o2b0cTreeCatch(Regs& r);
    void o2b58TreeWait(Regs& r);

    // bonuses.cpp
    void bonusesUpdate(Regs& r);
    void bonusState(int addr, Regs& r);
    void bonusSpawn(Regs& r);
    void b2be4Falling(Regs& r);
    void b2c97Landed(Regs& r);
    void b2ca9Lying(Regs& r);
    bool copterTouchesBonus(Regs& r);
};

}  // namespace ugh
