// Reports byte ranges (>= 8 B) of the code segments not covered by any instruction in listing.asm
'use strict';
const fs = require('fs');
const L = fs.readFileSync(process.argv[2], 'utf8').split(/\r?\n/);
const segs = { '1000': [0, 5048], '113b': [8, 0x5290], '1664': [0, 8515], '1878': [3, 0x1ba0], '1a32': [0, 848] };
const starts = {}; for (const s in segs) starts[s] = [];
for (const l of L) { const m = l.match(/^([0-9a-f]{4}):([0-9a-f]{4})  (.*)$/); if (m && starts[m[1]]) starts[m[1]].push(parseInt(m[2], 16)); }
for (const s in segs) {
  const cov = new Uint8Array(0x10000), st = [...new Set(starts[s])].sort((a, b) => a - b);
  for (let i = 0; i < st.length; i++) { const end = i + 1 < st.length ? Math.min(st[i + 1], st[i] + 16) : st[i] + 1; for (let o = st[i]; o < end; o++) cov[o] = 1; }
  const runs = []; let a = -1;
  for (let o = segs[s][0]; o <= segs[s][1]; o++) { if (o < segs[s][1] && !cov[o]) { if (a < 0) a = o; } else if (a >= 0) { if (o - a >= 8) runs.push([a, o]); a = -1; } }
  console.log(`${s}: ${runs.reduce((x, r) => x + r[1] - r[0], 0)} B uncovered in ${runs.length} runs: ` + runs.map(r => r[0].toString(16) + '-' + r[1].toString(16)).join(' '));
}
