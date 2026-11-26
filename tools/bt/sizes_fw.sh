#!/bin/sh
# OpenPocket Bass, Bluetooth audio: what takes the flash in the audio image (firmware/build-bt): per object, then the largest
# functions and tables of the firmware's own objects. Run in the build container after OP_BT=1 sh firmware/build.sh.
TC="${JIELI_TOOLCHAIN:-/opt/jieli/toolchain}"; O=/src/firmware/build-bt; OD="$TC/pi32v2/bin/objdump"
echo "== objects (flash data bss)"; python3 /src/tools/bt/sizes.py "$OD" $O/*.o $O/jl/*.o | tail -45
for f in openpocket.o px_dsp.o px_math.o iis.o; do
  echo "== $f: the 40 largest symbols"
  $OD -t $O/$f | awk 'NF>=5 && $(NF-1) ~ /^[0-9a-f]+$/ {print strtonum("0x"$(NF-1)), $(NF-2), $NF}' | sort -rn | head -40
done
