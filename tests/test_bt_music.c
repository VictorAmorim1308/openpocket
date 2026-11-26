/* OpenPocket Bass, Bluetooth audio: firmware/bt/bt_music.h on the PC.
 *  1. the SBC decoder: tests/data/bt_sine8.sbc (ffmpeg's SBC encoder: 441 Hz left, 1102.5 Hz right, 44.1 kHz stereo,
 *     8 subbands) and bt_sine4.sbc (441 Hz, 4 subbands, stereo: ffmpeg's encoder leaves the right channel of a 4-band
 *     stream silent, so only the left one is checked) decode to their tones (each channel's fit to its tone, and the frame count);
 *  2. the ring: a phone whose clock runs 300 ppm fast or slow, at 44.1 or 48 kHz, sending 7 frames every 20 ms with
 *     jitter, while the main loop sometimes stalls for 60 ms: after the start the ring never runs empty or over, its level
 *     settles near BT_PRIME, and the 441 Hz tone comes out clean (the interpolator's error);
 *  3. the mix: the music's gain (phone x pedal, squared) on top of the bass, the soft knee, the fade when the ring stops. */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../firmware/bt/bt_music.h"

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL: "); printf(__VA_ARGS__); printf("\n"); } } while (0)

/* the tone's share of x (n samples): 10 log10(power / residual) of a least-squares fit of sin and cos at f */
static double tone_snr(const double *x, int n, double f, double fs)
{
    double ss = 0, sc = 0, cc = 0, xs = 0, xc = 0, p = 0, e = 0;
    for (int i = 0; i < n; i++) { double s = sin(2 * M_PI * f * i / fs), c = cos(2 * M_PI * f * i / fs); ss += s * s; cc += c * c; sc += s * c; xs += x[i] * s; xc += x[i] * c; }
    double det = ss * cc - sc * sc, a = (xs * cc - xc * sc) / det, b = (xc * ss - xs * sc) / det;
    for (int i = 0; i < n; i++) { double y = a * sin(2 * M_PI * f * i / fs) + b * cos(2 * M_PI * f * i / fs); p += y * y; e += (x[i] - y) * (x[i] - y); }
    return 10 * log10(p / (e + 1e-12));
}

static void decode_file(const char *path, int nsb, int nch, double fr)
{
    static uint8_t buf[65536]; static int16_t pcm[256]; static double l[40000], r[40000];
    FILE *f = fopen(path, "rb");
    if (!f) { CHECK(0, "%s missing", path); return; }
    int n = (int)fread(buf, 1, sizeof buf, f), p = 0, m = 0;
    fclose(f);
    bt_sbc_t s; memset(&s, 0, sizeof s);
    while (p < n) {
        int u = bt_sbc_frame(&s, buf + p, n - p, pcm);
        if (!u) break;
        p += u;
        for (int k = 0; k < s.nblk * s.nsb && m < 40000; k++, m++) { l[m] = pcm[2 * k] / 32768.0; r[m] = pcm[2 * k + 1] / 32768.0; }
    }
    CHECK(p == n && s.bad == 0, "%s: %d of %d bytes decoded, %u bad", path, p, n, s.bad);
    CHECK(s.nsb == nsb && s.fs == 2 && s.nch == nch, "%s: header nsb %d fs %d nch %d", path, s.nsb, s.fs, s.nch);
    CHECK(m >= 21000, "%s: %d frames", path, m);
    double sl = tone_snr(l + 2000, 16000, 441, 44100), sr = fr > 0 ? tone_snr(r + 2000, 16000, fr, 44100) : 99;
    printf("  %s: %u frames, bitpool %d, %d samples: left %.1f dB, right %.1f dB\n", path, s.frames, s.bitpool, m, sl, sr);
    CHECK(sl > 40 && sr > 40, "%s: tones %.1f / %.1f dB", path, sl, sr);
}

