// SPDX-License-Identifier: GPL-3.0-only
// OpenPocket Bass: builds docs/sim/, the simulated pedal as a page of the site (nothing to install: open it in Chrome).
//   node tools/sim/web.mjs      (needs clang with the wasm32-wasi target and wasi-libc: apt install clang wasi-libc
//                                libclang-rt-dev-wasm32)
// tests/edsim.c (the pedal's own editor, preset store and screen, PX_BASS) becomes docs/sim/edsim.wasm; the front panel
// (tools/sim/pedal.html) and the sound (tools/sim/pedal-*.js, px_engine.wasm) are copied with their paths made relative,
// and tools/sim/web-host.js answers their /api/ requests in the page.
import { spawnSync } from "node:child_process";
import { cpSync, mkdirSync, readFileSync, readdirSync, writeFileSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), "..", "..");
const SIM = join(ROOT, "tools", "sim"), OUT = join(ROOT, "docs", "sim");
mkdirSync(join(OUT, "loops"), { recursive: true });

const cc = spawnSync(process.env.CC_WASM || "clang", ["--target=wasm32-wasi", "--sysroot=/usr", "-isystem", "/usr/include/wasm32-wasi",
  "-O2", "-mexec-model=reactor", "-DEDSIM_WASM", "-DPX_BASS=1", "-Wall", "-Wno-unused-function",
  "-o", join(OUT, "edsim.wasm"), "tests/edsim.c", "engine/px_dsp.c", "firmware/px_math.c", "-L/usr/lib/wasm32-wasi", "-lm"],
  { cwd: ROOT, stdio: "inherit" });
if (cc.error || cc.status !== 0) { console.error("edsim.wasm did not build (clang, wasi-libc)"); process.exit(1); }

const rel = (s) => s.replaceAll('"/sim/', '"./').replaceAll("'/sim/", "'./").replaceAll('"/loops/', '"./loops/');
for (const f of ["pedal-audio.js", "pedal-input.js", "pedal-worklet.js", "px-engine.js", "px_engine.wasm", "web-host.js"]) {
  const src = join(SIM, f);
  if (f.endsWith(".js")) writeFileSync(join(OUT, f), rel(readFileSync(src, "utf8")));
  else cpSync(src, join(OUT, f));
}
let page = rel(readFileSync(join(SIM, "pedal.html"), "utf8"));
page = page.replace(/<nav>[\s\S]*?<\/nav>/, '<nav><a href="../index.html">Instalar na pedaleira</a> <a href="../editor.html">Editor</a> ' +
  '<a href="#" id="simreset" title="Apaga os presets salvos neste simulador">Zerar o simulador</a></nav>');
page = page.replace('import { PedalAudio, dbfs } from "./pedal-audio.js";',
  'import { resetFlash } from "./web-host.js";\ndocument.getElementById("simreset").onclick = (e) => { e.preventDefault(); if (confirm("Apagar os presets salvos no simulador?")) resetFlash(); };\nimport { PedalAudio, dbfs } from "./pedal-audio.js";');
if (!page.includes("./web-host.js")) { console.error("tools/sim/pedal.html changed: the host import was not placed"); process.exit(1); }
writeFileSync(join(OUT, "index.html"), page);
for (const f of readdirSync(join(ROOT, "assets", "loops"))) if (/\.(wav|json|md)$/.test(f)) cpSync(join(ROOT, "assets", "loops", f), join(OUT, "loops", f));
console.log("docs/sim/ built");
