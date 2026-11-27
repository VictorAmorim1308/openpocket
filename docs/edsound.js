// SPDX-License-Identifier: GPL-3.0-only
// OpenPocket web editor: the current sound. Its name and what can be done with it, the signal chain (the order, what is
// on), and the block editor (type, settings). Every change plays at once on the pedal; a change made on the pedal shows here.

import { app, $, h, icon, on, emit, blk, typeOf, choices, act, menu, slotName } from "./edcore.js";
import { save, saveAs, revert, exportOne, setCompare, comparing, duplicate, moveTo, restoreFactory } from "./edpresets.js";
import { knobIndex, indexKnob, KINDS } from "./pxlib.js";
import { t3kLink } from "./pxt3k.js";

let dragging = null;

// ---- live editing (also what a learned MIDI control calls) ----
let pendingParam = null, sendingParam = false;
export function sendParam(b, k, v) {
  if (!app.ed || comparing()) return;
  app.st.preset.p[b][k] = v;
  pendingParam = { b, k, v };
  if (!sendingParam) flushParam();
}
async function flushParam() {
  sendingParam = true;
  try {
    for (;;) {
      while (pendingParam) {
        const q = pendingParam; pendingParam = null;
        await app.ed.setParam(q.b, q.k, q.v);
      }
      await sync();
      if (!pendingParam) break;                     // a value that came in meanwhile goes out too
    }
  } catch (_) { } finally { sendingParam = false; }
}
// the pedal's word on the sound after a change of ours (its "edited" flag): the other sections hear of it
async function sync() { app.st = await app.ed.state(); renderHeader(); emit("state", { own: true }); }

export function setFocus(b, tell = true) {
  app.focus = b; renderTiles(); renderPage();
  if (tell) act(() => app.ed.focus(b));
}
export function setOn(b, v) {
  if (comparing()) return undefined;
  return act(async () => { app.st.preset.on[b] = await app.ed.setOn(b, v); await sync(); renderTiles(); renderPage(); });
}
export const toggle = (b) => setOn(b, !app.st.preset.on[b]);
export function setType(b, t) {
  if (comparing()) return undefined;
  const n = blk(b).types.length;
  t = ((t % n) + n) % n;
  return act(async () => { app.st.preset.type[b] = await app.ed.setType(b, t); await sync(); renderTiles(); renderPage(); });
}
async function sendOrder(order) {
  await act(async () => { if (await app.ed.order(order) === 0) app.st.preset.order = order; await sync(); renderTiles(); renderPage(); });
}
function moveBlock(b, d) {
  if (comparing()) return;
  const o = [...app.st.preset.order], i = o.indexOf(b);
  let j = i + d;
  while (j >= 0 && j < o.length && blk(o[j]).used === false) j += d;   // (OpenPocket Bass: past the blocks not shown)
  if (j < 0 || j >= o.length) return;
  [o[i], o[j]] = [o[j], o[i]];
  sendOrder(o).then(() => document.querySelector(`.tile[data-b="${b}"] .tsel`)?.focus());
}

// ---- the header ----
function renderHeader() {
  const st = app.st, cmp = comparing();
  $("num").textContent = slotName(st.slot);
  if (document.activeElement !== $("name")) $("name").value = st.preset.name;
  $("name").disabled = cmp;
  const star = $("star");
  star.textContent = cmp ? "Hearing the saved preset" : st.dirty ? "Edited" : "";
  star.className = `state${cmp ? " cmp" : st.dirty ? " dirty" : ""}`;
  $("save").disabled = !st.dirty && !cmp;
  $("revert").disabled = !st.dirty && !cmp;
  $("compare").disabled = !st.dirty && !cmp;
  $("compare").setAttribute("aria-pressed", cmp ? "true" : "false");
  $("compare").textContent = cmp ? "Back to my changes" : "Compare";
  document.title = `${st.dirty ? "• " : ""}${slotName(st.slot)} ${st.preset.name} · OpenPocket Editor`;
  $("view-sound").classList.toggle("comparing", cmp);
  for (const id of ["tiles", "block"]) $(id).inert = cmp;
  $("hint").textContent = cmp ? "This is the preset as it is saved. Press “Back to my changes” to go on editing."
    : matchMedia("(pointer: coarse)").matches ? "Hold a block, then drag it to move it. The arrows in the panel below move it too."
    : "Drag a block to move it. Arrow keys choose a block, Space switches it, Alt + arrows move it.";
}

