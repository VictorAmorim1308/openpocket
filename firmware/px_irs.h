/* SPDX-License-Identifier: GPL-3.0-only
 * OpenPocket — the user cabinet IRs in flash (v38).
 *
 * (OpenPocket Bass: 20 slots, ir_addr below; first) 8 slots of 4 KiB at 0xF4000..0xFC000 (above the preset store; the update loader leaves this range alone:
 * loader/ldr_core.c LDR_IR_LO/HI). A slot: a 64-byte header, then PX_IR_TAPS (2000; v1: 1024) 16-bit samples at 44.1 kHz:
 *   magic "OPIR", version 2, 2000, name (11 chars + 0), scale (float: the samples' full scale), CRC16 of the samples.
 * The web editor (docs/pxir.js) reads the user's WAV file, resamples it to 44.1 kHz, keeps its first 1024 samples (a
 * short fade at the end), and sends it in pieces (IR_WRITE) into the staging buffer, which is the NAM models' (one
 * upload at a time); IR_COMMIT checks it (header, CRC) and writes the slot with the audio held, as for a NAM model.
 * The names are read once at boot into RAM; the samples are read from the flash only when a preset picks the IR (a full
 * change: px_engine_set with the audio held, as the NAM weights). The engine sets the level (px_dsp.c ir_load_h).
 */
#pragma once

#define IR_BASE   0xF4000u
#define IR_SLOT   0x1000u
/* OpenPocket Bass: 20 slots. 0..7 as before at 0xF4000..0xFC000 (kept through the update); 8..18 at 0xE5000..0xF0000
 * (above the update loader's staging, below the preset store: unused by OpenPocket); 19 at 0xFC000. Each range is
 * user data for the update loader (loader/ldr_core.c LDR_IR2, LDR_IR3) and writable by the app (px_usb.h). Bytes
 * 0xF00..0xF07 of each sector stay erased (no update record): the samples skip them (IR_GAP below). */
#define IR2_BASE  0xE5000u
#define IR3_BASE  0xFC000u
static uint32_t ir_addr(uint32_t s)
{
    return s < 8u ? IR_BASE + s * IR_SLOT : s < 19u ? IR2_BASE + (s - 8u) * IR_SLOT : IR3_BASE + (s - 19u) * IR_SLOT;
}
#define IR_MAGIC  0x5249504Fu                    /* "OPIR" */
#define IR_HDR    64u
/* OpenPocket Bass: version 2 holds PX_IR_TAPS (2000) samples; version 1 (1024) is still read. The samples run from byte
 * 64 to the sector's 0xF00, skip the update loader's 8 bytes there (left erased), and go on from 0xF08: 4072 bytes.
 * The editor sends them contiguous (64 + 2 x 2000 = 4064 bytes in the staging buffer); ir_flash_rw splits them. */
#define IR_GAP_AT 0xF00u
#define IR_GAP    8u
#define IR_BYTES  (IR_HDR + PX_IR_TAPS * 2u)     /* 4064: the staged record (samples contiguous) */
#define IR_CHUNK  112u                           /* bytes per IR_WRITE (7-bit packed: 128) */
typedef char ir_fit_check[IR_BYTES + IR_GAP <= IR_SLOT ? 1 : -1];

typedef struct {
    uint32_t magic;
    uint16_t ver, n;
    char name[12];
    float scale;                                 /* sample k = s[k] / 32767 * scale */
    uint16_t crc;                                /* CRC16 (ota_crc16) of the samples */
    uint8_t pad[IR_HDR - 26u];
} ir_hdr_t;
typedef char ir_hdr_check[sizeof(ir_hdr_t) == IR_HDR ? 1 : -1];
typedef char ir_stage_check[sizeof nm_stage >= IR_BYTES ? 1 : -1];
#define ir_stage nm_stage                        /* (the NAM models' staging buffer: one upload at a time) */

static struct { uint8_t ok; char name[12]; } ir_cache[PX_IR_N];
static uint32_t ir_gen;

