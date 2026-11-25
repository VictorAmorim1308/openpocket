// SPDX-License-Identifier: GPL-3.0-only
// OpenPocket Bass: docs/sim/ (tools/sim/web.mjs), the simulated pedal as a page: it loads with no server behind it, shows
// the bass build's first preset, and its controls answer (PARA, a footswitch). Needs Playwright.
import { createRequire } from "node:module";
const { chromium } = createRequire(import.meta.url)("playwright");
import { spawn } from "node:child_process";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
const ROOT = join(dirname(fileURLToPath(import.meta.url)), "..");
const srv = spawn("python3", ["-m", "http.server", "8812", "--bind", "127.0.0.1"], { cwd: join(ROOT, "docs"), stdio: "ignore" });
await new Promise((r) => setTimeout(r, 800));
const b = await chromium.launch(); const p = await b.newPage({ viewport: { width: 1100, height: 1000 } });
const errs = []; p.on("pageerror", (e) => errs.push(e.message)); p.on("console", (m) => { if (m.type() === "error") errs.push(m.text()); });
await p.goto("http://127.0.0.1:8812/sim/index.html");
await p.waitForTimeout(2500);
const s1 = await p.evaluate(async () => (await (await fetch("/api/state")).json()));
await p.evaluate(async () => { await fetch("/api/knob/1", { method: "POST" }); await fetch("/api/press/fs_up", { method: "POST" }); });
await p.waitForTimeout(800);
const s2 = await p.evaluate(async () => (await (await fetch("/api/state")).json()));
await b.close(); srv.kill();
const ok = s1.preset.name === "DI LIMPO" && s2.slot === 1 && s2.preset.name === "AMP 1" && !errs.length;
console.log(ok ? "web simulator: ok (the bass build runs in the page, the controls answer)" : `web simulator: FAILED ${errs.join(" | ")}`);
if (!ok) process.exit(1);
