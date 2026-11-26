/* SPDX-License-Identifier: GPL-3.0-only OR MPL-2.0 */
/* OpenPocket Bass, Bluetooth audio: the music's path from the phone's SBC frames to the output, beside the bass.
 *
 *   bt_music_sbc(p, n)    main loop (bt_a2dp.c): decodes the SBC frames of one A2DP packet into the music ring
 *   bt_music_mix(d, n)    audio interrupt (px_audio.h a_send): adds the ring's music to the bass, both channels
 *
 * The ring holds 16-bit stereo at the phone's rate (44.1 or 48 kHz). The interrupt reads it at its own pace through a
 * 4-point interpolator whose step is the two rates' ratio, corrected by how full the ring is: the phone's clock and the
 * pedal's never agree exactly, so a ring that slowly fills is read a little faster and one that empties a little slower
 * (at most +-0.5 %, far below hearing: the SDK's decoder does the same, bt_decode.c bt_sync). An empty ring fades the
 * music out and waits until it holds BT_PRIME frames again.
 * No hardware access: tests/test_bt_music.c runs it on the PC against ffmpeg's SBC decoder. The SBC decoder follows the
 * A2DP specification (section 12.6: bit allocation, reconstruction, the synthesis filter bank). */
#pragma once
#include <stdint.h>
#include <string.h>

#ifndef BT_RAM
#define BT_RAM                                 /* (the pedal's openpocket.c: the free end of the pool, app.ld .pool.tail) */
#endif
#define BT_RING 12288u                         /* stereo frames (48 KB): 279 ms at 44.1 kHz */
#define BT_PRIME 4608u                         /* frames held before the music starts, and the level the step keeps */
#define BT_FS_OUT 44100.0f

/* ---------------------------------------------------------------- SBC (A2DP 1.3, section 12) */
typedef struct {
    float v[2][160];                           /* the synthesis filter's history per channel (20 M values) */
    int fs, nch, nsb, nblk, mode, bitpool;     /* the last frame's header */
    uint32_t frames, bad;
} bt_sbc_t;

static const int bt_sbc_fs[4] = {16000, 32000, 44100, 48000};
static const int8_t bt_sbc_off4[4][4] = {{-1, 0, 0, 0}, {-2, 0, 0, 1}, {-2, 0, 0, 1}, {-2, 0, 0, 1}};
static const int8_t bt_sbc_off8[4][8] = {{-2, 0, 0, 0, 0, 0, 0, 1}, {-3, 0, 0, 0, 0, 0, 1, 2},
                                        {-4, 0, 0, 0, 0, 0, 1, 2}, {-4, 0, 0, 0, 0, 0, 1, 2}};
/* the synthesis window D (10 M taps), in the order the U vector is built (12.6.4), as fractions of 2^31: the prototype
 * filters Proto_4_40 and Proto_8_80 of the specification's tables 12.23 and 12.24, with the synthesis gain */
static const int32_t bt_sbc_d4[40] = {
    0x00000000, (int32_t)0xffa6982f, (int32_t)0xfba93848, 0x0456c7b8, 0x005967d1, (int32_t)0xfffb9ac7, (int32_t)0xff589157, (int32_t)0xf9c2a8d8,
    0x027c1434, 0x0019118b, (int32_t)0xfff3c74c, (int32_t)0xff137330, (int32_t)0xf81b8d70, 0x00ec1b8b, (int32_t)0xfff0b71a, (int32_t)0xffe99b00,
    (int32_t)0xfef84470, (int32_t)0xf6fb4370, (int32_t)0xffcdc351, (int32_t)0xffe01dc7,
    (int32_t)0xffe090ce, (int32_t)0xff2c0475, (int32_t)0xf694f800, (int32_t)0xff2c0475, (int32_t)0xffe090ce, (int32_t)0xffe01dc7, (int32_t)0xffcdc351, (int32_t)0xf6fb4370,
    (int32_t)0xfef84470, (int32_t)0xffe99b00, (int32_t)0xfff0b71a, 0x00ec1b8b, (int32_t)0xf81b8d70, (int32_t)0xff137330, (int32_t)0xfff3c74c, 0x0019118b,
    0x027c1434, (int32_t)0xf9c2a8d8, (int32_t)0xff589157, (int32_t)0xfffb9ac7};