// ---- the chain ----
function renderTiles() {
  const box = $("tiles"), p = app.st.preset, had = box.contains(document.activeElement);
  box.textContent = "";
  const shown = p.order.filter((b) => blk(b).used !== false);   // OpenPocket Bass: the blocks taken out are not shown
  shown.forEach((b, i) => {
    const B = blk(b), is = !!p.on[b], sel = b === app.focus;
    const tsel = h("button", { type: "button", class: "tsel", tabIndex: sel ? 0 : -1, "aria-pressed": sel ? "true" : "false",
      "aria-label": `${B.name}, ${typeOf(p, b).name}, ${is ? "on" : "off"}, ${i + 1} of ${shown.length}`, "aria-keyshortcuts": "Space Alt+ArrowLeft Alt+ArrowRight" },
      h("span", { class: "tl", text: B.label }), h("span", { class: "tt", text: typeOf(p, b).name }));
    const pw = h("button", { type: "button", class: "pw", tabIndex: -1, role: "switch", "aria-checked": is ? "true" : "false", "aria-label": `${B.name} ${is ? "on" : "off"}` },
      h("i"), h("span", { text: is ? "ON" : "OFF" }));
    const t = h("div", { class: `tile${is ? " on" : ""}${sel ? " sel" : ""}${dragging && dragging.b === b ? " drag" : ""}`, "data-b": b, style: `--c:${B.color}` }, tsel, pw, h("i", { class: "share" }));
    pw.onclick = () => toggle(b);
    tsel.onpointerdown = (e) => startDrag(e, t, b);
    tsel.onkeydown = (e) => {
      const d = e.key === "ArrowLeft" ? -1 : e.key === "ArrowRight" ? 1 : 0;
      if (d && e.altKey) { e.preventDefault(); moveBlock(b, d); }
      else if (d) { e.preventDefault(); const n = shown[i + d]; if (n !== undefined) { setFocus(n); document.querySelector(`.tile[data-b="${n}"] .tsel`)?.focus(); } }
      else if (e.key === " ") { e.preventDefault(); toggle(b); }
      else if (e.key === "Enter") { e.preventDefault(); $("type").focus(); }
    };
    tsel.onclick = (e) => { if (e.detail === 0) setFocus(b); };      // (a pointer's click is the end of startDrag)
    box.append(t);
  });
  if (had) box.querySelector(".tile.sel .tsel")?.focus();
  renderShares();
}
function renderShares() {
  const prof = app.ping && app.ping.prof;
  for (const t of $("tiles").children) {
    const v = prof ? prof[+t.dataset.b] || 0 : 0;
    t.querySelector(".share").style.width = `${Math.min(100, v * 2)}%`;
    t.title = prof && v ? `${blk(+t.dataset.b).name}: ${v} % of the processor` : "";
  }
}
function startDrag(e, el, b) {
  if (e.button !== 0) return;
  const x0 = e.clientX, y0 = e.clientY, touch = e.pointerType === "touch";
  let ghost = null, order = null, armed = !touch;
  const lift = (ev) => {
    dragging = { b }; order = [...app.st.preset.order];
    ghost = el.cloneNode(true); ghost.classList.add("ghost"); ghost.style.width = `${el.getBoundingClientRect().width}px`;
    document.body.append(ghost); renderTiles(); place(ev);
  };
  const place = (ev) => { ghost.style.left = `${ev.clientX}px`; ghost.style.top = `${ev.clientY}px`; };
  // a finger holds still first, so that a swipe over the chain still scrolls the page
  const hold = touch ? setTimeout(() => { armed = true; lift(e); }, 280) : 0;
  const move = (ev) => {
    const far = Math.hypot(ev.clientX - x0, ev.clientY - y0) >= 6;
    if (!ghost) {
      if (!armed) { if (far) end(); return; }
      if (!far) return;
      lift(ev);
    }
    place(ev);
    // the tile under the pointer: the dragged block takes its place
    let best = -1, bd = 1e9;
    [...$("tiles").children].forEach((t) => { const r = t.getBoundingClientRect(); const d = Math.hypot(ev.clientX - (r.left + r.width / 2), ev.clientY - (r.top + r.height / 2)); if (d < bd) { bd = d; best = order.indexOf(Number(t.dataset.b)); } });
    const cur = order.indexOf(b);
    if (best >= 0 && best !== cur) { order.splice(cur, 1); order.splice(best, 0, b); app.st.preset.order = [...order]; renderTiles(); }
  };
  const end = () => {
    clearTimeout(hold);
    removeEventListener("pointermove", move); removeEventListener("pointerup", up); removeEventListener("pointercancel", up);
  };
  const up = () => {
    end();
    if (ghost) { ghost.remove(); dragging = null; renderTiles(); sendOrder(order); }
    else { setFocus(b); document.querySelector(`.tile[data-b="${b}"] .tsel`)?.focus({ preventScroll: true }); }
  };
  addEventListener("pointermove", move); addEventListener("pointerup", up); addEventListener("pointercancel", up);
}

