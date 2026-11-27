// SPDX-License-Identifier: GPL-3.0-only
// OpenPocket Bass: a rotary knob over a setting's range input. The input stays the control (its value, its keys, its
// label, what assistive technology reads); the knob draws it and turns it: drag up or down (Shift: fine), the wheel,
// a double click back to the middle for a setting that goes both ways (LOW, MID, HIGH, LEVEL).
const NS = "http://www.w3.org/2000/svg";
const A0 = -135, A1 = 135, R = 21, C = 28;
const pt = (a) => { const r = (a - 90) * Math.PI / 180; return [C + R * Math.cos(r), C + R * Math.sin(r)]; };
const arc = (a, b) => {
  if (Math.abs(b - a) < 0.5) return "";
  const [x0, y0] = pt(Math.min(a, b)), [x1, y1] = pt(Math.max(a, b));
  return `M${x0.toFixed(2)} ${y0.toFixed(2)}A${R} ${R} 0 ${Math.abs(b - a) > 180 ? 1 : 0} 1 ${x1.toFixed(2)} ${y1.toFixed(2)}`;
};
const el = (t, a) => { const e = document.createElementNS(NS, t); for (const [n, v] of Object.entries(a)) e.setAttribute(n, v); return e; };

export function knob(input, { bipolar = false } = {}) {
  const wrap = document.createElement("div");
  wrap.className = `knob${bipolar ? " bi" : ""}`;
  const svg = el("svg", { viewBox: "0 0 56 56", "aria-hidden": "true" });
  const track = el("path", { class: "k-track", d: arc(A0, A1) });
  const val = el("path", { class: "k-val" });
  const cap = el("circle", { class: "k-cap", cx: C, cy: C, r: 15 });
  const tick = el("line", { class: "k-tick", x1: C, y1: C - 6, x2: C, y2: C - 13 });
  svg.append(track, val, cap, tick);
  input.classList.add("knob-in");
  input.parentNode && input.parentNode.replaceChild(wrap, input);
  wrap.append(svg, input);
  const min = () => +input.min || 0, max = () => (input.max === "" ? 100 : +input.max);
  const draw = () => {
    const f = (+input.value - min()) / (max() - min() || 1), a = A0 + (A1 - A0) * f;
    val.setAttribute("d", bipolar ? arc(0, a) : arc(A0, a));
    tick.setAttribute("transform", `rotate(${a.toFixed(1)} ${C} ${C})`);
  };
  const set = (v) => {
    v = Math.max(min(), Math.min(max(), Math.round(v)));
    if (v === +input.value) return;
    input.value = v; draw();
    input.dispatchEvent(new Event("input", { bubbles: true }));
  };
  input.addEventListener("input", draw);
  input._knob = draw;
  let drag = null;
  wrap.addEventListener("pointerdown", (e) => {
    if (e.button !== 0) return;
    e.preventDefault();
    input.focus({ preventScroll: true });
    drag = { y: e.clientY, v: +input.value, id: e.pointerId };
    wrap.setPointerCapture(e.pointerId);
    input.dataset.held = "1";
    wrap.classList.add("turning");
  });
  wrap.addEventListener("pointermove", (e) => {
    if (!drag || e.pointerId !== drag.id) return;
    const px = e.shiftKey ? 600 : 160;                         // (the whole range in 160 px; with Shift, 600)
    set(drag.v + (drag.y - e.clientY) * (max() - min()) / px);
  });
  const up = (e) => { if (!drag || e.pointerId !== drag.id) return; drag = null; delete input.dataset.held; wrap.classList.remove("turning"); };
  wrap.addEventListener("pointerup", up);
  wrap.addEventListener("pointercancel", up);
  wrap.addEventListener("wheel", (e) => { e.preventDefault(); set(+input.value + (e.deltaY < 0 ? 1 : -1) * (e.shiftKey ? 5 : 1)); }, { passive: false });
  wrap.addEventListener("dblclick", () => { if (bipolar) set((min() + max()) / 2); });
  draw();
  return wrap;
}
// is a setting one that goes both ways from the middle (its value at 50 reads 0, or 0.0 dB)?
export const isBipolar = (param) => !!(param && param.texts && /^[+-]?0(\.0)?( ?dB)?$/i.test(String(param.texts[50] || "").trim()))
  || /^(LOW|MID|HIGH|BASS|TREBLE|LEVEL|PRESET LVL)\b/.test(param && param.name || "") && !/FREQ|CUT|Q$/.test(param.name);