static const int32_t bt_sbc_d8[80] = {
    0x00000000, (int32_t)0xfe8d1970, (int32_t)0xee979f00, 0x11686100, 0x0172e690, (int32_t)0xfff5bd1a, (int32_t)0xfdf1c8d4, (int32_t)0xeac182c0,
    0x0d9daee0, 0x00e530da, (int32_t)0xffe9811d, (int32_t)0xfd52986c, (int32_t)0xe7054ca0, 0x0a00d410, 0x006c1de4, (int32_t)0xffdba705,
    (int32_t)0xfcbc98e8, (int32_t)0xe3889d20, 0x06af2308, 0x000bb7db, (int32_t)0xffca00ed, (int32_t)0xfc3fbb68, (int32_t)0xe071bc00, 0x03bf7948,
    (int32_t)0xffc4e05c, (int32_t)0xffb54b3b, (int32_t)0xfbedadc0, (int32_t)0xdde26200, 0x0142291c, (int32_t)0xff960e94, (int32_t)0xff9f3e17, (int32_t)0xfbd8f358,
    (int32_t)0xdbf79400, (int32_t)0xff405e01, (int32_t)0xff7d4914, (int32_t)0xff8b1a31, (int32_t)0xfc1417b8, (int32_t)0xdac7bb40, (int32_t)0xfdbb828c, (int32_t)0xff762170,
    (int32_t)0xff7c272c, (int32_t)0xfcb02620, (int32_t)0xda612700, (int32_t)0xfcb02620, (int32_t)0xff7c272c, (int32_t)0xff762170, (int32_t)0xfdbb828c, (int32_t)0xdac7bb40,
    (int32_t)0xfc1417b8, (int32_t)0xff8b1a31, (int32_t)0xff7d4914, (int32_t)0xff405e01, (int32_t)0xdbf79400, (int32_t)0xfbd8f358, (int32_t)0xff9f3e17, (int32_t)0xff960e94,
    0x0142291c, (int32_t)0xdde26200, (int32_t)0xfbedadc0, (int32_t)0xffb54b3b, (int32_t)0xffc4e05c, 0x03bf7948, (int32_t)0xe071bc00, (int32_t)0xfc3fbb68,
    (int32_t)0xffca00ed, 0x000bb7db, 0x06af2308, (int32_t)0xe3889d20, (int32_t)0xfcbc98e8, (int32_t)0xffdba705, 0x006c1de4, 0x0a00d410,
    (int32_t)0xe7054ca0, (int32_t)0xfd52986c, (int32_t)0xffe9811d, 0x00e530da, 0x0d9daee0, (int32_t)0xeac182c0, (int32_t)0xfdf1c8d4, (int32_t)0xfff5bd1a};
static BT_RAM float bt_sbc_win4[40], bt_sbc_win8[80], bt_sbc_n4[8][4], bt_sbc_n8[16][8];
static int bt_sbc_tables;

