// SPDX-License-Identifier: GPL-3.0-only
// OpenPocket Bass: docs/pxrestore.js against a simulated stock BlackBox V20 that answers reads (0x23) and takes the
// official editor's writes (0x22) as PROTOCOL.md describes: object 5 into a staging buffer at address 0, the slot at
// 0xA0000000, the install at 0xF0000000 (+ 0x100 a cabinet); object 4 the presets, flush, valid. The backup (pxbackup.js)
// of a pedal, the pedal overwritten (as OpenPocket did), then restoreBank: every slot back, only the damaged ones written.
import assert from "node:assert/strict";
import { pack7, unpack7 } from "../docs/pxota.js";
import { backup, AMPCAB_LEN, PRESET_LEN } from "../docs/pxbackup.js";
import { restoreBank, bankFromZip, restoreTiming } from "../docs/pxrestore.js";

const check = (what, fn) => { fn(); console.log(`${what.padEnd(76)} ok`); };
const sum = (a) => ~a.reduce((s, b) => s + b, 0) & 0xff;
restoreTiming({ settle: 0, retry: 0 });

const bank = new Uint8Array(AMPCAB_LEN), presets = new Uint8Array(PRESET_LEN);
for (let i = 0; i < bank.length; i++) bank[i] = (i * 7 + (i >> 9)) & 0xff;
for (let i = 0; i < 20; i++) bank.set(new TextEncoder().encode(`AM4\0AMP ${i + 1}`), i * 0x2000);
for (let i = 0; i < 20; i++) bank.set(new TextEncoder().encode(`CAB\0IR ${i + 1}`), 0x28000 + i * 0x1000);
for (let i = 0; i < presets.length; i++) presets[i] = (i * 13 + 5) & 0xff;

const stage = new Uint8Array(0x2000);
let slotSel = -1, writes = { amp: 0, cab: 0, preset: 0 }, lost = 0;
const link = {
  q: [], drain() { this.q = []; }, close() {},
  send(pkt) {
    const b = Uint8Array.from(unpack7(pkt.slice(1, -1)));
    const dv = new DataView(b.buffer), object = b[6], addr = dv.getUint32(7, true), len = dv.getUint16(11, true);
    assert.equal(b[b.length - 1], sum(b.subarray(6, -1)), "checksum");
    if (b[2] === 0x22) {
      const data = b.subarray(14, 14 + len);
      if (object === 5) {
        if (addr < 0x2000) stage.set(data, addr);
        else if (addr === 0xa0000000) slotSel = data[0];
        else if ((addr & 0xf0000000) >>> 0 === 0xf0000000) {
          const cab = (addr & 0x100) !== 0, s = addr & 0xff;
          assert.equal(s, slotSel, "the install names the slot selected");
          if (cab) { bank.set(stage.subarray(0, 0x1000), 0x28000 + s * 0x1000); writes.cab++; }
          else { bank.set(stage.subarray(0, 0x2000), s * 0x2000); writes.amp++; }
        } else assert.fail(`object 5 write at ${addr.toString(16)}`);
      } else if (object === 4) {
        if (addr < PRESET_LEN) { presets.set(data, addr); writes.preset++; }
        else assert.ok(addr === 0xf0000000 || (addr >>> 0) - 0xe0000000 < 80, `object 4 write at ${addr.toString(16)}`);
      } else assert.fail(`write to object ${object}`);
      return true;
    }
    if (b[2] !== 0x23) return true;
    if (++lost % 97 === 0) return true;                 // now and then a reply is lost
    const src = object === 5 ? bank.subarray(addr - 0x70000000, addr - 0x70000000 + len) : presets.subarray(addr, addr + len);
    const r = new Uint8Array(len + 15);
    r.set([0, 0x59, 0x23]); const n = r.length - 7; r[3] = n & 0xff; r[4] = (n >> 8) & 0xff;
    r[6] = object; new DataView(r.buffer).setUint32(7, addr, true); new DataView(r.buffer).setUint16(11, len, true);
    r.set(src, 14); r[r.length - 1] = sum(r.subarray(6, -1));
    this.q.push([0xf0, ...pack7(r), 0xf7]);
    return true;
  },
  async read() { return this.q.shift() || null; },
};

const { zip } = await backup(link, "BlackBox_020", () => {});
const saved = bankFromZip(zip);
check("the backup ZIP gives the bank and the presets back", () => {
  assert.deepEqual(saved.ampcab, bank); assert.deepEqual(saved.presets, presets);
});
// OpenPocket's storage over the stock bank: amps 3..20 and cabinets 2..10 and 13..20 overwritten, presets 5 and 70 changed
for (let i = 2 * 0x2000; i < 0x28000; i++) bank[i] = 0xff;
for (let s of [1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 13, 14, 15, 16, 17, 18, 19]) bank.fill(0x4f, 0x28000 + s * 0x1000, 0x28000 + (s + 1) * 0x1000);
presets[4 * 92 + 3] ^= 0x55; presets[69 * 92] ^= 1;
const res = await restoreBank(link, saved);
check("restoreBank: everything reads back as the backup", () => { assert.ok(res.same); assert.deepEqual(bank, saved.ampcab); assert.deepEqual(presets, saved.presets); });
check("only the damaged slots were written: 18 amps, 17 cabinets, 2 presets", () => {
  assert.deepEqual([res.amps, res.cabs, res.presets, res.failed.length], [18, 17, 2, 0]);
  assert.deepEqual([writes.amp, writes.cab, writes.preset], [18, 17, 2]);
});
const again = await restoreBank(link, saved);
check("a second run writes nothing", () => assert.deepEqual([again.amps, again.cabs, again.presets, again.same], [0, 0, 0, true]));
console.log("restore test passed");
