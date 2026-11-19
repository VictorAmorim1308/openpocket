/* OpenPocket Bass: the bass additions against what they promise.
 *  1. USER IR's BLEND through the two-core pipeline (PRE/NAM/POST with the clean signal handed over in cln_w/cln_r,
 *     double-buffered as firmware/px_audio.h does) gives the plain engine's output bit for bit, one block later;
 *  2. BLEND 0 leaves the output as it was without the clean path; BLEND 100 is the clean signal, upside down with
 *     CLN PHASE at INVERT;
 *  3. BLEND 50 with each factory NAM model: with CLN PHASE set the better way the low E is not cancelled (some amps
 *     turn the lows upside down: what CLN PHASE is for);
 *  4. BASS CHORUS keeps a 41 Hz low E untouched and moves a 1 kHz tone;
 *  5. BASS PEQ's mid reaches 40 Hz and its texts read in Hz;
 *  5b. BASS OCTAVE: on plucked low notes (strong 2nd harmonic, decaying) the sub is one octave down and steady;
 *  6. the tuner (firmware/px_audio.h, its code copied here) reads the strings of a 5-string bass within 2 cents. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define PX_IR_USER_GIVEN
#include "../../engine/px_dsp.c"

static float cab_ir[PX_IR_LEN];
int px_ir_user(int slot, char *name12, float *h)       /* slot 0: a dark, short bass-cab-like IR; the others empty */
{
    if (slot != 0) return 0;
    if (name12) strcpy(name12, "TEST 8X10");
    if (h) memcpy(h, cab_ir, sizeof cab_ir);
    return 1;
}

#define FR 128
#define NB 200
static px_engine e;
static float sig[FR * NB], ref[FR * NB], got[FR * NB];
static int fails;
#define CHECK(c, m) do { if (c) printf("  ok    %s\n", m); else { printf("  FAIL  %s\n", m); fails++; } } while (0)

static void bass_preset(px_preset_t *p, int blend, int clv, int nam_on)
{
    memset(p, 0, sizeof *p);
    px_order_default(p->order);
    p->on[PX_B_GATE] = 1; p->p[PX_B_GATE][PX_PATCH_LEVEL] = 50;
    p->on[PX_B_AMP] = (uint8_t)nam_on; p->type[PX_B_AMP] = PX_AMP_NAM;
    p->p[PX_B_AMP][0] = (uint8_t)px_nam_knob(0); p->p[PX_B_AMP][1] = 50; p->p[PX_B_AMP][2] = 50;
    p->p[PX_B_AMP][3] = 50; p->p[PX_B_AMP][4] = 50;
    p->on[PX_B_CAB] = 1; p->type[PX_B_CAB] = PX_CAB_IR;
    p->p[PX_B_CAB][0] = (uint8_t)px_ir_knob(0); p->p[PX_B_CAB][1] = 0; p->p[PX_B_CAB][2] = 0;
    p->p[PX_B_CAB][3] = (uint8_t)blend; p->p[PX_B_CAB][4] = (uint8_t)clv;
}

static void run_plain(const px_preset_t *p, const float *in, float *out, int nfr)
{
    px_engine_init(&e, 44100.0f); px_engine_set(&e, p);
    for (int o = 0; o < nfr; o += 64) px_engine_process(&e, in + o, out + o, 64);
}

static float rms(const float *x, int n) { double s = 0; for (int i = 0; i < n; i++) s += (double)x[i] * x[i]; return (float)sqrt(s / n); }

/* the tuner, as in firmware/px_audio.h (between its markers) */
static uint32_t pxc_status __attribute__((unused));
#define PXC_READY 1
#define OP_BLE_TEST 0
#include "tuner_copy.h"

