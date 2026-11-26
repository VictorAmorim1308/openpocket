#!/bin/sh
# OpenPocket Bass, Bluetooth audio: what JieLi's classic stack costs on the pedal. Run in the build container (CI: bt.yml)
# with the SDK's libraries in $SDK (cpu/wl82/liba). Compiles each member of tools/bt/members.txt (the objects an A2DP sink
# pulls in, tools/bt/closure.py) for pi32v2 at -Os, alone and linked as one module (LTO, if the toolchain has llvm-link),
# and prints their sizes. Nothing of the firmware is built.
set -e
TC="${JIELI_TOOLCHAIN:-/opt/jieli/toolchain}"; SDK="${SDK:-/src/build/sdk}"
echo "== toolchain"; ls "$TC/pi32v2/bin" "$TC/common/bin" | tr '\n' ' '; echo
W=/tmp/btp; rm -rf "$W"; mkdir -p "$W/x"
for a in btstack btctrler audio_server cpu; do mkdir -p "$W/x/$a"; (cd "$W/x/$a" && ar x "$SDK/cpu/wl82/liba/$a.a"); done
mv "$W/x/audio_server" "$W/x/as"
CC="$TC/pi32v2/bin/clang -target pi32v2"
: > "$W/objs"
while read -r m; do
  o="$W/$(echo "$m" | tr / _).o"
  if $CC -Os -c -x ir "$W/x/$m" -o "$o" 2>"$W/err"; then echo "$o" >> "$W/objs"; else echo "FAILED $m: $(head -c 300 "$W/err")"; fi
done < /src/tools/bt/members.txt
OD="$TC/pi32v2/bin/objdump"
echo "== objdump -h sample"; $OD -h "$(head -1 "$W/objs")" | head -20
echo "== per object (flash data bss): the 25 largest"
python3 /src/tools/bt/sizes.py "$OD" $(cat "$W/objs")
LINK="$TC/common/bin/llvm-link"
echo "== LTO: one module"
ins=""; while read -r m; do ins="$ins $W/x/$m"; done < /src/tools/bt/members.txt
if $LINK $ins -o "$W/all.bc" 2>"$W/lerr"; then $CC -Os -c -x ir "$W/all.bc" -o "$W/all.o" && python3 /src/tools/bt/sizes.py "$OD" "$W/all.o"; else head -c 3000 "$W/lerr"; fi
echo "== undefined in the module"; "$TC/pi32v2/bin/nm" -u "$W/all.o" 2>/dev/null | awk '{print $NF}' | sort -u | tr '\n' ' '; echo
