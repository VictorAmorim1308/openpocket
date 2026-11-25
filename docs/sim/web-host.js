// SPDX-License-Identifier: GPL-3.0-only
// OpenPocket Bass: the simulated pedal inside the web page, no server and nothing to install (docs/sim/, built by
// tools/sim/web.mjs). tests/edsim.c, the pedal's own editor, preset store and screen code, runs as WebAssembly
// (edsim.wasm, -DEDSIM_WASM); this file answers the page's /api/ requests as tools/sim.mjs does, from it. The pedal's
// flash is kept in this browser (IndexedDB), so presets saved in the simulator are there the next time.
const enc = new TextEncoder(), dec = new TextDecoder();
let mem, X, out = "", waiters = new Map(), dirty = false;

// ---- the few WASI calls the C library makes (stdout to the line handler, the clock; no files) ----
const wasi = {
  fd_write(fd, iovs, n, nw) {
    const v = new DataView(mem.buffer);
    let t = 0;
    for (let i = 0; i < n; i++) {
      const p = v.getUint32(iovs + i * 8, true), l = v.getUint32(iovs + i * 8 + 4, true);
      if (fd === 1) out += dec.decode(new Uint8Array(mem.buffer, p, l));
      t += l;
    }
    v.setUint32(nw, t, true);
    if (fd === 1) lines();
    return 0;
  },
  clock_time_get(id, prec, at) { new DataView(mem.buffer).setBigUint64(at, BigInt(Math.round(performance.timeOrigin + performance.now())) * 1000000n, true); return 0; },
  fd_close: () => 0, fd_seek: () => 70, fd_read: () => 0, fd_fdstat_get: () => 0, fd_fdstat_set_flags: () => 0,
  fd_prestat_get: () => 8, fd_prestat_dir_name: () => 8, path_open: () => 44, path_remove_directory: () => 63,
  path_rename: () => 63, path_unlink_file: () => 63, proc_exit: (c) => { throw new Error(`exit ${c}`); },
  environ_sizes_get: (a, b) => { const v = new DataView(mem.buffer); v.setUint32(a, 0, true); v.setUint32(b, 0, true); return 0; },
  environ_get: () => 0, args_sizes_get: (a, b) => { const v = new DataView(mem.buffer); v.setUint32(a, 0, true); v.setUint32(b, 0, true); return 0; },
  args_get: () => 0, random_get: (p, n) => { crypto.getRandomValues(new Uint8Array(mem.buffer, p, n)); return 0; },
};
function lines() {
  let i;
  while ((i = out.indexOf("\n")) >= 0) {
    const ln = out.slice(0, i); out = out.slice(i + 1);
    if (!ln.startsWith("#") || ln.startsWith("#port ")) continue;
    const sp = ln.indexOf(" "), word = sp < 0 ? ln.slice(1) : ln.slice(1, sp);
    waiters.get(word)?.shift()?.(sp < 0 ? "" : ln.slice(sp + 1));
  }
}
function send(line) {
  const b = enc.encode(line);
  const at = X.ed_inbuf();
  new Uint8Array(mem.buffer, at, b.length + 1).set([...b, 0]);
  X.ed_line();
}
function ask(cmd, word = cmd.slice(1).split(" ")[0]) {
  let got = null;
  (waiters.get(word) || waiters.set(word, []).get(word)).push((v) => { got = v; });
  send(cmd);
  step();
  return got;
}
function step() { if (X.ed_step()) dirty = true; }

// ---- the flash, kept in this browser ----
const DB = "openpocket-bass-sim";
const db = () => new Promise((ok, no) => { const r = indexedDB.open(DB, 1); r.onupgradeneeded = () => r.result.createObjectStore("flash"); r.onsuccess = () => ok(r.result); r.onerror = () => no(r.error); });
async function loadFlash() {
  try {
    const d = await db();
    const v = await new Promise((ok) => { const r = d.transaction("flash").objectStore("flash").get("nor"); r.onsuccess = () => ok(r.result || null); r.onerror = () => ok(null); });
    d.close();
    return v && v.length === 0x100000 ? v : null;
  } catch (_) { return null; }
}
async function saveFlash() {
  try {
    const d = await db(), copy = new Uint8Array(mem.buffer, X.ed_nor(), 0x100000).slice();
    await new Promise((ok) => { const t = d.transaction("flash", "readwrite"); t.objectStore("flash").put(copy, "nor"); t.oncomplete = ok; t.onerror = ok; });
    d.close();
  } catch (_) { /* (a private window: the presets last until the page closes) */ }
}
export async function resetFlash() {
  try { const d = await db(); await new Promise((ok) => { const t = d.transaction("flash", "readwrite"); t.objectStore("flash").delete("nor"); t.oncomplete = ok; t.onerror = ok; }); d.close(); } catch (_) { }
  location.reload();
}

