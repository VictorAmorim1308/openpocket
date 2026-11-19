// SPDX-License-Identifier: GPL-3.0-only
// OpenPocket Bass: a READ-ONLY backup of a BlackBox running the official firmware, over USB MIDI, before installing
// anything: the 20 amp models (NAM / AM captures), the 20 cabinets (IRs) and the 80 presets, as the stock firmware keeps
// them (docs/STOCK-FIRMWARE.md 10.4 and 12.2). Only command 0x23 (read) is ever sent: nothing on the pedal changes.
// The protocol is the one docs/pxfactory.js uses to verify the V21 factory sounds.
import { pack7, unpack7 } from "./pxota.js";

export const AMPCAB_LEN = 0x3c000;        // 20 x 8 KB amp slots, then 20 x 4 KB cabinet slots (pd40j 0x00000..0x3BFFF)
export const PRESET_LEN = 80 * 92;        // 80 presets x 92 bytes
const checksum = (bytes) => ~bytes.reduce((sum, b) => sum + b, 0) & 0xff;

export function readRequest(object, address, length) {
  const body = Uint8Array.of(0, 0x59, 0x23, 8, 0, 0, object, 0, 0, 0, 0, 0, 0, 0, 0);
  const fields = new DataView(body.buffer);
  fields.setUint32(7, address, true);
  fields.setUint16(11, length, true);
  body[14] = checksum(body.subarray(6, 14));
  return [0xf0, ...pack7(body), 0xf7];
}

// one read: the reply's data, or throws
export async function readChunk(link, object, address, length, timeout = 1800) {
  link.drain();
  if (!link.send(readRequest(object, address, length))) throw new Error("a BlackBox desconectou do MIDI");
  const end = Date.now() + timeout;
  while (Date.now() < end) {
    const packet = await link.read(end - Date.now());
    if (!packet) break;
    if (packet[0] !== 0xf0 || packet.at(-1) !== 0xf7) continue;
    const bytes = Uint8Array.from(unpack7(packet.slice(1, -1)));
    if (bytes[0] !== 0 || bytes[1] !== 0x59 || bytes.length < 8 || checksum(bytes.subarray(6, -1)) !== bytes.at(-1)) continue;
    if (bytes.length === 8 && bytes[2] === 0 && bytes[6] !== 0)
      throw new Error("a pedaleira recusou a leitura (o banco de sons dela pode estar inválido)");
    if (bytes.length !== length + 15 || bytes[2] !== 0x23 || bytes[6] !== object) continue;
    const reply = new DataView(bytes.buffer);
    if (reply.getUint32(7, true) === address && reply.getUint16(11, true) === length) return bytes.slice(14, -1);
  }
  throw new Error("a pedaleira não respondeu a uma leitura");
}

export async function readBank(link, object, start, length, chunk, onProgress = () => {}, tries = 3) {
  const out = new Uint8Array(length);
  for (let off = 0; off < length; off += chunk) {
    const n = Math.min(chunk, length - off);
    for (let t = 1; ; t++) {
      try { out.set(await readChunk(link, object, start + off, n), off); break; } catch (e) { if (t >= tries) throw e; }
    }
    onProgress((off + n) / length);
  }
  return out;
}

// a model's file: the 4-byte magic ("NAM", "AM4", "CAB"...) and the 14-byte name at bytes 4..17
export function describe(rec) {
  const ascii = (a) => String.fromCharCode(...a).replace(/\0.*$/s, "").replace(/[^\x20-\x7e]/g, "").trim();
  const empty = rec.every((b) => b === 0xff) || rec.every((b) => b === 0);
  if (empty) return { magic: "", name: "", empty: true };
  return { magic: ascii(rec.subarray(0, 4)), name: ascii(rec.subarray(4, 18)), empty: false };
}

export function slots(ampcab) {
  const amps = [], cabs = [];
  for (let i = 0; i < 20; i++) amps.push(describe(ampcab.subarray(i * 0x2000, (i + 1) * 0x2000)));
  for (let i = 0; i < 20; i++) cabs.push(describe(ampcab.subarray(0x28000 + i * 0x1000, 0x28000 + (i + 1) * 0x1000)));
  return { amps, cabs };
}

