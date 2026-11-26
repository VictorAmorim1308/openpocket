#!/bin/sh
# OpenPocket Bass, Bluetooth audio: what takes the flash in the audio image (firmware/build-bt): per object, then the largest
# functions and tables of the firmware's own objects. Run in the build container after OP_BT=1 sh firmware/build.sh.
TC="${JIELI_TOOLCHAIN:-/opt/jieli/toolchain}"; O=/src/firmware/build-bt; OD="$TC/pi32v2/bin/objdump"
echo "== objects (flash data bss)"; python3 /src/tools/bt/sizes.py "$OD" $O/*.o $O/jl/*.o | head -40
for f in openpocket.o px_dsp.o px_math.o iis.o; do
  echo "== $f: the 40 largest symbols"
  $OD -t $O/$f | python3 -c "
import sys
r=[]
for l in sys.stdin:
    p=l.split()
    if len(p)>=5:
        try: r.append((int(p[-2],16),p[-3],p[-1]))
        except ValueError: pass
for x in sorted(r,reverse=True)[:40]: print(*x)"
done