// ---- the block editor ----
// the options of a setting that is a choice; the MODEL and IR settings count the pedal's slots (two can share a name)
function paramChoices(b, t, k, param) {
  for (const kind of ["nam", "ir"]) {
    const ref = app.refs[kind];
    if (ref && b === ref.b && t === ref.t && k === 0 && param.texts) {
      return Array.from({ length: ref.n }, (_, i) => {
        const value = indexKnob(i, ref.n), slot = i - ref.base, list = kind === "nam" ? app.nam.models : app.irs;
        const it = kind === "nam" ? list[i] : list[slot];
        return { label: it ? it.name : param.texts[value], value, index: i, slot, kind, empty: slot >= 0 && it && !it.used, tag: slot >= 0 ? KINDS[kind].tag(slot) : "" };
      });
    }
  }
  return choices(param);
}
const optionOf = (opts, v) => opts.find((o) => o.index !== undefined ? knobIndex(v, opts.length) === o.index : v >= o.lo && v <= o.hi) || opts[0];

function renderPage(fromPush) {
  const b = app.focus, B = blk(b), p = app.st.preset, T = typeOf(p, b), is = !!p.on[b];
  $("block").style.setProperty("--c", B.color);
  $("block").classList.toggle("off", !is);
  $("bname").textContent = B.name;
  const pos = p.order.indexOf(b), shown = p.order.filter((x) => blk(x).used !== false);   // (OpenPocket Bass: the blocks in the chain)
  $("bpos").textContent = `${shown.indexOf(b) + 1} of ${shown.length} in the chain`;
  const sw = $("onoff");
  sw.setAttribute("aria-checked", is ? "true" : "false");
  sw.setAttribute("aria-label", `${B.name} ${is ? "on" : "off"}`);
  sw.querySelector("span").textContent = is ? "On" : "Off";
  $("mvl").disabled = shown.indexOf(b) <= 0; $("mvr").disabled = shown.indexOf(b) >= shown.length - 1;
  void pos;
  const sel = $("type");
  if (sel.dataset.b !== String(b) || sel.options.length !== B.types.length) {
    sel.textContent = "";
    B.types.forEach((t, i) => sel.append(h("option", { value: i, text: t.name })));
    sel.dataset.b = b;
  }
  sel.value = p.type[b];
  sel.setAttribute("aria-label", `${B.name} type`);
  const one = B.types.length < 2 || B.fixed >= 0;   // (OpenPocket Bass: AMP is NAM, CAB is USER IR)
  sel.disabled = one; $("tprev").disabled = one; $("tnext").disabled = one;
  $("block").querySelector(".type").hidden = B.fixed >= 0;   // (a block of one type has no type row, as on the pedal)

  const box = $("params");
  const names = (T.params[0] && T.params[0].texts ? T.params[0].texts.join("|") : "");     // (the slots' names change)
  const sig = `${b}:${p.type[b]}:${names}`;
  if (box.dataset.sig !== sig) {
    box.textContent = "";
    T.params.forEach((param, k) => {
      const id = `p${k}`, opts = paramChoices(b, p.type[b], k, param);
      let ctl;
      if (opts && opts.length <= 4) {
        ctl = h("div", { class: "seg", id, role: "radiogroup", "aria-label": param.name }, opts.map((o) =>
          h("button", { type: "button", role: "radio", "data-v": o.value, text: o.label, onclick: () => { sendParam(b, k, o.value); showVal(k); } })));
      } else if (opts) {
        ctl = h("select", { id, class: "choice", onchange: () => { sendParam(b, k, +ctl.value); showVal(k); renderNote(); } });
        const groups = opts[0].index === undefined ? null : [["Factory", opts.filter((o) => o.slot < 0 && app.nam.models[o.index] && app.nam.models[o.index].used)],
          [opts[0].kind === "nam" ? "Your captures" : "Your IRs", opts.filter((o) => o.slot >= 0)]];   // (OpenPocket Bass: no factory models)
        const opt = (o) => h("option", { value: o.value, text: o.tag ? `${o.tag}  ${o.empty ? "empty" : o.label}` : o.label, disabled: !!o.empty });
        if (groups) for (const [name, list] of groups) { if (list.length) ctl.append(h("optgroup", { label: name }, list.map(opt))); }
        else ctl.append(...opts.map(opt));
      } else {
        ctl = h("input", { type: "range", id, min: 0, max: 100, step: 1 });
        ctl.oninput = () => { showVal(k); sendParam(b, k, +ctl.value); renderGraph(); };
        ctl.onpointerdown = () => { ctl.dataset.held = "1"; };
        ctl.onpointerup = ctl.onpointercancel = () => { delete ctl.dataset.held; };
      }
      const step = (d) => h("button", { type: "button", class: "nudge", tabIndex: -1, "aria-label": `${param.name} ${d < 0 ? "down" : "up"} one step`,
        onclick: () => { const v = Math.max(0, Math.min(100, +ctl.value + d)); ctl.value = v; showVal(k); sendParam(b, k, v); renderGraph(); } }, icon(d < 0 ? "minus" : "plus"));
      const range = ctl.type === "range";
      box.append(h("div", { class: `prm${range ? "" : " one"}` },
        h("label", { for: range || ctl.tagName === "SELECT" ? id : null, text: param.name }),
        range && step(-1), ctl, range && step(1),
        range && h("output", { class: "val", for: id })));
    });
    if (!T.params.length) box.append(h("p", { class: "dim", text: "This type has no settings." }));
    if (ampLevel(b)) {   // OpenPocket Bass: the AMP's LEVEL, kept in the SFX block (not in the chain)
      AMP_X.forEach(([name, k]) => {
        const ctl = h("input", { type: "range", id: `pampx${k}`, min: 0, max: 100, step: 1 });
        ctl.oninput = () => { showAmpX(k); sendParam(SFX, k, +ctl.value); };
        const step = (d) => h("button", { type: "button", class: "nudge", tabIndex: -1, "aria-label": `${name} ${d < 0 ? "down" : "up"} one step`,
          onclick: () => { const v = Math.max(0, Math.min(100, +ctl.value + d)); ctl.value = v; showAmpX(k); sendParam(SFX, k, v); } }, icon(d < 0 ? "minus" : "plus"));
        box.append(h("div", { class: "prm" }, h("label", { for: `pampx${k}`, text: name }), step(-1), ctl, step(1), h("output", { class: "val", for: `pampx${k}` })));
      });
    }
    box.dataset.sig = sig;
  }
  if (ampLevel(b)) for (const [, k] of AMP_X) { const c = $(`pampx${k}`); if (document.activeElement !== c) c.value = p.p[SFX][k]; showAmpX(k); }
  T.params.forEach((_, k) => {
    const c = $(`p${k}`), v = p.p[b][k];
    if (c.type === "range") { if (!(fromPush && (c.dataset.held || (document.activeElement === c && pendingParam)))) c.value = v; }
    else if (c.tagName === "SELECT") { const o = optionOf(paramChoices(b, p.type[b], k, T.params[k]), v); c.value = o.value; }
    showVal(k);
  });
  renderGraph();
  renderNote();
}
const SFX = 3;                                        // (engine/px_dsp.h PX_AMP_LVL_B: PX_B_SFX)
const ampLevel = (b) => blk(b).label === "AMP" && blk(b).fixed >= 0;
// (LEVEL +-12 dB; the AMP's tone bands are fixed now: LOW 100 Hz, MID 600 Hz, HIGH 3 kHz, engine/px_dsp.h)
const AMP_X = [["LEVEL", 0]];
function showAmpX(k) {
  const c = $(`pampx${k}`); if (!c) return;
  const v = +c.value, db = (v - 50) * 24 / 100;
  c.style.setProperty("--p", `${v}%`);
  c.parentElement.querySelector(".val").textContent = `${db > 0 ? "+" : ""}${db.toFixed(1)} dB`;
}
// ---- OpenPocket Bass: the EQ's curve, as the pedal draws it (engine/px_dsp.c eq_design; 30 Hz..8 kHz, +-12 dB) ----
const EQFS = 44100;
function bqDesign(kind, f, q, db) {
  const A = 10 ** (db / 40), w = 2 * Math.PI * f / EQFS, c = Math.cos(w), s = Math.sin(w);
  if (kind === "peak") { const al = s / (2 * q); return [1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A]; }
  const al = s / 2 * Math.SQRT2, ap = A + 1, am = A - 1, tsa = 2 * Math.sqrt(A) * al;
  if (kind === "low") return [A * (ap - am * c + tsa), 2 * A * (am - ap * c), A * (ap - am * c - tsa), ap + am * c + tsa, -2 * (am + ap * c), ap + am * c - tsa];
  return [A * (ap + am * c + tsa), -2 * A * (am + ap * c), A * (ap + am * c - tsa), ap - am * c + tsa, 2 * (am - ap * c), ap - am * c - tsa];
}
function bqDb([b0, b1, b2, a0, a1, a2], f) {
  const w = 2 * Math.PI * f / EQFS, c1 = Math.cos(w), s1 = Math.sin(w), c2 = Math.cos(2 * w), s2 = Math.sin(2 * w);
  const nr = b0 + b1 * c1 + b2 * c2, ni = -(b1 * s1 + b2 * s2), dr = a0 + a1 * c1 + a2 * c2, di = -(a1 * s1 + a2 * s2);
  return 10 * Math.log10((nr * nr + ni * ni) / (dr * dr + di * di));
}
// the EQ type's filters and its points [{ f, q, db, kind }], from the type's name (the pedal's catalogue) and its five values
function eqBands(name, v) {
  const d = (k) => (v[k] - 50) * 24 / 100, x = (k) => v[k] / 100;
  if (name === "5 BAND") return [["low", 100, 1, d(0)], ["peak", 400, 1, d(1)], ["peak", 1000, 1, d(2)], ["peak", 2500, 1, d(3)], ["high", 6000, 1, d(4)]];
  if (name === "GRAPHIC 5") return [80, 240, 750, 2200, 6600].map((f, k) => ["peak", f, 1.4, d(k)]);
  if (name === "BASS PEQ") return [["low", 80, 1, d(0)], ["peak", 40 * 100 ** x(1), 0.4 + x(3) * 5.6, d(2)], ["high", 3500, 1, d(4)]];
  if (name === "PARAMETRIC") return [["low", 100, 1, d(0)], ["peak", 150 * 33 ** x(1), 0.4 + x(3) * 5.6, d(2)], ["high", 5000, 1, d(4)]];
  return null;
}
function renderGraph() {
  const box = $("eqgraph"), b = app.focus, p = app.st.preset, B = blk(b);
  const bands = B.label === "EQ" ? eqBands(typeOf(p, b).name, p.p[b]) : null;
  box.hidden = !bands;
  if (!bands) return;
  const W = 640, H = 170, x0 = 34, x1 = W - 10, yc = H / 2, k = 6, lx = (f) => x0 + (x1 - x0) * Math.log(f / 30) / Math.log(8000 / 30);
  const filt = bands.map(([kind, f, q, db]) => bqDesign(kind, f, q, db));
  const pts = [];
  for (let i = 0; i <= 160; i++) {
    const f = 30 * (8000 / 30) ** (i / 160);
    const db = Math.max(-13.5, Math.min(13.5, filt.reduce((a, c) => a + bqDb(c, f), 0)));
    pts.push(`${lx(f).toFixed(1)},${(yc - db * k).toFixed(1)}`);
  }
  const NS = "http://www.w3.org/2000/svg", el = (t, a) => { const e = document.createElementNS(NS, t); for (const [n, v] of Object.entries(a)) e.setAttribute(n, v); return e; };
  const svg = el("svg", { viewBox: `0 0 ${W} ${H}`, class: "eqsvg", role: "img", "aria-label": "The EQ's curve" });
  for (const db of [-12, -6, 0, 6, 12]) {
    svg.append(el("line", { x1: x0, x2: x1, y1: yc - db * k, y2: yc - db * k, class: db ? "g" : "z" }));
    const t = el("text", { x: 4, y: yc - db * k + 4, class: "t" }); t.textContent = `${db > 0 ? "+" : ""}${db}`; svg.append(t);
  }
  for (const [f, lb] of [[40, "40"], [100, "100"], [250, "250"], [1000, "1K"], [2500, "2.5K"], [6000, "6K"]]) {
    svg.append(el("line", { x1: lx(f), x2: lx(f), y1: 6, y2: H - 6, class: "g" }));
    const t = el("text", { x: lx(f) + 3, y: H - 8, class: "t" }); t.textContent = lb; svg.append(t);
  }
  svg.append(el("polyline", { points: pts.join(" "), class: "c" }));
  for (const [kind, f, , db] of bands) {
    const y = yc - Math.max(-12, Math.min(12, filt.reduce((a, c) => a + bqDb(c, f), 0))) * k;
    svg.append(el("circle", { cx: lx(f), cy: y, r: 5, class: kind === "peak" ? "pk" : "sh" }));
    void db;
  }
  box.textContent = ""; box.append(svg);
}
function showVal(k) {
  const c = $(`p${k}`), b = app.focus, p = app.st.preset, param = typeOf(p, b).params[k];
  if (!c || !param) return;
  if (c.type === "range") {
    const v = +c.value, t = param.texts && param.texts[v];
    const out = c.parentElement.querySelector(".val");
    out.textContent = t || String(v);
    c.style.setProperty("--p", `${v}%`);
    c.setAttribute("aria-valuetext", t || String(v));
  } else if (c.classList.contains("seg")) {
    const cur = optionOf(choices(param), p.p[b][k]);
    for (const r of c.children) { const is = +r.dataset.v === cur.value; r.setAttribute("aria-checked", is ? "true" : "false"); r.tabIndex = is ? 0 : -1; }
  }
}
// under the settings of a NAM amp or a user IR: where its file comes from, and the way to the Models section
function renderNote() {
  const box = $("blocknote"), b = app.focus, p = app.st.preset;
  box.textContent = "";
  const kind = ["nam", "ir"].find((k) => app.refs[k] && app.refs[k].b === b && app.refs[k].t === p.type[b]);
  box.hidden = !kind;
  if (!kind) return;
  const ref = app.refs[kind], K = KINDS[kind], slot = knobIndex(p.p[b][0], ref.n) - ref.base;
  const list = kind === "nam" ? app.nam.models.filter((m) => m.user) : app.irs;
  const e = slot >= 0 ? app.held[kind].get(slot) : null, src = e && e.source;
  if (slot >= 0 && list[slot] && !list[slot].used) {
    box.append(h("p", { class: "warn" }, icon("alert"), ` ${K.tag(slot)} is empty: this preset plays without ${K.without}. Choose another ${K.thing} above, or load one into ${K.tag(slot)}.`));
  } else if (src && src.from === "tone3000") {
    box.append(h("p", { class: "src" }, h("span", { class: "t3k-mark", text: "T3K" }), " ",
      t3kLink(src.url) ? h("a", { href: t3kLink(src.url), target: "_blank", rel: "noopener", text: src.title }) : src.title, src.creator ? ` by ${src.creator}` : "", src.gear ? ` · ${src.gear.toUpperCase()} · NAM` : ""));
  } else if (src && src.from === "file") {
    box.append(h("p", { class: "src dim", text: `From the file ${src.file}.` }));
  }
  box.append(h("button", { type: "button", class: "btn small", onclick: () => emit("view-go", "models"),
    text: kind === "nam" ? "Get or manage captures" : "Load or manage IRs" }));
}