float cosf(float x);                           /* px_math.c on the pedal, the C library on the PC */
static void bt_sbc_init_tables(void)
{
    int k, i, j, t;
    /* the tables above hold, for output j and tap t, the even taps (m0: index 5 j + t) and then the odd ones (m1) */
    for (j = 0; j < 4; j++) for (t = 0; t < 5; t++) {
        bt_sbc_win4[j + 4 * (2 * t)] = (float)bt_sbc_d4[5 * j + t] * (1.0f / 2147483648.0f);
        bt_sbc_win4[j + 4 * (2 * t + 1)] = (float)bt_sbc_d4[20 + 5 * j + t] * (1.0f / 2147483648.0f);
    }
    for (j = 0; j < 8; j++) for (t = 0; t < 5; t++) {
        bt_sbc_win8[j + 8 * (2 * t)] = (float)bt_sbc_d8[5 * j + t] * (1.0f / 2147483648.0f);
        bt_sbc_win8[j + 8 * (2 * t + 1)] = (float)bt_sbc_d8[40 + 5 * j + t] * (1.0f / 2147483648.0f);
    }
    for (k = 0; k < 8; k++) for (i = 0; i < 4; i++) bt_sbc_n4[k][i] = cosf((i + 0.5f) * (k + 2) * 3.14159265f / 4.0f);
    for (k = 0; k < 16; k++) for (i = 0; i < 8; i++) bt_sbc_n8[k][i] = cosf((i + 0.5f) * (k + 4) * 3.14159265f / 8.0f);
    bt_sbc_tables = 1;
}

static uint32_t bt_bits(const uint8_t *p, uint32_t *pos, int n)   /* MSB first */
{
    uint32_t v = 0;
    while (n--) { v = (v << 1) | ((p[*pos >> 3] >> (7 - (*pos & 7))) & 1u); (*pos)++; }
    return v;
}

/* the frame's length from its header, or 0 */
static int bt_sbc_len(const uint8_t *p, int n)
{
    int nblk, mode, nch, nsb, bp, len;
    if (n < 4 || p[0] != 0x9C) return 0;
    nblk = 4 * (((p[1] >> 4) & 3) + 1); mode = (p[1] >> 2) & 3; nch = mode == 0 ? 1 : 2; nsb = p[1] & 1 ? 8 : 4; bp = p[2];
    len = 4 + (4 * nsb * nch) / 8;
    if (mode < 2) len += (nblk * nch * bp + 7) / 8;
    else len += ((mode == 3 ? nsb : 0) + nblk * bp + 7) / 8;
    return len;
}

/* bit allocation (12.6.3) for one channel (mono, dual) or both (stereo, joint): bits[ch][sb] */
static void bt_sbc_alloc(const bt_sbc_t *s, int snr, const uint8_t sf[2][8], int bits[2][8], int ch0, int nch)
{
    int need[2][8], ch, sb, maxn = 0, bitcount = 0, slice, slicecount = 0, bp = s->bitpool, fi = s->fs;
    const int8_t *off = s->nsb == 4 ? bt_sbc_off4[fi] : bt_sbc_off8[fi];
    for (ch = ch0; ch < ch0 + nch; ch++) for (sb = 0; sb < s->nsb; sb++) {
        int n;
        if (snr) n = sf[ch][sb];
        else if (!sf[ch][sb]) n = -5;
        else { int l = sf[ch][sb] - off[sb]; n = l > 0 ? l / 2 : l; }
        need[ch][sb] = n;
        if (n > maxn) maxn = n;
    }
    slice = maxn + 1;
    do {
        slice--; bitcount += slicecount; slicecount = 0;
        for (ch = ch0; ch < ch0 + nch; ch++) for (sb = 0; sb < s->nsb; sb++) {
            if (need[ch][sb] > slice + 1 && need[ch][sb] < slice + 16) slicecount++;
            else if (need[ch][sb] == slice + 1) slicecount += 2;
        }
    } while (bitcount + slicecount < bp);
    if (bitcount + slicecount == bp) { bitcount += slicecount; slice--; }
    for (ch = ch0; ch < ch0 + nch; ch++) for (sb = 0; sb < s->nsb; sb++) {
        int b = need[ch][sb] < slice + 2 ? 0 : need[ch][sb] - slice;
        bits[ch][sb] = b > 16 ? 16 : b;
    }
    ch = ch0; sb = 0;
    while (bitcount < bp && sb < s->nsb) {
        if (bits[ch][sb] >= 2 && bits[ch][sb] < 16) { bits[ch][sb]++; bitcount++; }
        else if (need[ch][sb] == slice + 1 && bp > bitcount + 1) { bits[ch][sb] = 2; bitcount += 2; }
        if (nch == 2 && ch == ch0) ch = ch0 + 1; else { ch = ch0; sb++; }
    }
    ch = ch0; sb = 0;
    while (bitcount < bp && sb < s->nsb) {
        if (bits[ch][sb] < 16) { bits[ch][sb]++; bitcount++; }
        if (nch == 2 && ch == ch0) ch = ch0 + 1; else { ch = ch0; sb++; }
    }
}

