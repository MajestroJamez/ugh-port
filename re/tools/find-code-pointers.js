// Collects candidate code entry points that Ghidra's flow analysis could not reach:
//  1) immediates stored into the handler tables  MOV word ptr [.. + 0x2a0d/0x2cf3/0x2d83], imm
//  2) DGROUP words that point into still-undisassembled parts of segment 113b (descriptor tables)
// Prints "113b:xxxx # reason" lines for DisassembleAt.java.
'use strict';
const fs = require('fs');
const [, , exePath, listingPath, extraPath] = process.argv;
const exe = fs.readFileSync(exePath);
const listing = fs.readFileSync(listingPath, 'utf8').split(/\r?\n/);
const G = 0xa00 + (0x6c09 - 0x1000) * 16;

// instruction starts per segment
const starts = new Set();
for (const l of listing) { const m = l.match(/^113b:([0-9a-f]{4}) /); if (m) starts.add(parseInt(m[1], 16)); }
const covered = new Uint8Array(0x5300);
{
  const s = [...starts].sort((a, b) => a - b);
  for (let i = 0; i < s.length - 1; i++) for (let o = s[i]; o < s[i + 1] && o - s[i] < 16; o++) covered[o] = 1;
}
const out = new Map();
const add = (o, why) => { if (!starts.has(o) && !out.has(o)) out.set(o, why); };

for (const l of listing) {
  const m = l.match(/^113b:([0-9a-f]{4})\s+MOV word ptr \[[^\]]*0x(2a0d|2cf3|2d83)\],0x([0-9a-f]+)/);
  if (m) add(parseInt(m[3], 16), `stored to [${m[2]}] at 113b:${m[1]}`);
}
// DGROUP descriptor area: words pointing into uncovered 113b code
for (let o = 0x7600; o < 0x7e00; o += 2) {
  const v = exe.readUInt16LE(G + o);
  if (v >= 0x0100 && v < 0x5290 && !covered[v]) add(v, `DGROUP:${o.toString(16)} descriptor word`);
}
if (extraPath) for (const l of fs.readFileSync(extraPath, 'utf8').split(/\r?\n/)) {
  const m = l.match(/^\s*([0-9a-f]{4}):([0-9a-f]{4})\s*(#.*)?$/i);
  if (m && m[1] === '113b') add(parseInt(m[2], 16), (m[3] || '# manual').slice(2));
}
for (const [o, why] of [...out].sort((a, b) => a[0] - b[0])) console.log(`113b:${o.toString(16).padStart(4, '0')} # ${why}`);
