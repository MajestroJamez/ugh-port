// Faithful port of the Pack-Ice depacker at 113b:4cad..4ddc in UGH.EXE.
// Works backwards from the end of the packed data, exactly like the x86 code.
'use strict';

// Tables read from UGH.EXE (113b:4ddd .. 113b:4e18)
const DIRECT_BITS = [0x000e, 0x0007, 0x0002, 0x0001, 0x0001];   // CS:[BP]      (BP = 4ddd..4de5)
const DIRECT_BASE = [0x010d, 0x000e, 0x0007, 0x0004, 0x0001];   // CS:[BP+0x0a]
const DIRECT_MARK = [0x7fff, 0x00ff, 0x0007, 0x0003, 0x0003];   // CS:[BP+0x14]
const LEN_BITS = [0x09, 0x01, 0x00, -1, -1];                    // CS:[BP+0x4e06], BP = -1..3
const LEN_BASE = [0x08, 0x04, 0x02, 0x01, 0x00];                // CS:[BP+0x4e0b]
const OFF_BITS = [0x0b, 0x04, 0x07];                            // CS:[BP+0x4e10], BP = -1..1
const OFF_BASE = [0x011f, -1, 0x001f];                          // CS:[2*BP+0x4e15]

function unice(src) {
  if (src.readUInt32BE(0) !== 0x49434521) throw new Error('not ICE!');
  const packedLen = src.readUInt32BE(4), unpackedLen = src.readUInt32BE(8);
  const out = Buffer.alloc(unpackedLen);
  let si = packedLen - 1, di = unpackedLen - 1;
  let ah = src[si--];

  const getbit = () => {
    let cf = (ah >> 7) & 1;
    ah = (ah << 1) & 0xff;
    if (ah === 0) {                 // SHL gave zero -> reload, RCL shifts CF in as sentinel
      ah = src[si--];
      const ncf = (ah >> 7) & 1;
      ah = ((ah << 1) | cf) & 0xff;
      cf = ncf;
    }
    return cf;
  };
  const getbits = (cx) => {         // reads cx+1 bits (x86 DEC CX / JNS loop)
    let dx = 0;
    do { dx = ((dx << 1) | getbit()) & 0xffff; cx--; } while (cx >= 0);
    return dx;
  };
  const s16 = v => (v << 16) >> 16;

  for (;;) {
    // literal run
    if (getbit()) {
      let dx = 0;
      if (getbit()) {
        let i = 4;                  // BP = 4de5 .. 4ddd
        for (;;) {
          dx = getbits(DIRECT_BITS[i]);
          if (dx !== DIRECT_MARK[i]) break;
          if (i === 0) break;
          i--;
        }
        dx = (dx + DIRECT_BASE[i]) & 0xffff;
      }
      do { out[di--] = src[si--]; dx = s16(dx - 1); } while (dx >= 0);
    }
    if (di === -1) break;

    // match
    let bp = 3;
    while (getbit()) { bp--; if (bp < 0) break; }
    let dx = 0;
    const lb = LEN_BITS[bp + 1];
    if (lb >= 0) dx = getbits(lb);
    let bx = (LEN_BASE[bp + 1] + dx) & 0xffff;
    if (bx !== 0) {
      bp = 1;
      while (getbit()) { bp--; if (bp < 0) break; }
      dx = getbits(OFF_BITS[bp + 1]);
      dx = (dx + OFF_BASE[bp + 1]) & 0xffff;
      if (s16(dx) < 0) dx = (dx - bx) & 0xffff;
    } else {
      let cx = 5, base = -1;
      if (getbit()) { cx = 8; base = 0x3f; }
      dx = (getbits(cx) + base) & 0xffff;
    }
    let s = (di + 2 + bx + dx) & 0xffff;
    out[di--] = out[s--];
    do { out[di--] = out[s--]; bx = s16(bx - 1); } while (bx >= 0);
  }
  return out;
}

module.exports = { unice };
