// SPDX-License-Identifier: GPL-3.0-only
// OpenPocket Bass: put a BlackBox backup (docs/pxbackup.js's ZIP) back into a BlackBox running the official V20, over
// USB MIDI, with the official editor's own write sequences (reverse-engineered and verified on V20 by
// github.com/Bulrok/mvave-blackbox-linux, PROTOCOL.md): every write is read back and compared with the backup.
//   cabinet (object 5): the record into the staging buffer (address 0, 1015-byte pieces), slot at 0xA0000000, the
//                       install at 0xF0000100 + slot (bit 8: a cabinet, low byte: the slot)
//   amp     (object 5): the same, 8 KB, installed at 0xF0000000 + slot
//   preset  (object 4): the 92 bytes at N x 92, flush at 0xF0000000 (no data), slot valid at 0xE0000000 + N <- 01
import { pack7 } from "./pxota.js";
import { readChunk, readBank, AMPCAB_LEN, PRESET_LEN } from "./pxbackup.js";

const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

export function writeFrame(object, addr, payload) {
  const f = [0x00, 0x59, 0x22, (payload.length + 8) & 0xff, (payload.length + 8) >> 8, 0x00, object,
    addr & 0xff, (addr >>> 8) & 0xff, (addr >>> 16) & 0xff, (addr >>> 24) & 0xff,
    payload.length & 0xff, payload.length >> 8, 0x00, ...payload];
  let s = 0; for (let i = 6; i < f.length; i++) s += f[i];
  f.push(~s & 0xff);
  return [0xf0, ...pack7(Uint8Array.from(f)), 0xf7];
}

async function put(link, object, addr, payload, pause = 25) {
  if (!link.send(writeFrame(object, addr, Array.from(payload)))) throw new Error("a BlackBox desconectou do MIDI");
  await sleep(pause);
}

// a store-only ZIP (pxbackup.js zip): name -> bytes
export function unzip(buf) {
  const d = new DataView(buf.buffer, buf.byteOffset, buf.byteLength), out = {};
  let p = 0;
  while (p + 30 <= buf.length && d.getUint32(p, true) === 0x04034b50) {
    const n = d.getUint32(p + 18, true), nl = d.getUint16(p + 26, true), xl = d.getUint16(p + 28, true);
    const name = new TextDecoder().decode(buf.subarray(p + 30, p + 30 + nl));
    out[name] = buf.subarray(p + 30 + nl + xl, p + 30 + nl + xl + n);
    p += 30 + nl + xl + n;
  }
  return out;
}

const same = (a, b) => a.length === b.length && a.every((x, i) => x === b[i]);

export async function restoreModel(link, kind, slot, record) {
  const size = kind === "amp" ? 0x2000 : 0x1000;
  if (record.length !== size) throw new Error(`registro de ${kind} com tamanho ${record.length}`);
  for (let o = 0; o < size; o += 1015) await put(link, 5, o, record.subarray(o, Math.min(size, o + 1015)));
  await put(link, 5, 0xa0000000, [slot]);
  await put(link, 5, 0xf0000000 + (kind === "cab" ? 0x100 : 0) + slot, [0], 600);
  const base = 0x70000000 + (kind === "amp" ? slot * 0x2000 : 0x28000 + slot * 0x1000);
  let back = null;
  for (let t = 0; t < 10; t++) {                     // the main loop writes the flash: give it time
    back = await readBank(link, 5, base, size, 512);
    if (same(back, record)) return true;
    await sleep(500);
  }
  return back;
}

export async function restorePreset(link, slot, record) {
  await put(link, 4, slot * 92, record);
  await put(link, 4, 0xf0000000, []);
  await put(link, 4, 0xe0000000 + slot, [1], 400);
  for (let t = 0; t < 6; t++) {
    const back = await readChunk(link, 4, slot * 92, 92);
    if (same(back, record)) return true;
    await sleep(300);
  }
  return false;
}

export { AMPCAB_LEN, PRESET_LEN };
