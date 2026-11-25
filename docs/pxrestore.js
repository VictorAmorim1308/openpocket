// SPDX-License-Identifier: GPL-3.0-only
// OpenPocket Bass: put a BlackBox backup (docs/pxbackup.js) back into a BlackBox running the official V20, over USB MIDI,
// with the official editor's own write sequences (github.com/Bulrok/mvave-blackbox-linux PROTOCOL.md), as they were run
// and checked on a real V20 on 2026-11-19 (all 20 amps and 20 cabinets back byte for byte). Every write is read back and
// compared with the backup; a slot that already matches is not written.
//   amp / cabinet (object 5): the record into the staging buffer (address 0, 1015-byte pieces, its name blanked), the
//     slot at 0xA0000000, the 20-byte header at address 0, then the install at 0xF0000000 + slot (+ 0x100: a cabinet)
//   preset (object 4): the 92 bytes at N x 92, the flush at 0xF0000000 (no data), slot N valid at 0xE0000000 + N <- 01
import { pack7 } from "./pxota.js";
import { readChunk, readBank, AMPCAB_LEN, PRESET_LEN } from "./pxbackup.js";

const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
export const RESTORE_FIRMWARE = "BlackBox_020";          // the only official firmware these writes were checked on

export function writeFrame(object, addr, payload) {
  const f = [0x00, 0x59, 0x22, (payload.length + 8) & 0xff, (payload.length + 8) >> 8, 0x00, object,
    addr & 0xff, (addr >>> 8) & 0xff, (addr >>> 16) & 0xff, (addr >>> 24) & 0xff,
    payload.length & 0xff, payload.length >> 8, 0x00, ...payload];
  let s = 0; for (let i = 6; i < f.length; i++) s += f[i];
  f.push(~s & 0xff);
  return [0xf0, ...pack7(Uint8Array.from(f)), 0xf7];
}