static void ring_run(int fs, double ppm, int seconds)
{
    /* the state, from zero */
    bt_w = bt_r = 0; bt_rf = 0; bt_run = 0; bt_fill = 0; bt_adj = 0; bt_gm = 0; bt_under = bt_over = 0; bt_rate = fs;
    bt_music_level(100, 100);
    const double fin = fs * (1 + ppm * 1e-6);              /* the phone's real rate */
    const int blocks = seconds * 44100 / 128;
    static int32_t d[256]; static double out[44100 * 4]; int nout = 0;
    double t_phone = 0, next_pkt = 0, phase = 0; uint32_t minl = 1u << 30, maxl = 0; int started = 0, under0 = 0;
    uint32_t seed = 1;
    int16_t pcm[2 * 7 * 128];
    double next_poll = 0, stall_until = 0;
    for (int b = 0; b < blocks; b++) {
        double now = b * 128.0 / 44100.0;
        /* the phone: 7 SBC frames (896 samples) per packet, every 896 / fin s, delivered with up to 15 ms of jitter;
         * the main loop takes them when it polls (every 1..5 ms, and a 60 ms stall every 2 s) */
        while (next_pkt <= now) {
            if (now >= next_poll && now >= stall_until) {
                for (int k = 0; k < 896; k++) { double v = 0.5 * sin(phase); phase += 2 * M_PI * 441.0 / fs; pcm[2 * k] = pcm[2 * k + 1] = (int16_t)lrint(v * 32767); }
                (void)pcm; 
                for (int c = 0; c < 7; c++) bt_music_put(pcm + 2 * 128 * c, 128);
                t_phone += 896 / fin;
                seed = seed * 1103515245u + 12345u;
                next_pkt = t_phone + ((seed >> 16) % 15000) * 1e-6;
                next_poll = now + (1 + (seed >> 8) % 5) * 1e-3;
            } else break;
        }
        if (fmod(now, 2.0) < 128.0 / 44100.0) stall_until = now + 0.060;
        for (int i = 0; i < 256; i++) d[i] = 0;
        bt_music_mix(d, 128, 1.0f);
        if (bt_run) started = 1;
        if (started && b > 44100 / 128) {           /* after the first second */
            uint32_t lv = bt_w - bt_r; if (lv < minl) minl = lv; if (lv > maxl) maxl = lv;
            if (!under0) under0 = 1;
        }
        if (b >= blocks - 4 * 44100 / 128 && nout + 128 <= 44100 * 4) for (int i = 0; i < 128; i++) out[nout++] = d[2 * i] / 8388607.0;
    }
    double f_out = 441.0 * (1 + ppm * 1e-6) * fs / fs;     /* the tone at the phone's real clock */
    double snr = 999;                                       /* the worst 50 ms window (the step's slow wobble is not distortion) */
    for (int w = 0; w + 2205 <= nout; w += 2205) {
        double best = -999;                                 /* the window's own frequency: within +-0.4 Hz of the tone */
        for (double df = -0.4; df <= 0.4; df += 0.01) { double x = tone_snr(out + w, 2205, f_out + df, 44100); if (x > best) best = x; }
        if (best < snr) snr = best;
    }
    printf("  %d Hz %+5.0f ppm: level %u..%u (prime %u), adj %+.0f ppm, under %u, over %u, tone %.1f dB\n",
           fs, ppm, minl, maxl, BT_PRIME, bt_adj * 1e6, bt_under, bt_over, snr);
    CHECK(bt_under == 0 && bt_over == 0, "%d Hz %+.0f ppm: under %u over %u", fs, ppm, bt_under, bt_over);
    CHECK(minl > 500 && maxl < BT_RING - 1500, "%d Hz %+.0f ppm: level %u..%u", fs, ppm, minl, maxl);
    CHECK(snr > 60, "%d Hz %+.0f ppm: tone %.1f dB", fs, ppm, snr);   /* (SBC itself: about 70 dB at best) */
}

static void mix_checks(void)
{
    static int32_t d[256];
    static int16_t pcm[2 * 6000];
    bt_w = bt_r = 0; bt_rf = 0; bt_run = 0; bt_fill = 0; bt_adj = 0; bt_gm = 0; bt_under = bt_over = 0; bt_rate = 44100;
    for (int k = 0; k < 6000; k++) pcm[2 * k] = pcm[2 * k + 1] = 16384;   /* a DC level of 0.5 */
    bt_music_put(pcm, 6000);
    bt_music_level(50, 100);                                 /* 0.25 */
    for (int b = 0; b < 20; b++) { for (int i = 0; i < 256; i++) d[i] = 0; bt_music_mix(d, 128, 1.0f); }
    double v = d[254] / 8388607.0;
    CHECK(fabs(v - 0.125) < 0.002, "gain: phone 50 %%, pedal 100 %% -> %.4f (0.125)", v);
    bt_music_level(100, 100);
    for (int b = 0; b < 4; b++) { for (int i = 0; i < 256; i++) d[i] = (int32_t)(0.6 * 8388607); bt_music_mix(d, 128, 1.0f); }
    v = d[254] / 8388607.0;                                  /* 0.6 + 0.5 = 1.1 through the knee */
    CHECK(v > 0.8 && v < 1.0, "the soft knee: %.4f", v);
    for (int i = 0; i < 256; i++) d[i] = 0;
    uint32_t before = bt_under;
    for (int b = 0; b < 60; b++) { for (int i = 0; i < 256; i++) d[i] = 0; bt_music_mix(d, 128, 1.0f); }
    CHECK(bt_under == before + 1 && !bt_run, "the ring ran empty once and stopped (%u)", bt_under - before);
    CHECK(fabs(d[254] / 8388607.0) < 0.001, "silent after the fade: %.5f", d[254] / 8388607.0);
}

int main(void)
{
    printf("SBC decoder\n");
    decode_file("tests/data/bt_sine8.sbc", 8, 2, 1102.5);
    decode_file("tests/data/bt_sine4.sbc", 4, 2, 0);
    printf("ring and drift\n");
    ring_run(44100, 300, 120); ring_run(44100, -300, 120); ring_run(48000, 300, 120); ring_run(48000, -300, 120); ring_run(44100, 0, 60);
    printf("mix\n");
    mix_checks();
    printf(fails ? "test_bt_music: %d FAILED\n" : "test_bt_music: ok\n", fails);
    return fails != 0;
}
