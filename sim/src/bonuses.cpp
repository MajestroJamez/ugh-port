// Bonus items (12 slots, index = slot * 2) - 113b:2b7f .. 113b:2d1b, ported from Bonuses.kt.
// Arrays: 2d6b descriptor, 2d83 state handler, 2d9b sprite (0xffff = free), 2dcb / 2de3 x / y (1/32 px),
// 2e5b x speed / lifetime, 2e73 y speed.
#include "sim.hpp"

namespace ugh {

namespace {

constexpr int B_DESC = 0x2d6b, B_STATE = 0x2d83, B_SPRITE = 0x2d9b, B_X = 0x2dcb, B_Y = 0x2de3;
constexpr int B_VX = 0x2e5b, B_VY = 0x2e73;

}  // namespace

/**
 * 113b:2b96 - Bonuses.kt bonusSpawn: drops a bonus item, SI = descriptor, AX / BP = position, CX / DX = speed.
 * Original: with all 12 slots in use the routine returns without its POP BX and jumps to CS:BX.
 */
void Sim::bonusSpawn(Regs& r) {
    int savedBx = r.bx;
    r.bx = 0x16;
    while ((u(B_SPRITE + r.bx) & 0x8000) == 0) {
        r.bx = w16(r.bx - 2);
        if (s16(r.bx) < 0) {
            problems.push_back("all 12 bonus slots in use: the original would jump to CS:BX");
            r.bx = savedBx;
            return;
        }
    }
    setD(B_DESC + r.bx, r.si);
    r.di = w16(u(r.si) << 5);
    r.ax = w16(r.ax - r.di);
    setD(B_X + r.bx, r.ax);
    r.di = w16(u(r.si + 2) << 4);
    r.bp = w16(r.bp - r.di);
    setD(B_Y + r.bx, r.bp);
    setD(B_VX + r.bx, r.cx);
    r.dx = w16(-(r.dx + u(r.si + 0x0c)));
    setD(B_VY + r.bx, r.dx);
    r.ax = u(r.si + 0x10);
    setD(B_SPRITE + r.bx, r.ax);
    r.ax = u(r.si + 6);
    setD(B_STATE + r.bx, r.ax);
    r.bx = savedBx;
}

}  // namespace ugh