const { instance } = await WebAssembly.instantiateStreaming(fetch(new URL("./edsim.wasm", import.meta.url)), { wasi_snapshot_preview1: wasi });
X = instance.exports; mem = X.memory;
X._initialize();
const kept = await loadFlash();
if (kept) new Uint8Array(mem.buffer, X.ed_nor(), 0x100000).set(kept);
X.ed_init(kept ? 0 : 1);
setInterval(step, 5);                                  // the pedal's main loop: held buttons, the editor, the screen
setInterval(() => { if (dirty) { dirty = false; saveFlash(); } }, 1000);

// ---- the page's /api/ requests (tools/sim.mjs's), answered here ----
const BUTTONS = ["fs_up", "fs_dn", "fx", "home", "push", "mod", "amp", "cab", "dly", "rev"];
const CONTROLS = [...BUTTONS, "push_long", "dpush", "home_long", "fx_long", "feet"];
const json = (o, status = 200) => new Response(JSON.stringify(o), { status, headers: { "content-type": "application/json" } });
function okOrErr(r) { return r === "ok" ? json({ ok: true }) : json({ error: (r || "no answer").replace(/^err ?/, "") || "refused" }, 400); }
function screen() {
  const px = new Uint16Array(mem.buffer, X.ed_fb(), 240 * 240), be = new Uint8Array(240 * 240 * 2);
  for (let i = 0; i < px.length; i++) { be[i * 2] = px[i] >> 8; be[i * 2 + 1] = px[i] & 255; }
  return new Response(be, { headers: { "content-type": "application/octet-stream" } });
}
function api(url, method) {
  const [what, a, b] = url.pathname.slice(url.pathname.indexOf("/api/") + 5).split("/");
  const n = (s) => Math.trunc(Number(s));
  if (what === "info") return json({ kind: "pedal", port: "OpenPocket", flash: "browser", controls: CONTROLS, log: [] });
  if (what === "state") return json(JSON.parse(ask("#status")));
  if (what === "screen.rgb565") { step(); return screen(); }
  if (method !== "POST") return json({ error: "POST" }, 405);
  if (what === "press" && CONTROLS.includes(a)) return okOrErr(ask(`#press ${a}`));
  if ((what === "down" || what === "up") && BUTTONS.includes(a)) return okOrErr(ask(`#${what} ${a}`));
  if (what === "knob") return okOrErr(ask(`#knob ${n(a)}${url.searchParams.has("fast") ? " fast" : ""}`));
  if (what === "volume") return okOrErr(ask(`#vol ${n(a)}`));
  if (what === "battery") return okOrErr(ask(`#bat ${n(a)} ${b === undefined ? "" : n(b)}`, "bat"));
  if (what === "tuner") return okOrErr(ask(`#hz ${Number(a)}`));
  if (what === "load") return okOrErr(ask(`#dsp load ${n(a)}`, "dsp"));
  if (what === "bypass") return okOrErr(ask(`#dsp bypass ${n(a)}`, "dsp"));
  if (what === "reset") { resetFlash(); return json({ ok: true }); }
  return json({ error: "no such endpoint" }, 404);
}
const realFetch = globalThis.fetch.bind(globalThis);
globalThis.fetch = async (input, init = {}) => {
  const url = new URL(typeof input === "string" ? input : input.url, location.href);
  if (url.origin === location.origin && url.pathname.includes("/api/")) return api(url, (init.method || "GET").toUpperCase());
  return realFetch(input, init);
};
