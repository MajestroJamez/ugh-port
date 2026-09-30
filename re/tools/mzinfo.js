// MZ header + relocation analysis for UGH.EXE
const fs = require('fs');
const d = fs.readFileSync(process.argv[2]);
const w = o => d.readUInt16LE(o);
const h = { lastp: w(2), pages: w(4), nrel: w(6), hdr: w(8), minalloc: w(10), maxalloc: w(12), ss: w(14), sp: w(16), csum: w(18), ip: w(20), cs: w(22), relo: w(24), ovl: w(26) };
const imgStart = h.hdr * 16;
const imgSize = (h.pages - 1) * 512 + (h.lastp || 512) - imgStart;
console.log(h, 'imgStart', imgStart, 'imgSize', imgSize, 'fileSize', d.length);
const segs = new Map();
const targetSegs = new Map();
for (let i = 0; i < h.nrel; i++) {
  const off = w(h.relo + i * 4), seg = w(h.relo + i * 4 + 2);
  segs.set(seg, (segs.get(seg) || 0) + 1);
  const lin = imgStart + seg * 16 + off;
  const val = w(lin);
  targetSegs.set(val, (targetSegs.get(val) || 0) + 1);
}
console.log('reloc sites grouped by segment (seg: count):');
console.log([...segs].sort((a, b) => a[0] - b[0]).map(([s, c]) => s.toString(16) + ':' + c).join(' '));
console.log('relocated segment values (seg: refs):');
console.log([...targetSegs].sort((a, b) => a[0] - b[0]).map(([s, c]) => s.toString(16) + ':' + c).join(' '));