// ---- a store-only ZIP (no compression), so the backup is one file anyone can open ----
const CRC = (() => { const t = new Uint32Array(256); for (let n = 0; n < 256; n++) { let c = n; for (let k = 0; k < 8; k++) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1; t[n] = c >>> 0; } return t; })();
export function crc32(d) { let c = 0xffffffff; for (let i = 0; i < d.length; i++) c = CRC[(c ^ d[i]) & 0xff] ^ (c >>> 8); return (c ^ 0xffffffff) >>> 0; }
export function zip(files) {            // files: [{ name, data: Uint8Array }]
  const enc = new TextEncoder(), parts = [], central = [];
  let off = 0;
  for (const f of files) {
    const name = enc.encode(f.name), crc = crc32(f.data), n = f.data.length;
    const lh = new DataView(new ArrayBuffer(30));
    lh.setUint32(0, 0x04034b50, true); lh.setUint16(4, 20, true); lh.setUint16(6, 0x0800, true);   // UTF-8 names
    lh.setUint32(14, crc, true); lh.setUint32(18, n, true); lh.setUint32(22, n, true); lh.setUint16(26, name.length, true);
    parts.push(new Uint8Array(lh.buffer), name, f.data);
    const ch = new DataView(new ArrayBuffer(46));
    ch.setUint32(0, 0x02014b50, true); ch.setUint16(4, 20, true); ch.setUint16(6, 20, true); ch.setUint16(8, 0x0800, true);
    ch.setUint32(16, crc, true); ch.setUint32(20, n, true); ch.setUint32(24, n, true); ch.setUint16(28, name.length, true);
    ch.setUint32(42, off, true);
    central.push(new Uint8Array(ch.buffer), name);
    off += 30 + name.length + n;
  }
  const csize = central.reduce((s, a) => s + a.length, 0);
  const end = new DataView(new ArrayBuffer(22));
  end.setUint32(0, 0x06054b50, true); end.setUint16(8, files.length, true); end.setUint16(10, files.length, true);
  end.setUint32(12, csize, true); end.setUint32(16, off, true);
  const all = [...parts, ...central, new Uint8Array(end.buffer)];
  const out = new Uint8Array(all.reduce((s, a) => s + a.length, 0));
  let p = 0; for (const a of all) { out.set(a, p); p += a.length; }
  return out;
}

async function sha256hex(d) {
  const h = await crypto.subtle.digest("SHA-256", d);
  return [...new Uint8Array(h)].map((b) => b.toString(16).padStart(2, "0")).join("");
}
const safe = (s) => s.replace(/[^A-Za-z0-9 _.-]/g, "_").trim() || "sem_nome";

// the whole backup: the bank read twice (both reads must agree), the presets, a ZIP with everything
export async function backup(link, identity, onProgress = () => {}, log = () => {}) {
  log("lendo amps e IRs (1ª leitura)…");
  const a1 = await readBank(link, 5, 0x70000000, AMPCAB_LEN, 512, (p) => onProgress(p * 0.45));
  log("lendo amps e IRs de novo, pra conferir…");
  const a2 = await readBank(link, 5, 0x70000000, AMPCAB_LEN, 512, (p) => onProgress(0.45 + p * 0.45));
  if (a1.length !== a2.length || a1.some((b, i) => b !== a2[i]))
    throw new Error("as duas leituras do banco de amps/IRs deram diferentes: nada foi salvo, tente de novo");
  log("lendo os 80 presets…");
  const p1 = await readBank(link, 4, 0, PRESET_LEN, 92, (p) => onProgress(0.9 + p * 0.05));
  const p2 = await readBank(link, 4, 0, PRESET_LEN, 92, (p) => onProgress(0.95 + p * 0.05));
  if (p1.some((b, i) => b !== p2[i])) throw new Error("as duas leituras dos presets deram diferentes: tente de novo");
  const { amps, cabs } = slots(a1);
  const info = {
    what: "Backup da M-VAVE BlackBox (firmware oficial), feito pelo OpenPocket Bass: somente leitura",
    firmware: identity, date: new Date().toISOString(),
    ampcab: { file: "ampcab.bin", bytes: a1.length, sha256: await sha256hex(a1),
      layout: "20 amps x 8192 B (0x00000), 20 cabinets x 4096 B (0x28000): o arquivo pd40j da pedaleira, 0x00000..0x3BFFF" },
    presets: { file: "presets.bin", bytes: p1.length, sha256: await sha256hex(p1), layout: "80 presets x 92 B" },
    amps: amps.map((s, i) => ({ slot: i + 1, ...s })), cabs: cabs.map((s, i) => ({ slot: i + 1, ...s })),
  };
  const files = [
    { name: "LEIA-ME.txt", data: new TextEncoder().encode(
      "Backup da BlackBox feito antes de instalar o OpenPocket Bass.\r\n\r\n" +
      "ampcab.bin  = os 20 amps (capturas) e os 20 IRs, byte a byte, como estavam na pedaleira\r\n" +
      "presets.bin = os 80 presets\r\n" +
      "amps/ e irs/ = cada slot num arquivo separado (o formato interno da pedaleira: magic + nome + dados)\r\n" +
      "info.json   = firmware, data, nomes dos slots e SHA-256 de tudo\r\n\r\n" +
      "Guarde este arquivo. Ele é o que permite devolver os teus sons à pedaleira depois.\r\n") },
    { name: "info.json", data: new TextEncoder().encode(JSON.stringify(info, null, 2)) },
    { name: "ampcab.bin", data: a1 },
    { name: "presets.bin", data: p1 },
  ];
  amps.forEach((s, i) => { if (!s.empty) files.push({ name: `amps/${String(i + 1).padStart(2, "0")}_${safe(s.name)}_${safe(s.magic)}.bin`, data: a1.slice(i * 0x2000, (i + 1) * 0x2000) }); });
  cabs.forEach((s, i) => { if (!s.empty) files.push({ name: `irs/${String(i + 1).padStart(2, "0")}_${safe(s.name)}_${safe(s.magic)}.bin`, data: a1.slice(0x28000 + i * 0x1000, 0x28000 + (i + 1) * 0x1000) }); });
  return { info, zip: zip(files) };
}