async function put(link, object, addr, payload, pause = 60) {
  if (!link.send(writeFrame(object, addr, Array.from(payload)))) throw new Error("a BlackBox desconectou do MIDI");
  if (pause) await sleep(pause);
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
// a backup ZIP -> { ampcab, presets } or throws
export function bankFromZip(bytes) {
  const f = unzip(bytes);
  const pick = (end) => f[Object.keys(f).find((n) => n.endsWith(end))];
  const ampcab = pick("ampcab.bin"), presets = pick("presets.bin");
  if (!ampcab || ampcab.length !== AMPCAB_LEN || !presets || presets.length !== PRESET_LEN)
    throw new Error("este arquivo não é um backup da BlackBox (ampcab.bin e presets.bin)");
  return { ampcab: Uint8Array.from(ampcab), presets: Uint8Array.from(presets) };
}

const same = (a, b) => a.length === b.length && a.every((x, i) => x === b[i]);
const T = { settle: 1500, retry: 700 };
export function restoreTiming(t) { Object.assign(T, t); }

// one amp (kind "amp", 8 KB) or cabinet ("cab", 4 KB) into slot 0..19; true once it reads back as record
export async function restoreModel(link, kind, slot, record) {
  const size = kind === "amp" ? 0x2000 : 0x1000;
  if (record.length !== size) throw new Error(`registro de ${kind} com tamanho ${record.length}`);
  const staged = record.slice(); staged.fill(0, 4, 18);
  for (let attempt = 0; attempt < 2; attempt++) {
    for (let o = 0; o < size; o += 1015) await put(link, 5, o, staged.subarray(o, Math.min(size, o + 1015)));
    await put(link, 5, 0xa0000000, [slot]);
    await put(link, 5, 0, record.subarray(0, 20));
    await put(link, 5, (0xf0000000 + (kind === "cab" ? 0x100 : 0) + slot) >>> 0, [0], T.settle);
    const base = 0x70000000 + (kind === "amp" ? slot * 0x2000 : 0x28000 + slot * 0x1000);
    for (let t = 0; t < 8; t++) {                     // the main loop writes the flash: give it time
      try { if (same(await readBank(link, 5, base, size, 512), record)) return true; } catch (_) { /* read again */ }
      await sleep(T.retry);
    }
  }
  return false;
}

export async function restorePreset(link, slot, record) {
  for (let attempt = 0; attempt < 2; attempt++) {
    await put(link, 4, slot * 92, record);
    await put(link, 4, 0xf0000000, []);
    await put(link, 4, (0xe0000000 + slot) >>> 0, [1], 400);
    for (let t = 0; t < 6; t++) {
      try { if (same(await readChunk(link, 4, slot * 92, 92), record)) return true; } catch (_) { /* read again */ }
      await sleep(T.retry / 2);
    }
  }
  return false;
}

// the whole bank: the slots that differ from the backup written, then everything read again and compared.
// onProgress(0..1, text). Returns { amps, cabs, presets: slots written, failed: [names], same: the final check }
export async function restoreBank(link, bank, onProgress = () => {}, log = () => {}) {
  onProgress(0, "lendo o que está na pedaleira…");
  const cur = await readBank(link, 5, 0x70000000, AMPCAB_LEN, 512);
  const curP = await readBank(link, 4, 0, PRESET_LEN, 92);
  const jobs = [];
  for (let i = 0; i < 20; i++) jobs.push({ kind: "amp", slot: i, off: i * 0x2000, size: 0x2000 });
  for (let i = 0; i < 20; i++) jobs.push({ kind: "cab", slot: i, off: 0x28000 + i * 0x1000, size: 0x1000 });
  const todo = jobs.filter((j) => !same(cur.subarray(j.off, j.off + j.size), bank.ampcab.subarray(j.off, j.off + j.size)));
  const ptodo = [];
  for (let i = 0; i < 80; i++) if (!same(curP.subarray(i * 92, i * 92 + 92), bank.presets.subarray(i * 92, i * 92 + 92))) ptodo.push(i);
  const res = { amps: 0, cabs: 0, presets: 0, failed: [], same: false };
  const total = todo.length + ptodo.length / 8 || 1;
  let done = 0;
  for (const j of todo) {
    const name = `${j.kind === "amp" ? "AMP" : "IR"} ${j.slot + 1}`;
    onProgress(done / total, `gravando ${name}…`);
    const ok = await restoreModel(link, j.kind, j.slot, bank.ampcab.slice(j.off, j.off + j.size));
    log(`${name}: ${ok ? "ok" : "FALHOU"}`);
    if (ok) res[j.kind === "amp" ? "amps" : "cabs"]++; else res.failed.push(name);
    done++;
  }
  for (const i of ptodo) {
    onProgress(done / total, `gravando o preset ${i + 1}…`);
    const ok = await restorePreset(link, i, bank.presets.slice(i * 92, i * 92 + 92));
    if (ok) res.presets++; else res.failed.push(`PRESET ${i + 1}`);
    done += 1 / 8;
  }
  onProgress(0.99, "conferindo tudo…");
  const fin = await readBank(link, 5, 0x70000000, AMPCAB_LEN, 512);
  const finP = await readBank(link, 4, 0, PRESET_LEN, 92);
  res.same = same(fin, bank.ampcab) && same(finP, bank.presets);
  onProgress(1, res.same ? "tudo igual ao backup" : "nem tudo ficou igual ao backup");
  return res;
}

// ---- the backup kept in this browser (IndexedDB), so the way back finds it by itself ----
const DB = "openpocket-bass", STORE = "bank";
function db() {
  return new Promise((ok, no) => {
    const r = indexedDB.open(DB, 1);
    r.onupgradeneeded = () => r.result.createObjectStore(STORE);
    r.onsuccess = () => ok(r.result); r.onerror = () => no(r.error);
  });
}
export async function keepBank(entry) {   // { date, firmware, ampcab, presets }
  const d = await db();
  await new Promise((ok, no) => { const t = d.transaction(STORE, "readwrite"); t.objectStore(STORE).put(entry, "last"); t.oncomplete = ok; t.onerror = () => no(t.error); });
  d.close();
}
export async function keptBank() {
  try {
    const d = await db();
    const v = await new Promise((ok, no) => { const r = d.transaction(STORE).objectStore(STORE).get("last"); r.onsuccess = () => ok(r.result || null); r.onerror = () => no(r.error); });
    d.close();
    return v;
  } catch (_) { return null; }
}

export { AMPCAB_LEN, PRESET_LEN };
