// Reports byte ranges of the code segments not covered by any instruction in listing.asm.
// Uses the exact instruction lengths written by ExportListing.java (";; len=N").
// Usage: node coverage.js listing.asm [minRun]
'use strict';
const fs = require('fs');
const L = fs.readFileSync(process.argv[2], 'utf8').split(/\r?\n/);
const minRun = +(process.argv[3] || 1);
const segs = { '1000': [0, 5048], '113b': [8, 0x5290], '1664': [0, 8515], '1878': [3, 0x1ba0], '1a32': [0, 848] };
const cov = {}; for (const s in segs) cov[s] = new Uint8Array(0x10000);
let missingLen = 0;
for (const l of L) {
  const m = l.match(/^([0-9a-f]{4}):([0-9a-f]{4})  .*;; len=(\d+)/);
  if (!m) { if (/^[0-9a-f]{4}:[0-9a-f]{4}  /.test(l)) missingLen++; continue; }
  if (!cov[m[1]]) continue;
  const o = parseInt(m[2], 16);
  for (let i = 0; i < +m[3]; i++) cov[m[1]][o + i] = 1;
}
if (missingLen) console.log(`warning: ${missingLen} instructions without length (old listing format?)`);
for (const s in segs) {
  const runs = []; let a = -1;
  for (let o = segs[s][0]; o <= segs[s][1]; o++) {
    if (o < segs[s][1] && !cov[s][o]) { if (a < 0) a = o; } else if (a >= 0) { if (o - a >= minRun) runs.push([a, o]); a = -1; }
  }
  console.log(`${s}: ${runs.reduce((x, r) => x + r[1] - r[0], 0)} B uncovered in ${runs.length} runs: ` + runs.map(r => r[0].toString(16) + '-' + r[1].toString(16)).join(' '));
}