export function initSound() {
  on("state", (o) => { if (!app.st) return; renderHeader(); if (!o.own) { renderTiles(); renderPage(o.push); } });
  on("compare", () => { renderHeader(); renderTiles(); renderPage(); });
  on("assets", () => { if (app.st && app.conn === "ready") { $("params").dataset.sig = ""; renderPage(); } });
  on("ping", renderShares);
  $("onoff").onclick = () => toggle(app.focus);
  $("type").onchange = () => setType(app.focus, +$("type").value);
  $("tprev").onclick = () => setType(app.focus, app.st.preset.type[app.focus] - 1);
  $("tnext").onclick = () => setType(app.focus, app.st.preset.type[app.focus] + 1);
  $("mvl").onclick = () => moveBlock(app.focus, -1);
  $("mvr").onclick = () => moveBlock(app.focus, 1);
  $("name").onchange = () => act(async () => { app.st.preset.name = await app.ed.name($("name").value); $("name").value = app.st.preset.name; await sync(); });
  $("name").onkeydown = (e) => { if (e.key === "Enter") $("name").blur(); else if (e.key === "Escape") { $("name").value = app.st.preset.name; $("name").blur(); } };
  $("save").onclick = () => save();
  $("saveas").onclick = () => saveAs();
  $("revert").onclick = () => revert();
  $("compare").onclick = () => setCompare(!comparing());
  $("pmore").onclick = (e) => {
    const s = app.st.slot, narrow = matchMedia("(max-width: 899px)").matches, cmp = comparing();
    menu(e.currentTarget, [
      narrow && { label: cmp ? "Back to my changes" : "Compare with the saved preset", run: () => setCompare(!cmp), disabled: !app.st.dirty && !cmp, why: "Nothing was changed." },
      narrow && { label: "Revert", run: () => revert(), disabled: !app.st.dirty && !cmp, why: "Nothing was changed." },
      narrow && "-",
      { label: "Copy to…", run: () => duplicate(s) },
      { label: "Move to…", run: () => moveTo(s) },
      { label: "Export", run: () => exportOne(s) },
      "-",
      { label: "Back to factory", run: () => restoreFactory(s), danger: true, disabled: !app.slots[s].saved, why: "This is already the factory preset." },
    ]);
  };
  // a block lifted with a finger: the page stays still under it
  $("tiles").addEventListener("touchmove", (e) => { if (dragging) e.preventDefault(); }, { passive: false });
  // arrows on a segmented choice
  $("params").addEventListener("keydown", (e) => {
    const seg = e.target.closest(".seg");
    if (!seg || !["ArrowLeft", "ArrowRight", "ArrowUp", "ArrowDown"].includes(e.key)) return;
    e.preventDefault();
    const r = [...seg.children], i = r.indexOf(e.target), n = r[(i + (e.key === "ArrowLeft" || e.key === "ArrowUp" ? -1 : 1) + r.length) % r.length];
    n.click(); n.focus();
  });
}
