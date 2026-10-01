// Prints a compact listing of seg:from..seg:to from re/out/listing.asm (no byte dumps, far targets as seg:off).
// Usage: node asm-range.js <seg> <fromHex> <toHex> [listing]
const fs = require('fs');
const path = require('path');
const [seg, from, to, file] = process.argv.slice(2);
const listing = file || path.join(__dirname, '..', 'out', 'listing.asm');
const lo = parseInt(from, 16), hi = parseInt(to, 16);
let last = -1;
for (const line of fs.readFileSync(listing, 'utf8').split(/\r?\n/)) {
  const m = /^([0-9a-f]{4}):([0-9a-f]{4})  (.*?)\s*;; len=/.exec(line);
  if (!m || m[1] !== seg) continue;
  const off = parseInt(m[2], 16);
  if (off < lo || off > hi || off === last) continue;
  last = off;
  let ins = m[3].replace(/\s+/g, ' ').trim();
  // "CALL 0x1000:7b43 -> FUN_1664_1503" -> "CALL 1664:1503"; keep data references short
  ins = ins.replace(/0x1000:[0-9a-f]+\s*-> (?:FUN_|LAB_)?([0-9a-f]{4})[_:]([0-9a-f]{4})/g, '$1:$2');
  ins = ins.replace(/0x1000:[0-9a-f]+\s*-> ([0-9a-f]{4}:[0-9a-f]{4})/g, '$1');
  ins = ins.replace(/\s*-> [0-9a-f]{4}:[0-9a-f]{4}(\s*-> [0-9a-f]{4}:[0-9a-f]{4})*/g, '');
  ins = ins.replace(/\s*-> Stack\[[^\]]*\]/g, '').replace(/;seg \S+/g, '');
  console.log(m[2] + ' ' + ins);
}
