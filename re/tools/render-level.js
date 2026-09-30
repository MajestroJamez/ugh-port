// Renders UGH levels (tile map only) to PNG using data straight from UGH.EXE
'use strict';
const fs = require('fs'), path = require('path');
const { unice } = require('./unice');
const { indexedPng } = require('./png');
const exe = fs.readFileSync(process.argv[2]);
const outDir = process.argv[3]; fs.mkdirSync(outDir, { recursive: true });
const seg = (s, o = 0) => 0xa00 + (s - 0x1000) * 16 + o;
const G = seg(0x6c09), gw = o => exe.readUInt16LE(G + o);

const maps = unice(exe.subarray(seg(0x2a38)));                      // CODE_7: all level maps
const sprTab = exe.subarray(seg(0x6b63), seg(0x6b63) + 2656);       // CODE_27: 664 x {u16 off, u8 w, u8 h}
const banks = [[0, 0x2e1f], [0xd9, 0x47a7], [0x12a, 0x538a], [0x223, 0x5e83]];  // first sprite -> segment
// palette: 128 colours from CODE_9, duplicated into 128..255 (113b:466a)
const src = exe.subarray(seg(0x3bc6), seg(0x3bc6) + 384);
const pal = Buffer.alloc(768); src.copy(pal, 0); src.copy(pal, 384);

function drawSprite(px, W, n, x0, y0) {
  const e = n * 4, off = sprTab.readUInt16LE(e), w = sprTab[e + 2], h = sprTab[e + 3];
  let bank = banks[0]; for (const b of banks) if (n >= b[0]) bank = b;
  let p = seg(bank[1], off);
  for (let x = 0; x < w; x++) for (let y = 0; y < h; y++) { const c = exe[p++]; if (c) px[(y0 + y) * W + x0 + x] = c; }
}

const W = 320, H = 192;
const tables = { '1p': 0x3349, '2p': 0x33d5 };
for (const [mode, t] of Object.entries(tables)) {
  for (let i = 0; gw(t + 2 * i) !== 0xffff; i++) {
    const rec = gw(t + 2 * i), mapOff = gw(rec);
    const px = new Uint8Array(W * H);
    for (let r = 0; r < 16; r++) for (let c = 0; c < 20; c++) drawSprite(px, W, maps[mapOff + r * 20 + c], c * 16, r * 12);
    fs.writeFileSync(path.join(outDir, `${mode}_level${String(i + 1).padStart(2, '0')}.png`), indexedPng(W, H, px, pal));
  }
}
console.log('done');

// contact sheet of all 1-player levels at half size, 7 columns
{
  const t = tables['1p'], cols = 7, cw = 160, ch = 96, n = (() => { let i = 0; while (gw(t + 2 * i) !== 0xffff) i++; return i; })();
  const rows = Math.ceil(n / cols), SW = cols * (cw + 4), SH = rows * (ch + 4), sheet = new Uint8Array(SW * SH);
  for (let i = 0; i < n; i++) {
    const mapOff = gw(gw(t + 2 * i)), px = new Uint8Array(W * H);
    for (let r = 0; r < 16; r++) for (let c = 0; c < 20; c++) drawSprite(px, W, maps[mapOff + r * 20 + c], c * 16, r * 12);
    const ox = (i % cols) * (cw + 4) + 2, oy = Math.floor(i / cols) * (ch + 4) + 2;
    for (let y = 0; y < ch; y++) for (let x = 0; x < cw; x++) sheet[(oy + y) * SW + ox + x] = px[(2 * y) * W + 2 * x];
  }
  fs.writeFileSync(path.join(outDir, '1p_all_levels.png'), indexedPng(SW, SH, sheet, pal));
}
