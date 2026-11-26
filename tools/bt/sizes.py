# Sums llvm-objdump -h output per object: code+const (flash), data (flash+RAM), bss (RAM). Usage: sizes.py objdump obj...
import subprocess, sys, re
od, objs = sys.argv[1], sys.argv[2:]
rows = []
for o in objs:
    out = subprocess.run([od, "-h", o], capture_output=True, text=True).stdout
    t = d = b = 0
    for line in out.splitlines():
        m = re.match(r"\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+[0-9a-fA-F]+\s*(.*)", line)
        if not m: continue
        name, size, kind = m.group(1), int(m.group(2), 16), m.group(3)
        if size == 0 or name.startswith(".debug") or name in (".comment",) or name.startswith(".note") or name.startswith(".rel"): continue
        if "BSS" in kind or "bss" in name: b += size
        elif "DATA" in kind and "code" not in name and "const" not in name and "rodata" not in name and "text" not in name: d += size
        elif "TEXT" in kind or "DATA" in kind: t += size
    rows.append((t, d, b, o.split("/")[-1]))
rows.sort(reverse=True)
for r in rows[:25]: print("%8d %6d %6d  %s" % r)
T = [sum(r[i] for r in rows) for i in range(3)]
print("TOTAL flash(code+const) %d  data %d  bss %d  -> flash %d, RAM %d" % (T[0], T[1], T[2], T[0] + T[1], T[1] + T[2]))
