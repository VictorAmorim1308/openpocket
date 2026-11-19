// SPDX-License-Identifier: GPL-3.0-only
// OpenPocket Bass: docs/pxbackup.js (the BlackBox backup page's reads) against a simulated stock BlackBox that answers
// command 0x23 as docs/STOCK-FIRMWARE.md 11.8 / 12.2 describe: the whole bank comes back byte for byte, the slots are
// named, a reply lost once is read again, the ZIP opens, only reads are ever sent.
import assert from "node:assert/strict";
import { execFileSync } from "node:child_process";
import { mkdtempSync, writeFileSync, rmSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { pack7, unpack7 } from "../docs/pxota.js";
import { backup, AMPCAB_LEN, PRESET_LEN, readRequest } from "../docs/pxbackup.js";

const check = (what, fn) => { fn(); console.log(`${what.padEnd(76)} ok`); };
const sum = (a) => ~a.reduce((s, b) => s + b, 0) & 0xff;

// the pedal's storage: amps 1 and 3 hold NAM captures, cabinet 2 an IR, the rest erased; presets random
const bank = new Uint8Array(AMPCAB_LEN).fill(0xff);
const put = (off, magic, name, fill, size = 7000) => {
  bank.set(new TextEncoder().encode(magic.padEnd(4, "\0")), off);
  bank.set(new TextEncoder().encode(name.padEnd(14, "\0")), off + 4);
  for (let i = 18; i < size; i++) bank[off + i] = (fill * 31 + i * 7) & 0xff;
};
put(0x0000, "NAM", "SVT CLASSIC", 1);
put(0x4000, "NAM", "B15 FLIP", 2);
put(0x28000 + 0x1000, "CAB", "8X10 SVT", 3, 3092);
for (let i = 0x28000 + 0x1000 + 18; i < 0x28000 + 0x1000 + 3092; i++) bank[i] = i & 0xff;
const presets = Uint8Array.from({ length: PRESET_LEN }, (_, i) => (i * 13 + 5) & 0xff);

let reads = 0, drops = 0, others = 0;
const link = {
  q: [],
  drain() { this.q = []; },
  close() {},
  send(pkt) {
    const b = Uint8Array.from(unpack7(pkt.slice(1, -1)));
    if (b[2] !== 0x23) { others++; return true; }
    const dv = new DataView(b.buffer), object = b[6], addr = dv.getUint32(7, true), len = dv.getUint16(11, true);
    reads++;
    if (reads === 37) { drops++; return true; }           // one reply lost: must be read again
    const src = object === 5 ? bank.subarray(addr - 0x70000000, addr - 0x70000000 + len) : presets.subarray(addr, addr + len);
    const r = new Uint8Array(len + 15);
    r.set([0, 0x59, 0x23]); const n = r.length - 7; r[3] = n & 0xff; r[4] = (n >> 8) & 0xff; r[5] = n >> 16;
    r[6] = object; new DataView(r.buffer).setUint32(7, addr, true); new DataView(r.buffer).setUint16(11, len, true);
    r.set(src, 14); r[r.length - 1] = sum(r.subarray(6, -1));
    this.q.push([0xf0, ...pack7(r), 0xf7]);
    return true;
  },
  async read() { return this.q.shift() || null; },
};

const { info, zip } = await backup(link, "BlackBox_021", () => {});
check("only reads (0x23) were sent, never a write", () => assert.equal(others, 0));
check("a lost reply was read again", () => assert.equal(drops, 1));
check("the request is the V21 verifier's (object 5, 0x70000000, 512)", () =>
  assert.deepEqual(readRequest(5, 0x70000000, 512).slice(0, 2), [0xf0, 0x00]));
check("slots named: amp 1 SVT CLASSIC (NAM), amp 3 B15 FLIP, IR 2 8X10 SVT, the others empty", () => {
  assert.equal(info.amps[0].name, "SVT CLASSIC"); assert.equal(info.amps[0].magic, "NAM");
  assert.equal(info.amps[2].name, "B15 FLIP"); assert.equal(info.cabs[1].name, "8X10 SVT"); assert.equal(info.cabs[1].magic, "CAB");
  assert.equal(info.amps.filter((s) => s.empty).length, 18, JSON.stringify(info.amps)); assert.equal(info.cabs.filter((s) => s.empty).length, 19, JSON.stringify(info.cabs));
});
const dir = mkdtempSync(join(tmpdir(), "opbackup-"));
try {
  writeFileSync(join(dir, "b.zip"), zip);
  const out = execFileSync("python3", ["-c", `
import zipfile,hashlib,json,sys
z=zipfile.ZipFile(sys.argv[1]); assert z.testzip() is None
n=z.namelist(); i=json.loads(z.read("info.json"))
a=z.read("ampcab.bin"); p=z.read("presets.bin")
assert hashlib.sha256(a).hexdigest()==i["ampcab"]["sha256"] and hashlib.sha256(p).hexdigest()==i["presets"]["sha256"]
sys.stdout.write(json.dumps({"names":n,"a":a.hex()[:0],"alen":len(a),"plen":len(p)}))
`, join(dir, "b.zip")]).toString();
  const r = JSON.parse(out);
  check("the ZIP opens (CRC checked by Python's zipfile), SHA-256 as info.json says", () => {
    assert.equal(r.alen, AMPCAB_LEN); assert.equal(r.plen, PRESET_LEN);
  });
  check("each filled slot also as its own file", () => {
    assert.ok(r.names.includes("amps/01_SVT CLASSIC_NAM.bin") && r.names.includes("amps/03_B15 FLIP_NAM.bin"));
    assert.ok(r.names.includes("irs/02_8X10 SVT_CAB.bin"));
  });
  const a = execFileSync("python3", ["-c", "import zipfile,sys;sys.stdout.buffer.write(zipfile.ZipFile(sys.argv[1]).read('ampcab.bin'))", join(dir, "b.zip")]);
  check("the bank in the ZIP is the pedal's, byte for byte", () => assert.ok(Buffer.from(bank).equals(a)));
} finally { rmSync(dir, { recursive: true, force: true }); }

// two reads that differ (the bank changed between them, valid replies both times): nothing is offered
const before = reads;
const changing = { ...link, q: [], send(pkt) { const ok = link.send.call(this, pkt); if (reads === before + 480) bank[1000] ^= 1; return ok; } };
await assert.rejects(backup(changing, "BlackBox_021"), /diferentes/);
console.log("two reads that differ: no backup offered".padEnd(76) + " ok");
console.log("backup test passed");