/* one frame -> out (stereo interleaved, nblk * nsb frames, mono doubled); the bytes it took, or 0 if it is not one */
static int bt_sbc_frame(bt_sbc_t *s, const uint8_t *p, int n, int16_t *out)
{
    uint8_t sf[2][8], join = 0;
    int bits[2][8], ch, sb, blk, len = bt_sbc_len(p, n), snr, fi;
    uint32_t pos = 32;
    float smp[16][2][8];
    if (!len || len > n) { s->bad++; return 0; }
    if (!bt_sbc_tables) bt_sbc_init_tables();
    fi = (p[1] >> 6) & 3;
    s->fs = fi; s->nblk = 4 * (((p[1] >> 4) & 3) + 1); s->mode = (p[1] >> 2) & 3; s->nch = s->mode == 0 ? 1 : 2;
    snr = (p[1] >> 1) & 1; s->nsb = p[1] & 1 ? 8 : 4; s->bitpool = p[2];
    if (s->mode == 3) {
        for (sb = 0; sb < s->nsb - 1; sb++) join |= (uint8_t)(bt_bits(p, &pos, 1) << sb);
        pos++;                                  /* RFA */
    }
    for (ch = 0; ch < s->nch; ch++) for (sb = 0; sb < s->nsb; sb++) sf[ch][sb] = (uint8_t)bt_bits(p, &pos, 4);
    if (s->mode < 2) for (ch = 0; ch < s->nch; ch++) bt_sbc_alloc(s, snr, sf, bits, ch, 1);
    else bt_sbc_alloc(s, snr, sf, bits, 0, 2);
    for (blk = 0; blk < s->nblk; blk++) for (ch = 0; ch < s->nch; ch++) for (sb = 0; sb < s->nsb; sb++) {
        int b = bits[ch][sb];
        if (!b) { smp[blk][ch][sb] = 0.0f; continue; }
        {
            uint32_t q = bt_bits(p, &pos, b);
            float lv = (float)((1u << b) - 1u);
            smp[blk][ch][sb] = (float)(2u << sf[ch][sb]) * ((2.0f * (float)q + 1.0f) / lv - 1.0f);
        }
    }
    if (s->mode == 3)
        for (blk = 0; blk < s->nblk; blk++) for (sb = 0; sb < s->nsb; sb++)
            if (join & (1u << sb)) {
                float a = smp[blk][0][sb], b = smp[blk][1][sb];
                smp[blk][0][sb] = a + b; smp[blk][1][sb] = a - b;
            }
    /* synthesis (12.6.4): V shifted by 2 M, the matrix, U from V, the window, the sums */
    {
        const int M = s->nsb, M2 = 2 * M, NV = 20 * M;
        const float *win = M == 8 ? bt_sbc_win8 : bt_sbc_win4;
        for (ch = 0; ch < s->nch; ch++) {
            float *v = s->v[ch];
            for (blk = 0; blk < s->nblk; blk++) {
                int k, i, j;
                const float *S = smp[blk][ch];
                for (i = NV - 1; i >= M2; i--) v[i] = v[i - M2];
                for (k = 0; k < M2; k++) {
                    float a = 0.0f;
                    const float *N = M == 8 ? bt_sbc_n8[k] : bt_sbc_n4[k];
                    for (i = 0; i < M; i++) a += N[i] * S[i];
                    v[k] = a;
                }
                for (j = 0; j < M; j++) {
                    float x = 0.0f;
                    for (i = 0; i < 5; i++) {
                        x += v[i * 4 * M + j] * win[j + M * (2 * i)];
                        x += v[i * 4 * M + 3 * M + j] * win[j + M * (2 * i + 1)];
                    }
                    x *= M == 8 ? 4.0f : 16.0f;      /* the tables' scale (2^-31 of the 8-band window, 2^-29 of the 4-band one) */
                    {
                        int32_t y = (int32_t)(x < 0.0f ? x - 0.5f : x + 0.5f);
                        if (y > 32767) y = 32767; else if (y < -32768) y = -32768;
                        out[2 * (blk * M + j) + ch] = (int16_t)y;
                        if (s->nch == 1) out[2 * (blk * M + j) + 1] = (int16_t)y;
                    }
                }
            }
        }
    }
    s->frames++;
    return len;
}

