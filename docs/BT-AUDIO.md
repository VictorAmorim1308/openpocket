# Bluetooth audio test image (OpenPocket Bass)

> **Status: built, not yet run on a pedal.** The image links and packages in CI, and the music path (SBC decoder, the
> music ring, the mix) passes its PC tests. Nothing here has met a real radio or a real phone yet. This is a separate
> test image: the normal installer page never offers it, and the normal firmware does not change.

The pedal becomes a Bluetooth speaker. Play music from a phone through it and play bass over it, the way the M-VAVE
firmware does it. Only music playback is supported: no calls and no MIDI.

## Install, and go back

- **Install:** open the installer with `?channel=bt-test` added to its address
  (`https://victoramorim1308.github.io/openpocket/?channel=bt-test`). Then press **Install**, as for any OpenPocket
  update. Your presets, NAM slots and IRs stay where they are.
- **Go back:** open the installer without `?channel=…` and press **Install**.

The boot screen says `BT AUDIO TEST` under the version.

## Use

1. The pedal starts its radio a moment after the PLAY page appears. The status corner then scrolls `VISIBLE ...`.
2. On the phone, open Bluetooth and choose **OpenPocket Bass**. No PIN is asked.
3. Play music. The status corner scrolls `PLAYING ...`.

| Control | What it sets |
| --- | --- |
| The phone's volume | the music only (AVRCP absolute volume; the phone sends it to the pedal) |
| The preset's VOL (PLAY page) | the bass only |
| The MASTER knob | both |

There is no low cut on the music: it is mixed as it comes, in stereo, after the bass's own chain. The sum goes through
the same soft limit as the bass alone.

## The status line

`PLAYING L1 P2210 B1530K X0 V80 SBC 44 J53 R4600 U0`

| Part | Meaning |
| --- | --- |
| `VISIBLE` / `CONNECTED` / `PLAYING` | the radio is up and waiting / a phone is connected / music is arriving |
| `L` | phone connections since the start |
| `P`, `B` | A2DP packets and kilobytes of SBC received |
| `X` | packets with no SBC frame in them (should stay 0) |
| `V` | the phone's volume, 0 to 100 |
| `SBC 44 J53` | the stream: 44.1 kHz (or 48), joint stereo (`S` stereo, `D` dual, `M` mono), bitpool 53 |
| `R` | the music ring's level in frames. It hovers around 4608 (104 ms) |
| `U` | how many times the ring ran empty. The music fades out and restarts, so a rising `U` is a dropout |

The log channel (`node tools/op.mjs log`) prints the same line, plus the last host event and the stand-ins called.

## Please report

- whether the phone sees and pairs with **OpenPocket Bass**, and which phone it is;
- whether music plays, and whether `U` rises (dropouts) or the sound crackles;
- the status corner's DSP load (`DSP nn%`) with a NAM preset, with and without music playing;
- a photo of the screen if it shows `FAILED ...`.

## What this image does not do yet

- **Pairing is not remembered** across a power cycle. A phone that insists on its old bond may need to forget
  OpenPocket Bass and pair again.
- **No music level on the pedal yet.** The phone's volume sets it. A `BAIXO / MÚSICA` mixer on the PLAY page comes once
  the radio is proven.
- **The 12 factory guitar NAM models are left out of this image** to make room for the radio (88 KB). The bass presets
  use your own NAM slots, which work as usual. A preset set to a factory guitar model plays no amp sound here.
- Every pedal has the same Bluetooth address (`02:4F:50:42:41:53`). Two of them in one room get in each other's way.
- SBC only. A phone always offers SBC, so AAC is not needed.

## How it works

- **The stack:** JieLi's own classic Bluetooth controller and host from the AC79 SDK, at the commit the BLE test image
  pins (Apache-2.0, LLVM bitcode, unchanged). `third_party/jieli-bt/` holds the classic members (LMP, the BR/EDR
  baseband, L2CAP, SDP, AVDTP / A2DP sink, AVCTP / AVRCP, the pairing's ECDH). `SHARED` lists the members of
  `third_party/jieli-ble/` the image also takes. The LE parts, TWS, HFP, SPP, HID and the test modes are not built.
  `tools/bt/gen_stubs.py` writes their stand-ins (`firmware/bt/bt_stubs_gen.c`). Each stand-in counts its calls on the
  status line.
- **The rest is the BLE test image's.** That is the cooperative scheduler (`ble_os.c`), the radio start-up replayed
  from the stock firmware (`ble_rf.c`, with the stored radio records), and the start-up safety. After a failed boot,
  the next boot leaves the radio off.
- **The phone side** (`firmware/bt/bt_a2dp.c`): the host is set up like the SDK's `bt_music` example (Secure Simple
  Pairing with no input or output, A2DP sink, AVRCP as controller and target for the volume). The main loop takes the
  A2DP packets out of the stack, skips their RTP header and decodes their SBC frames.
- **The music path** (`firmware/bt/bt_music.h`, checked on the PC by `tests/test_bt_music.c`):
  - The SBC decoder follows the A2DP specification. Against ffmpeg's decoder it agrees to within rounding (79 dB).
  - The music ring holds 12288 stereo frames (279 ms). Playback starts when it holds 4608.
  - The audio interrupt reads the ring through a 4-point interpolator. Its step is the phone's rate over 44.1 kHz,
    corrected by the ring's level. This absorbs the two clocks' drift, as the SDK's own decoder does, and handles
    48 kHz streams.
  - In the test, a phone 300 ppm fast or slow, sending with 15 ms of jitter while the main loop stalls 60 ms every 2 s,
    never empties or overflows the ring.
  - The mix runs in `a_send`, on CPU0, after the bass's chain. It runs from RAM, like the rest of the audio path.
- **Memory** (CI build of this image):

  | What | Size |
  | --- | --- |
  | Image | 532 KB of the 581 KB application area |
  | Main RAM | 31 KB free |
  | Pool's free end | holds the music ring (48 KB), the stack's heap (40 KB) and the library's zeroed data. 164 bytes left |
  | RAM code window | 10.9 KB free |

- **Build:** `OP_BT=1 sh firmware/build.sh` (`firmware/openpocket-bt.bin`, `build/bt/`), or `node tools/op.mjs package-bt`
  for the installer's test channel (`docs/firmware/bt-test/`).
