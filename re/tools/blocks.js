// Characterize the data blocks of UGH.EXE (Ghidra load base = segment 0x1000)
const fs = require('fs');
const d = fs.readFileSync(process.argv[2]);
const blocks = fs.readFileSync(process.argv[3], 'utf8').split('\n').filter(l => /^CODE_\d+ /.test(l)).map(l => {
  const m = l.match(/^(\S+)\s+([0-9a-f]{4}):([0-9a-f]{4}) - [0-9a-f]{4}:[0-9a-f]{4}\s+size=(\d+)/);
  return { name: m[1], seg: parseInt(m[2], 16), off: parseInt(m[3], 16), size: +m[4] };
});
for (const b of blocks) {
  const fo = 0xA00 + (b.seg - 0x1000) * 16 + b.off;
  const buf = d.subarray(fo, fo + b.size);
  const hist = new Array(256).fill(0); for (const x of buf) hist[x]++;
  let H = 0; for (const c of hist) if (c) { const p = c / buf.length; H -= p * Math.log2(p); }
  const top = hist.map((c, i) => [i, c]).sort((a, b) => b[1] - a[1]).slice(0, 4).map(([i, c]) => i.toString(16).padStart(2, '0') + ':' + (100 * c / buf.length).toFixed(0) + '%').join(' ');
  const max = Math.max(...buf);
  console.log(`${b.name.padEnd(8)} ${b.seg.toString(16)} file@${fo.toString(16).padStart(6, '0')} size=${String(b.size).padStart(6)} H=${H.toFixed(2)} max=${max.toString(16)} top=${top}  ${buf.subarray(0, 16).toString('hex')}`);
}