int main(void)
{
    for (int i = 0; i < PX_IR_LEN; i++) {               /* a decaying, low-passed noise burst: a cabinet of sorts */
        static unsigned s = 99;
        s = s * 1103515245u + 12345u;
        cab_ir[i] = (((s >> 8) & 65535) / 32768.0f - 1.0f) * expf(-(float)i / 60.0f) * 0.2f;
    }
    for (int i = 1; i < PX_IR_LEN; i++) cab_ir[i] = 0.7f * cab_ir[i - 1] + 0.3f * cab_ir[i];
    cab_ir[0] += 0.5f;
    unsigned s = 7;
    for (int i = 0; i < FR * NB; i++) {
        s = s * 1103515245u + 12345u;
        sig[i] = 0.3f * sinf((float)i * 0.0157f) * (i % 9000 < 5000) + 0.02f * (((s >> 8) & 65535) / 32768.0f - 1.0f);
    }

    printf("1. BLEND through the pipeline\n");
    {
        const int cases[][2] = {{50, 0}, {100, 100}, {30, 60}, {0, 0}};
        for (unsigned c = 0; c < sizeof cases / sizeof cases[0]; c++) {
            px_preset_t p;
            bass_preset(&p, cases[c][0], cases[c][1], 1);
            run_plain(&p, sig, ref, FR * NB);
            px_engine_init(&e, 44100.0f); px_engine_set(&e, &p);
            static float nb[2][FR], cl[2][FR];
            int cur = 0;
            memset(got, 0, sizeof got);
            for (int b = 0; b < NB; b++) {
                if (b) for (int o = 0; o < FR; o += 64) {
                    px_cln_r = cl[cur ^ 1] + o;
                    px_engine_part(&e, nb[cur ^ 1] + o, got + FR * (b - 1) + o, 64, PX_PART_POST);
                }
                for (int o = 0; o < FR; o += 64) {
                    px_cln_w = cl[cur] + o;
                    px_engine_part(&e, sig + FR * b + o, nb[cur] + o, 64, PX_PART_PRE);
                }
                for (int o = 0; o < FR; o += 64) px_engine_part(&e, nb[cur] + o, nb[cur] + o, 64, PX_PART_NAM);
                cur ^= 1;
            }
            char m[96];
            snprintf(m, sizeof m, "BLEND %d CLN PHASE %d: the pipeline's output = the plain one, one block later",
                     cases[c][0], cases[c][1]);
            CHECK(!memcmp(ref, got, sizeof(float) * FR * (NB - 1)), m);
        }
    }

    printf("2. BLEND 0 and 100\n");
    {
        px_preset_t p0, p100, pdi;
        bass_preset(&p0, 0, 0, 1);
        static float a[FR * NB], b[FR * NB];
        run_plain(&p0, sig, a, FR * NB);
        bass_preset(&pdi, 0, 100, 1);                   /* at BLEND 0 the clean path is not heard */
        run_plain(&pdi, sig, b, FR * NB);
        CHECK(!memcmp(a, b, sizeof a), "BLEND 0: CLN PHASE changes nothing");
        bass_preset(&p100, 100, 0, 1);                  /* BLEND 100: the same as amp off + cab off */
        run_plain(&p100, sig, a, FR * NB);
        px_preset_t q = p100; q.on[PX_B_AMP] = 0; q.on[PX_B_CAB] = 0;
        run_plain(&q, sig, b, FR * NB);
        double err = 0, ref2 = 0;
        for (int i = 4096; i < FR * NB; i++) { err += (a[i] - b[i]) * (a[i] - b[i]); ref2 += (double)b[i] * b[i]; }
        char m[96];
        snprintf(m, sizeof m, "BLEND 100: the clean signal alone (difference %.1f dB)", 10 * log10(err / ref2 + 1e-30));
        CHECK(err / ref2 < 1e-6, m);
        bass_preset(&p100, 100, 100, 1);                /* CLN PHASE INVERT: the clean signal upside down */
        run_plain(&p100, sig, a, FR * NB);
        err = 0;
        for (int i = 4096; i < FR * NB; i++) err += (a[i] + b[i]) * (a[i] + b[i]);
        snprintf(m, sizeof m, "CLN PHASE INVERT: the clean signal upside down (difference %.1f dB)", 10 * log10(err / ref2 + 1e-30));
        CHECK(err / ref2 < 1e-6, m);
        char t[16];
        px_param_text(PX_B_CAB, PX_CAB_IR, 4, 0, t);  CHECK(!strcmp(t, "NORMAL"), "CLN PHASE at 0 reads NORMAL");
        px_param_text(PX_B_CAB, PX_CAB_IR, 4, 100, t); CHECK(!strcmp(t, "INVERT"), "CLN PHASE at 100 reads INVERT");
    }

    printf("3. BLEND 50 on a 41 Hz low E with every factory NAM model: the lows with the better CLN PHASE\n");
    {
        /* the amp and the clean signal at a 41 Hz low E (empty IR slot: passes): with CLN PHASE set the way that sounds
         * fuller, BLEND 50 must not lose the low E against the louder of the two alone by more than 3 dB */
        static float in[44100], o[44100];
        for (int i = 0; i < 44100; i++) in[i] = 0.1f * sinf(2.0f * 3.14159265f * 41.2f * i / 44100.0f);
        for (int mo = 0; mo < PX_NAM_NFACT; mo++) {
            px_preset_t p;
            bass_preset(&p, 0, 0, 1);
            p.p[PX_B_AMP][0] = (uint8_t)px_nam_knob(mo); p.p[PX_B_AMP][1] = 30;
            p.p[PX_B_CAB][0] = (uint8_t)px_ir_knob(3);
            float lv[4];
            const int cfg[4][2] = {{0, 0}, {100, 0}, {50, 0}, {50, 100}};
            for (int c = 0; c < 4; c++) {
                p.p[PX_B_CAB][3] = (uint8_t)cfg[c][0]; p.p[PX_B_CAB][4] = (uint8_t)cfg[c][1];
                run_plain(&p, in, o, 44096);
                lv[c] = rms(o + 22050, 22000);
            }
            const float half = 0.5f * (lv[0] > lv[1] ? lv[0] : lv[1]), best = lv[2] > lv[3] ? lv[2] : lv[3];
            char nm[12], msg[128];
            px_nam_name(mo, nm);
            snprintf(msg, sizeof msg, "%-10s NORMAL %+5.1f dB, INVERT %+5.1f dB (vs half the louder): use %s", nm,
                     20 * log10f(lv[2] / half), 20 * log10f(lv[3] / half), lv[2] >= lv[3] ? "NORMAL" : "INVERT");
            CHECK(20 * log10f(best / half) > -3.0f, msg);
        }
    }

    printf("4. BASS CHORUS\n");
    {
        px_preset_t p;
        memset(&p, 0, sizeof p); px_order_default(p.order);
        p.on[PX_B_GATE] = 1; p.p[PX_B_GATE][PX_PATCH_LEVEL] = 50;
        p.on[PX_B_MOD] = 1; p.type[PX_B_MOD] = PX_MOD_BASSCHO;
        p.p[PX_B_MOD][0] = 40; p.p[PX_B_MOD][1] = 60; p.p[PX_B_MOD][2] = 100; p.p[PX_B_MOD][3] = 50;   /* X-OVER 158 Hz */
        char t[16];
        px_param_text(PX_B_MOD, PX_MOD_BASSCHO, 3, 50, t);
        CHECK(!strcmp(t, "158"), "X-OVER at 50 reads 158 (Hz)");
        CHECK(!strcmp(px_type_name(PX_B_MOD, PX_MOD_BASSCHO), "BASS CHORUS") && px_nparams(PX_B_MOD, PX_MOD_BASSCHO) == 4,
              "BASS CHORUS: its name, 4 settings");
        static float in[88200], out[88200], off[88200];
        for (int i = 0; i < 88200; i++) in[i] = 0.25f * sinf(2.0f * 3.14159265f * 41.2f * i / 44100.0f);
        run_plain(&p, in, out, 88192);
        p.on[PX_B_MOD] = 0;
        run_plain(&p, in, off, 88192);
        p.on[PX_B_MOD] = 1;
        double d = 0, r = 0;
        for (int i = 44100; i < 88192; i++) { d += (out[i] - off[i]) * (out[i] - off[i]); r += (double)off[i] * off[i]; }
        char m[96];
        snprintf(m, sizeof m, "41 Hz at MIX 100 against the chorus off: the difference %.1f dB below it", -10 * log10(d / r));
        CHECK(d / r < 0.01, m);
        for (int i = 0; i < 88200; i++) in[i] = 0.25f * sinf(2.0f * 3.14159265f * 1000.0f * i / 44100.0f);
        run_plain(&p, in, out, 88192);
        /* the chorus at 1 kHz: the envelope moves (beating between dry and the swept copy) */
        float lo = 1e9f, hi = 0;
        for (int w = 44100; w + 441 <= 88192; w += 441) { float v = rms(out + w, 441); if (v < lo) lo = v; if (v > hi) hi = v; }
        snprintf(m, sizeof m, "1 kHz is chorused: its level swings %.1f dB", 20 * log10f(hi / lo));
        CHECK(20 * log10f(hi / lo) > 3.0f, m);
    }

    printf("5. BASS PEQ\n");
    {
        char t[16];
        px_param_text(PX_B_EQ, PX_EQ_BASS, 1, 0, t);
        CHECK(!strcmp(t, "40"), "MID FREQ at 0 reads 40 (Hz)");
        px_param_text(PX_B_EQ, PX_EQ_BASS, 1, 100, t);
        CHECK(!strcmp(t, "4000"), "MID FREQ at 100 reads 4000 (Hz)");
        px_preset_t p;
        memset(&p, 0, sizeof p); px_order_default(p.order);
        p.on[PX_B_GATE] = 1; p.p[PX_B_GATE][PX_PATCH_LEVEL] = 50;
        p.on[PX_B_EQ] = 1; p.type[PX_B_EQ] = PX_EQ_BASS;
        p.p[PX_B_EQ][0] = 50; p.p[PX_B_EQ][1] = 20; p.p[PX_B_EQ][2] = 100; p.p[PX_B_EQ][3] = 30; p.p[PX_B_EQ][4] = 50;
        static float in[44100], out[44100];
        const float f = 40.0f * powf(100.0f, 0.2f);    /* 100 Hz */
        for (int i = 0; i < 44100; i++) in[i] = 0.1f * sinf(2.0f * 3.14159265f * f * i / 44100.0f);
        run_plain(&p, in, out, 44096);
        float g = 20 * log10f(rms(out + 22050, 22000) / rms(in + 22050, 22000));
        char m[96];
        snprintf(m, sizeof m, "MID GAIN +12 dB at 100 Hz: %+.1f dB", g);
        CHECK(fabsf(g - 12.0f) < 0.6f, m);
    }

    printf("5b. BASS OCTAVE\n");
    {
        CHECK(!strcmp(px_type_name(PX_B_PITCH, PX_PIT_BASSOCT), "BASS OCTAVE") && px_nparams(PX_B_PITCH, PX_PIT_BASSOCT) == 5,
              "BASS OCTAVE: its name, 5 settings");
        static const float HZ[] = {30.87f, 41.20f, 46.25f, 55.00f, 61.74f, 73.42f};
        static const char *NM[] = {"B0", "E1", "F#1", "A1", "B1", "D2"};
        static float in[88200], out[88200];
        for (unsigned k = 0; k < sizeof HZ / sizeof HZ[0]; k++) {
            double ph = 0;
            for (int i = 0; i < 88200; i++) {           /* plucked: 2nd harmonic stronger than the fundamental, decaying */
                const float env = 0.4f * expf(-(float)i / 40000.0f);
                /* the fundamental 12 dB under the 2nd harmonic: a bass string plucked near the bridge (the old trigger
                 * counted the harmonic there and the "octave down" landed on the note played) */
                in[i] = env * (0.25f * sinf((float)ph) + 1.0f * sinf(2 * (float)ph + 0.7f) + 0.6f * sinf(3 * (float)ph + 1.9f));
                ph += 2.0 * 3.14159265358979 * HZ[k] / 44100.0;
            }
            for (int up = 0; up < 2; up++) {
                px_preset_t p;
                memset(&p, 0, sizeof p); px_order_default(p.order);
                p.on[PX_B_GATE] = 1; p.p[PX_B_GATE][PX_PATCH_LEVEL] = 50;
                p.on[PX_B_PITCH] = 1; p.type[PX_B_PITCH] = PX_PIT_BASSOCT;
                p.p[PX_B_PITCH][0] = 0; p.p[PX_B_PITCH][1] = up ? 0 : 100; p.p[PX_B_PITCH][2] = up ? 100 : 0;
                p.p[PX_B_PITCH][3] = 40; p.p[PX_B_PITCH][4] = HZ[k] > 90.0f ? 60 : 30;
                run_plain(&p, in, out, 88192);
                /* Goertzel over the second half: the target (f/2 or 2f) against f, f/2, 2f, 3f/2 */
                const float tgt = up ? 2.0f * HZ[k] : 0.5f * HZ[k];
                const float cand[4] = {0.5f * HZ[k], HZ[k], 1.5f * HZ[k], 2.0f * HZ[k]};
                float pw[4], pt = 0;
                for (int c = 0; c < 4; c++) {
                    const float w = 2.0f * 3.14159265f * cand[c] / 44100.0f, co = 2.0f * cosf(w);
                    double s0 = 0, s1 = 0, s2 = 0;
                    for (int i = 44100; i < 88192; i++) { s0 = out[i] + co * s1 - s2; s2 = s1; s1 = s0; }
                    pw[c] = (float)(s1 * s1 + s2 * s2 - co * s1 * s2);
                    if (cand[c] == tgt) pt = pw[c];
                }
                float other = 0;
                for (int c = 0; c < 4; c++)                 /* (1.5 f: the sub's own 3rd harmonic, a square's) */
                    if (cand[c] != tgt && !(!up && c == 2) && pw[c] > other) other = pw[c];
                char m[112];
                snprintf(m, sizeof m, "%s %.1f Hz, %s: %.1f Hz is %.1f dB above the strongest other", NM[k], HZ[k],
                         up ? "UP +1" : "SUB -1", tgt, 10 * log10f(pt / (other + 1e-30f)));
                CHECK(10 * log10f(pt / (other + 1e-30f)) > (up ? -6.0f : 10.0f), m);   /* (before the tracking filter: SUB -35..-43 dB, the note itself) */   /* (an analog octave up is never pure) */
            }
        }
    }

    printf("6. tuner\n");
    {
        static const float HZ[] = {30.87f, 41.20f, 55.00f, 73.42f, 98.00f, 130.81f, 196.0f, 261.6f};
        static const char *NM[] = {"B0", "E1", "A1", "D2", "G2", "C3", "G3 (harmonic, 12th fret)", "C4 (harmonic, 12th fret)"};
        for (unsigned k = 0; k < sizeof HZ / sizeof HZ[0]; k++) {
            /* a plucked bass string: a strong 2nd harmonic, some 3rd and 4th, decaying, noise */
            static float blk[128];
            tn_w = 0; memset(tn_buf, 0, sizeof tn_buf);
            double ph = 0;
            unsigned sd = 3;
            for (int b = 0; b < 344; b++) {             /* 1 s */
                for (int i = 0; i < 128; i++) {
                    int n = b * 128 + i;
                    float env = 0.5f * expf(-(float)n / 30000.0f);
                    sd = sd * 1103515245u + 12345u;
                    blk[i] = env * (0.6f * sinf((float)ph) + 0.8f * sinf(2 * (float)ph + 0.3f) + 0.3f * sinf(3 * (float)ph)
                                    + 0.2f * sinf(4 * (float)ph)) + 0.002f * (((sd >> 8) & 65535) / 32768.0f - 1.0f);
                    ph += 2.0 * 3.14159265358979 * HZ[k] / 44100.0;
                }
                tn_capture(blk, 128);
            }
            float hz = tuner_estimate();
            float cents = hz > 0 ? 1200.0f * log2f(hz / HZ[k]) : 9999.0f;
            char m[96];
            snprintf(m, sizeof m, "%s %.2f Hz: read %.2f Hz (%+.1f cents)", NM[k], HZ[k], hz, cents);
            CHECK(fabsf(cents) < (HZ[k] > 150.0f ? 3.0f : 2.0f), m);
        }
    }
#if PX_BASS
    printf("7. the bass factory presets (PX_BASS)\n");
    {
        int ok = 1, eqfirst = 1, piped = 1;
        for (int f = 0; f < px_preset_count(); f++) {
            px_preset_t p;
            px_preset_factory(&p, f);
            if (!px_order_valid(p.order)) ok = 0;
            int ie = -1, ia = -1, ic = -1, im = -1, ip = -1;
            for (int i = 0; i < PX_NBLK; i++) {
                if (p.order[i] == PX_B_PITCH) ip = i;
                if (p.order[i] == PX_B_EQ) ie = i;
                if (p.order[i] == PX_B_AMP) ia = i;
                if (p.order[i] == PX_B_CAB) ic = i;
                if (p.order[i] == PX_B_MOD) im = i;
            }
            if (!(ip < ie && ie < ia && ia < ic && ic < im)) eqfirst = 0;
            for (int b = 0; b < PX_NBLK; b++)
                if (p.on[b] && b != PX_B_EQ && b != PX_B_AMP && b != PX_B_CAB && b != PX_B_MOD && b != PX_B_COMP &&
                    b != PX_B_PITCH) ok = 0;
            if (f < 3) printf("        %02d %s\n", f + 1, p.name);
            /* the plain engine and the pipeline agree on every preset that uses the NAM amp */
            if (p.on[PX_B_AMP] && f < 12) {
                p.p[PX_B_AMP][0] = (uint8_t)px_nam_knob(f % PX_NAM_NFACT);   /* (the user slots are empty here: a factory model) */
                run_plain(&p, sig, ref, FR * 60);
                px_engine_init(&e, 44100.0f); px_engine_set(&e, &p);
                static float nb[2][FR], cl[2][FR];
                int cur = 0;
                memset(got, 0, sizeof got);
                for (int b = 0; b < 60; b++) {
                    if (b) for (int o = 0; o < FR; o += 64) { px_cln_r = cl[cur ^ 1] + o; px_engine_part(&e, nb[cur ^ 1] + o, got + FR * (b - 1) + o, 64, PX_PART_POST); }
                    for (int o = 0; o < FR; o += 64) { px_cln_w = cl[cur] + o; px_engine_part(&e, sig + FR * b + o, nb[cur] + o, 64, PX_PART_PRE); }
                    for (int o = 0; o < FR; o += 64) px_engine_part(&e, nb[cur] + o, nb[cur] + o, 64, PX_PART_NAM);
                    cur ^= 1;
                }
                if (memcmp(ref, got, sizeof(float) * FR * 59)) piped = 0;
            }
        }
        CHECK(ok, "every preset: a valid order, only EQ, AMP, CAB, MOD (COMP on SLAP, PITCH on OITAVA) on");
        CHECK(eqfirst, "every preset: PITCH -> EQ -> AMP -> CAB -> MOD");
        CHECK(piped, "the NAM presets (a factory model in the slot): the pipeline = the plain engine, one block later");
    }
#endif
    printf(fails ? "bass test: %d FAILED\n" : "bass test passed\n", fails);
    return fails != 0;
}