/* ---------------------------------------------------------------- the music ring and the mix */
static BT_RAM int16_t bt_ring[BT_RING][2];
static volatile uint32_t bt_w;                 /* frames written (main loop) */
static volatile uint32_t bt_r;                 /* the read position's whole part (interrupt) */
static float bt_rf;                            /* its fraction */
static volatile uint8_t bt_run;                /* 1: the interrupt plays the ring */
static volatile int bt_rate = 44100;           /* the phone's rate */
static float bt_fill;                          /* the ring's level, smoothed (interrupt) */
static float bt_adj;                           /* the step's correction (interrupt) */
static float bt_g, bt_gm;                      /* the music's gain: target (main loop), now (interrupt) */
static uint32_t bt_under, bt_over;             /* the ring ran empty / overflowed */
static BT_RAM bt_sbc_t bt_sbc;
static BT_RAM int16_t bt_pcm[16 * 8 * 2];

static uint32_t bt_music_free(void) { return BT_RING - 1u - (bt_w - bt_r); }

/* main loop: nf stereo frames into the ring (a block that does not fit is dropped and counted) */
static void bt_music_put(const int16_t *pcm, int nf)
{
    int k;
    if ((uint32_t)nf > bt_music_free()) { bt_over++; return; }
    for (k = 0; k < nf; k++) {
        uint32_t at = (bt_w + (uint32_t)k) % BT_RING;
        bt_ring[at][0] = pcm[2 * k]; bt_ring[at][1] = pcm[2 * k + 1];
    }
    bt_w += (uint32_t)nf;                        /* (after the data: the interrupt reads up to bt_w) */
}

/* main loop: the SBC frames of one packet into the ring */
static void bt_music_sbc(const uint8_t *p, int n)
{
    while (n >= 4) {
        int used, k, nf;
        if (p[0] != 0x9C) { p++; n--; continue; }
        used = bt_sbc_frame(&bt_sbc, p, n, bt_pcm);
        if (!used) break;
        p += used; n -= used;
        nf = bt_sbc.nblk * bt_sbc.nsb;
        bt_rate = bt_sbc_fs[bt_sbc.fs];
        bt_music_put(bt_pcm, nf);
        (void)k;
    }
}

/* the music's level: the phone's volume (0..100) times the pedal's MUSIC setting (0..100), with the same quadratic curve
 * as the master volume; the master volume itself applies to both (bt_music_mix's master) */
static void bt_music_level(int phone, int pedal)
{
    float a = (float)phone * 0.01f, b = (float)pedal * 0.01f;
    bt_g = a * a * b * b;
}

static float bt_cubic(const int16_t *a, const int16_t *b, const int16_t *c, const int16_t *d, int ch, float t)
{   /* 4-point Hermite between b and c */
    float y0 = a[ch], y1 = b[ch], y2 = c[ch], y3 = d[ch];
    float c1 = 0.5f * (y2 - y0), c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3, c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
    return ((c3 * t + c2) * t + c1) * t + y1;
}

/* audio interrupt: d holds the bass (24-bit, L R L R ...), n frames; the music is added with the master gain, and the sum
 * goes through the same soft knee as the bass alone (px_audio.h dsp_out) */