static int ir_hdr_ok(const ir_hdr_t *h)
{
    return h->magic == IR_MAGIC && ((h->ver == 1u && h->n == 1024u) || (h->ver == 2u && h->n == PX_IR_TAPS))
        && h->scale > 0.0f && h->scale < 1000.0f;
}
/* the samples' bytes [0, n) of the slot at base, around the gap: read (w == 0) into buf, or written from buf */
static int ir_flash_rw(uint32_t base, uint8_t *buf, uint32_t n, int w)
{
    const uint32_t a = IR_HDR + n <= IR_GAP_AT ? n : IR_GAP_AT - IR_HDR;
    if (w ? fl_write(base + IR_HDR, buf, a) : ota_fread(base + IR_HDR, buf, a)) return 1;
    if (n > a && (w ? fl_write(base + IR_GAP_AT + IR_GAP, buf + a, n - a) : ota_fread(base + IR_GAP_AT + IR_GAP, buf + a, n - a)))
        return 1;
    return 0;
}
static void ir_scan(void)
{
    ir_hdr_t h;
    for (uint32_t s = 0; s < PX_IR_N; s++) {
        ir_cache[s].ok = 0;
        if (ota_fread(ir_addr(s), &h, sizeof h) || !ir_hdr_ok(&h)) continue;
        ir_cache[s].ok = 1;
        nm_name_clean(ir_cache[s].name, h.name);
    }
    ir_gen++;
}
/* the engine's hook (engine/px_dsp.h): the name from the cache, the samples from the flash (checked) */
int px_ir_user(int slot, char *name12, float *h)
{
    if (slot < 0 || slot >= PX_IR_N || !ir_cache[slot].ok) return 0;
    memcpy(name12, ir_cache[slot].name, 12);
    if (h) {
        ir_hdr_t hd;
        const uint32_t off = ir_addr((uint32_t)slot);
        int16_t *sm = (int16_t *)h + PX_IR_LEN;        /* the samples into the upper half of h, then spread out */
        if (ota_fread(off, &hd, sizeof hd) || !ir_hdr_ok(&hd)) return 0;
        const int n = hd.n;
        if (ir_flash_rw(off, (uint8_t *)sm, (uint32_t)n * 2u, 0)) return 0;
        if ((uint16_t)ota_crc16((const uint8_t *)sm, (uint32_t)n * 2u, 0) != hd.crc) return 0;
        const float k = hd.scale / 32767.0f;
        for (int i = 0; i < n; i++) h[i] = (float)sm[i] * k;   /* (h[i] is past sm[i / 2]: read before written) */
        for (int i = n; i < PX_IR_LEN; i++) h[i] = 0.0f;
    }
    return 1;
}
/* IR_WRITE: piece idx of the staging buffer; 0 = ok */
static int ir_write_piece(uint32_t idx, const uint8_t *packed, uint32_t n)
{
    uint32_t off = idx * IR_CHUNK;
    if (off >= IR_BYTES) return 1;
    uint32_t want = IR_BYTES - off < IR_CHUNK ? IR_BYTES - off : IR_CHUNK;
    return nm_unpack7(packed, n, ir_stage + off, want) == want ? 0 : 1;
}
/* IR_COMMIT: the staged IR into slot s. 0 ok, 1 invalid, 2 flash */
static int ir_commit(uint32_t s)
{
    ir_hdr_t *h = (ir_hdr_t *)ir_stage;
    if (s >= PX_IR_N || !ir_hdr_ok(h) || h->ver != 2u) return 1;
    if ((uint16_t)ota_crc16(ir_stage + IR_HDR, PX_IR_TAPS * 2u, 0) != h->crc) return 1;
    nm_name_clean(h->name, h->name);
    const uint32_t base = ir_addr(s);
    uint32_t k;
    int rc = 0;
    st_audio_hold(1);
    if (fl_erase4k(base, &k)) rc = 2;
    if (!rc && ir_flash_rw(base, ir_stage + IR_HDR, IR_BYTES - IR_HDR, 1)) rc = 2;
    if (!rc && fl_write(base, ir_stage, IR_HDR)) rc = 2;   /* header last: a cut write leaves no valid slot */
    if (!rc) {
        static uint8_t chk[256];
        if (ota_fread(base, chk, IR_HDR) || memcmp(chk, ir_stage, IR_HDR)) rc = 2;
        for (uint32_t o = 0, m; !rc && o < IR_BYTES - IR_HDR; o += m) {   /* the samples, in pieces that stop at the gap */
            const uint32_t at = IR_HDR + o;
            m = IR_BYTES - at < sizeof chk ? IR_BYTES - at : sizeof chk;
            if (at < IR_GAP_AT && at + m > IR_GAP_AT) m = IR_GAP_AT - at;
            if (ota_fread(base + (at < IR_GAP_AT ? at : at + IR_GAP), chk, m) || memcmp(chk, ir_stage + at, m)) rc = 2;
        }
    }
    st_audio_hold(0);
    ir_scan();
    return rc;
}
/* IR_ERASE: slot s emptied. 0 ok, 1 invalid, 2 flash */
static int ir_erase(uint32_t s)
{
    uint32_t k;
    int rc = 0;
    if (s >= PX_IR_N) return 1;
    st_audio_hold(1);
    if (fl_erase4k(ir_addr(s), &k)) rc = 2;
    st_audio_hold(0);
    ir_scan();
    return rc;
}
