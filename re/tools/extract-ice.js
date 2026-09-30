// Unpacks all ICE! blocks from UGH.EXE into re/out/assets
'use strict';
const fs = require('fs'), path = require('path');
const { unice } = require('./unice');
const { indexedPng } = require('./png');
const exe = fs.readFileSync(process.argv[2]);
const outDir = process.argv[3]; fs.mkdirSync(outDir, { recursive: true });
const seg2file = (seg, off = 0) => 0xa00 + (seg - 0x1000) * 16 + off;
const pal0 = exe.subarray(seg2file(0x3bc6), seg2file(0x3bc6) + 768);   // CODE_9 palette
const blocks = { CODE_7: [0x2a38, 0], CODE_10: [0x3bf6, 0], CODE_11: [0x3d9a, 0], CODE_12: [0x3f9e, 0], CODE_13: [0x3fb7, 0] };
for (const [name, [seg, off]] of Object.entries(blocks)) {
  const src = exe.subarray(seg2file(seg, off));
  const out = unice(src);
  fs.writeFileSync(path.join(outDir, name + '.bin'), out);
  let info = `${name}: packed=${src.readUInt32BE(4)} unpacked=${out.length}`;
  if (out.length === 0xfd00) {
    fs.writeFileSync(path.join(outDir, name + '.png'), indexedPng(320, 200, out.subarray(0, 64000), out.subarray(64000)));
    info += ' -> 320x200 image + own palette';
  } else if (out.length === 32000) {
    fs.writeFileSync(path.join(outDir, name + '_320x100.png'), indexedPng(320, 100, out, pal0));
    fs.writeFileSync(path.join(outDir, name + '_160x200.png'), indexedPng(160, 200, out, pal0));
    info += ' -> unknown 32000 B, tried 320x100 / 160x200 with CODE_9 palette';
  }
  console.log(info);
}