static void bt_music_mix(int32_t *d, uint32_t n, float master)
{
    uint32_t i, have = bt_w - bt_r;
    float step, gm = bt_gm, g = bt_g * master;
    bt_fill += ((float)have - bt_fill) * (1.0f / 64.0f);       /* per block (2.9 ms): ~0.2 s */
    if (!bt_run) {
        if (have >= BT_PRIME) { bt_run = 1; bt_fill = (float)have; bt_adj = 0.0f; }
        else if (gm < 1e-5f) return;
    }
    /* the step: the rates' ratio, corrected in proportion to the level's distance from BT_PRIME (one ring away: +-0.5 %) */
    {
        float e = (bt_fill - (float)BT_PRIME) * (1.0f / (float)BT_RING);
        bt_adj += (e * 0.005f - bt_adj) * (1.0f / 256.0f);
    }
    step = (float)bt_rate * (1.0f / BT_FS_OUT) * (1.0f + bt_adj);
    for (i = 0; i < n; i++) {
        float l = 0.0f, r = 0.0f, vl, vr;
        gm += ((bt_run ? g : 0.0f) - gm) * 0.004f;
        if (bt_run) {
            if (bt_w - bt_r < 4u) { bt_run = 0; bt_under++; }
            else {
                const int16_t *a = bt_ring[(bt_r + BT_RING - 1u) % BT_RING], *b = bt_ring[bt_r % BT_RING],
                              *c = bt_ring[(bt_r + 1u) % BT_RING], *e = bt_ring[(bt_r + 2u) % BT_RING];
                l = bt_cubic(a, b, c, e, 0, bt_rf); r = bt_cubic(a, b, c, e, 1, bt_rf);
                bt_rf += step;
                while (bt_rf >= 1.0f) { bt_rf -= 1.0f; bt_r++; }
            }
        }
        vl = (float)d[2 * i] * (1.0f / 8388607.0f) + l * gm * (1.0f / 32768.0f);
        vr = (float)d[2 * i + 1] * (1.0f / 8388607.0f) + r * gm * (1.0f / 32768.0f);
        {   /* the soft knee from 0.8 of full scale */
            float m = vl < 0.0f ? -vl : vl;
            if (m > 0.8f) { float t = (m - 0.8f) * 5.0f; m = 0.8f + 0.2f * t / (1.0f + t); vl = vl < 0.0f ? -m : m; }
            m = vr < 0.0f ? -vr : vr;
            if (m > 0.8f) { float t = (m - 0.8f) * 5.0f; m = 0.8f + 0.2f * t / (1.0f + t); vr = vr < 0.0f ? -m : m; }
        }
        d[2 * i] = (int32_t)(vl * 8388607.0f);
        d[2 * i + 1] = (int32_t)(vr * 8388607.0f);
    }
    bt_gm = gm;
}

/* "SBC 44 J53 R2048 U0": the stream's rate, mode and bitpool, the ring's level, the times it ran empty */
static char *bt_music_text(char *o)
{
    uint32_t v;
    char t[11]; int k;
    *o++ = 'S'; *o++ = 'B'; *o++ = 'C'; *o++ = ' ';
    v = (uint32_t)bt_rate / 1000u; k = 0; do { t[k++] = (char)('0' + v % 10u); v /= 10u; } while (v); while (k) *o++ = t[--k];
    *o++ = ' '; *o++ = "MDSJ"[bt_sbc.mode & 3];
    v = (uint32_t)bt_sbc.bitpool; k = 0; do { t[k++] = (char)('0' + v % 10u); v /= 10u; } while (v); while (k) *o++ = t[--k];
    *o++ = ' '; *o++ = 'R';
    v = bt_w - bt_r; k = 0; do { t[k++] = (char)('0' + v % 10u); v /= 10u; } while (v); while (k) *o++ = t[--k];
    *o++ = ' '; *o++ = 'U';
    v = bt_under; k = 0; do { t[k++] = (char)('0' + v % 10u); v /= 10u; } while (v); while (k) *o++ = t[--k];
    return o;
}
