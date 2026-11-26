/* SPDX-License-Identifier: GPL-3.0-only
 * OpenPocket X — UI v4 (v36): a PLAY screen and an EDIT screen, like the big multi-effects (Helix / HX Stomp,
 * Boss GT / GX): the preset readable from a distance, the chain in colour, big clear settings. No flicker: nothing
 * is cleared, every element is a fixed box redrawn in one pixel stream, and only when it changed.
 *
 *  PLAY                                   EDIT (a block)
 *  +------------------------------+       +------------------------------+
 *  | VOL 80     EDITED     C1 42% |       |#AMP ######################ON#|  band in the block's colour
 *  | 07   JCM900                  |       | <         JCM900          >  |  model
 *  |      4x12 GREEN              |       | GAIN                     55  |
 *  |        BRIT CRUNCH           |       | ==========-----------------  |  up to 5 settings, bar + value
 *  | [GT][CMP][PIT][SFX][SYN][DRV]|       | BASS                     50  |
 *  | [AMP][CAB][EQ][MOD][DLY][REV]|       | ...                          |
 *  | AMP   JCM900             ON  |       |                              |
 *  | PUSH EDIT   FOOT: PRESETS    |       | PUSH EDIT   HOME BACK DSP 42%|
 *  +------------------------------+       +------------------------------+
 *
 * Controls (ui_buttons, ui_turn and ui_action below: the main loop and the simulator both use them):
 * KNOB = select / adjust; PUSH = open / edit / confirm; HOME = back to PLAY, on PLAY the preset list (v42), hold 0.7 s =
 * tuner, held on = diagnostics; FOOTSWITCHES = next / previous preset, both together = tuner (v42);
 * AMP CAB DLY REV MOD = that block's page (press again = on/off); FX = the next of DRV, SFX, SYN, PIT, CMP, GT, EQ in the
 * preset's chain order (v42). Hold PUSH = SAVE (into one of the 52 slots, in flash). Hold FX = MIDI.
 * Double-click PUSH on PLAY = MOVE the selected block: KNOB moves it along the chain, PUSH drops it, HOME puts it back.
 * The preset LEVEL (v36) is the GT page's 3rd setting ("PRESET LVL").
 */
#pragma once

enum { M_CHAIN, M_BLOCK, M_EDIT, M_TUNER, M_SAVE, M_MOVE, M_MIDI, M_LIST, M_DIAG, M_PVOL, M_NAME };   /* M_NAME: renaming (Bass) */   /* M_PVOL: the preset's volume, on PLAY (Bass) */
static uint32_t st_gen;                 /* bumped on each save: the "EDITED" reference reloads */
static void st_get(uint32_t k, px_preset_t *p);
static int st_save(uint32_t k, const px_preset_t *p);
static int st_reset(uint32_t k);
static int st_saved(uint32_t k);
static void save_draw(void);
static void name_draw(void);
static void midi_draw(void);
static void list_draw(void);
static uint32_t ui_mode = M_CHAIN, ui_blk = PX_B_AMP, ui_item = 0;
static int ui_pvol;                     /* OpenPocket Bass: on PLAY the knob is on the preset's VOL (not a pedal) */
static int ui_sub;                      /* OpenPocket Bass: editing an AMP band: 0 its frequency, 1 its gain */
#define UI_PAGE (ui_mode == M_BLOCK || ui_mode == M_EDIT)   /* the block page has the focus */
#ifdef PX_BASS
#define PX_BASS_UI PX_BASS
#else
#define PX_BASS_UI 0
#endif

#define C_OFF  0x2124                  /* a block switched off */
#define C_LINE 0x2945                  /* thin lines, connectors */
#define C_CARD 0x18E3                  /* a selected row */
#define C_BAR  0x3186                  /* the empty part of a bar */
#include "ui_colors.h"                 /* BLK_COL: the blocks' colours (also the web editor's, px_editor.h) */

/* geometry (font heights: FT_SM 15, FT_MD 23, FT_NM 32, FT_XL 45 px) */
#define S_H      20u                   /* status bar */
#define P_NUM_Y  26u                   /* PLAY: preset number */
#define P_NAME_Y 76u                   /* PLAY: preset name */
#define T_Y0     122u                  /* PLAY: 1st row of tiles */
#define T_DY     30u
#define T_COLS   6
#define T_PITCH  39u
#define T_W      35u
#define T_H      24u
#define P_INFO_Y 186u                  /* PLAY: the selected block */
#define F_Y      222u                  /* footer (both screens) */
#define E_BAND   30u                   /* EDIT: block band */
#define E_TYPE_Y 36u                   /* EDIT: model row */
#define E_P0     70u                   /* EDIT: 1st setting */
#define E_ROW    30u
#define BAR_X    10u
#define BAR_W    220u
#define ST_X     164u                  /* the load figure's box: top right of PLAY, bottom right of a block page */
#define ST_W     72u                   /* (wide enough for "SAFE 999%", 70 px, and "PEAK 999%", 71 px) */
#define C_ALERT  0xB000                /* the overload banner: white text on it reads (contrast 7.1) */

static void num3(char *b, uint32_t v)  /* number without leading zeros */
{
    int i = 0;
    if (v >= 100u) b[i++] = (char)('0' + v / 100u);
    if (v >= 10u) b[i++] = (char)('0' + (v / 10u) % 10u);
    b[i++] = (char)('0' + v % 10u); b[i] = 0;
}
static uint16_t ui_dark(uint16_t c)    /* a colour at ~1/3: the "off" version of a block colour */
{
    return (uint16_t)((((c >> 11) & 31u) / 3u) << 11 | (((c >> 5) & 63u) / 3u) << 5 | ((c & 31u) / 3u));
}

/* what the load figure stands for: the overload banner (BYPASS), LOADING a NAM model (amber on the MODEL row, the amp name
 * greyed on PLAY), a late block (the figure in red), little headroom left (v42: the figure in amber) */
enum { UI_ST_NORM, UI_ST_LATE, UI_ST_BYPASS, UI_ST_LOADING, UI_ST_HIGH };
#define UI_LOAD_HIGH 80u               /* above this load the figure turns amber */

/* ---- cache of what is on screen ---- */
static struct {
    int full, screen;                  /* screen: 0 PLAY, 1 EDIT, -1 another page was drawn over it */
    uint32_t preset, vol; char name[14];
    int dirty, amp_t, cab_t, amp_on, cab_on, amp_m, cab_m;
    uint8_t tile_on[PX_NBLK], tile_sel[PX_NBLK], tile_blk[PX_NBLK], tile_mv[PX_NBLK];
    int info_b, info_t, info_on, info_m;
    int blk, type, np, item, mode, on;
    uint8_t val[PX_NPAR + 1];
    int hint, amp_ld, row_ld; uint32_t bnum;
    char status[12]; int status_kind;
} uc;
static void ui_invalidate(void) { uc.full = 1; }
/* OpenPocket Bass: a block of one type (AMP, CAB: px_type_fixed) has no model row: its first setting (the capture, the
 * IR) takes that place and item 0, the others follow as items 2..; the AMP has a 6th setting, LEVEL (PX_AMP_LVL_B) */
#define UI_MAXROW (PX_NPAR + 1)
static int ui_fixed(void) { return px_type_fixed((int)ui_blk) >= 0; }
static int ui_np(void) { return px_nparams((int)ui_blk, PRESET.type[ui_blk]) + (PX_BASS_UI && ui_blk == PX_B_AMP ? 1 : 0); }
static int ui_nitems(void) { return (ui_fixed() ? 1 : 2) + ui_np(); }   /* 0 = type, 1 = on/off, 2.. = parameters */
static int ui_k_of(uint32_t it) { return ui_fixed() ? (it == 0u ? 0 : it >= 2u ? (int)it - 1 : -1) : it >= 2u ? (int)it - 2 : -1; }
static int ui_it_of(int k) { return ui_fixed() ? (k == 0 ? 0 : k + 1) : k + 2; }
static uint32_t ui_row_y(int k) { return (ui_fixed() ? E_TYPE_Y - 2u : E_P0) + (uint32_t)k * E_ROW; }
static uint8_t *ui_pv(int k) { return k >= PX_NPAR ? &PRESET.p[PX_AMP_LVL_B][PX_AMP_LVL_K] : &PRESET.p[ui_blk][k]; }
static const char *ui_pname(int k) { return k >= PX_NPAR ? "LEVEL" : px_param_name((int)ui_blk, PRESET.type[ui_blk], k); }
static int ui_amp_band(void)           /* the AMP band (0 BASS, 1 MID, 2 TREBLE) the page is on, -1: none */
{
    const int k = ui_k_of(ui_item);
    return PX_BASS_UI && ui_blk == PX_B_AMP && k >= 2 && k <= 4 ? k - 2 : -1;
}
static const char *ui_block_name(int b) { return b == PX_B_SYN ? "SYNTH" : px_block_name(b); }   /* (the documents' name) */

/* v42: a setting that picks from a list (NAM MODEL, USER IR) steps one item a detent, skips the empty slots and shows its
 * place in the list. The stored value stays 0..100 (px_nam_knob, px_ir_knob): presets do not change. */
static int ui_list_n(int b, int t, int k)              /* the list's length, 0: not a list */
{
    if (k != 0) return 0;
    return b == PX_B_AMP && t == PX_AMP_NAM ? PX_NAM_N : b == PX_B_CAB && t == PX_CAB_IR ? PX_IR_N : 0;
}
static int ui_list_of(int b, int v) { return b == PX_B_AMP ? px_nam_model(v) : px_ir_slot(v); }
static int ui_list_val(int b, int i) { return b == PX_B_AMP ? px_nam_knob(i) : px_ir_knob(i); }
static int ui_list_name(int b, int i, char *nm12) { return b == PX_B_AMP ? px_nam_name(i, nm12) : px_ir_name(i, nm12); }
static int ui_list_place(int b, int n, int cur, int *count)   /* cur's place among the filled items (1..), 0: an empty one */
{
    char nm[12];
    int at = 0;
    *count = 0;
    for (int i = 0; i < n; i++) if (ui_list_name(b, i, nm)) { ++*count; if (i == cur) at = *count; }
    return at;
}
static void ui_order_fix(void) { if (!px_order_valid(PRESET.order)) px_order_default(PRESET.order); }
static int ui_pos_of(uint32_t b) { for (int i = 0; i < PX_NBLK; i++) if (PRESET.order[i] == b) return i; return 0; }
/* OpenPocket Bass: the blocks shown (px_block_used), in the preset's order: how many, into v */
static int ui_vis(uint8_t *v) { int n = 0; for (int i = 0; i < PX_NBLK; i++) if (px_block_used(PRESET.order[i])) v[n++] = PRESET.order[i]; return n; }
static int ui_vpos_of(const uint8_t *v, int n, uint32_t b) { for (int i = 0; i < n; i++) if (v[i] == b) return i; return 0; }

static int bat_percent(int mv)         /* a Li-ion cell at rest: 4.15 V full, 3.30 V empty */
{
    static const int16_t V[12] = {3300, 3500, 3630, 3690, 3720, 3750, 3790, 3840, 3900, 3970, 4050, 4150};
    static const int8_t P[12] = {0, 5, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
    if (mv <= V[0]) return 0;
    for (int i = 1; i < 12; i++) if (mv < V[i]) return P[i - 1] + (P[i] - P[i - 1]) * (mv - V[i - 1]) / (V[i] - V[i - 1]);
    return 100;
}
/* battery, top centre of PLAY: a cell icon + "78%" (green / amber / red), "USB" when the 5 V supply is there, an empty cell and
 * "--" while there is no reading (v37: shown, not hidden: the raw readings are on the tuner page, bat_raw) */
static void draw_battery(int force)
{
    static int last = -100;
    int usb = bat_usb_mv > 4300, pct = bat_pct >= 0 ? bat_pct : bat_mv ? bat_percent(bat_mv) : -1, key = usb ? 1000 + (pct < 0 ? 0 : pct) : pct;
    if (!force && key == last) return;
    last = key;
    const uint32_t x = 84u, y = 4u;                    /* icon 22 x 12 + tip, then the text */
    uint16_t c = usb ? GRN : pct <= 15 ? RED : pct <= 30 ? YEL : GRN;
    lcd_rect(x, 0, 74, y, UI_PANEL); lcd_rect(x, y + 12u, 74, S_H - y - 12u, UI_PANEL);
    lcd_rect(x, y, 22, 1, UI_TXT); lcd_rect(x, y + 11u, 22, 1, UI_TXT);
    lcd_rect(x, y + 1u, 1, 10, UI_TXT); lcd_rect(x + 21u, y + 1u, 1, 10, UI_TXT);
    lcd_rect(x + 22u, y + 3u, 2, 6, UI_TXT); lcd_rect(x + 22u, y, 2, 3, UI_PANEL); lcd_rect(x + 22u, y + 9u, 2, 3, UI_PANEL);
    uint32_t f = pct < 0 ? (usb ? 18u : 0u) : (uint32_t)pct * 18u / 100u;
    if (f) lcd_rect(x + 2u, y + 2u, f, 8, c);
    if (f < 18u) lcd_rect(x + 2u + f, y + 2u, 18u - f, 8, UI_PANEL);
    lcd_rect(x + 1u, y + 1u, 1, 10, UI_PANEL); lcd_rect(x + 20u, y + 1u, 1, 10, UI_PANEL);
    lcd_rect(x + 2u, y + 1u, 18, 1, UI_PANEL); lcd_rect(x + 2u, y + 10u, 18, 1, UI_PANEL);
    char b[8] = "USB";
    if (!usb && pct < 0) { b[0] = '-'; b[1] = '-'; b[2] = 0; }
    else if (!usb) { num3(b, (uint32_t)pct); int i = 0; while (b[i]) i++; b[i] = '%'; b[i + 1] = 0; }
    lcd_rect(x + 24u, y, 4, 12, UI_PANEL);
    ft_box(x + 28u, 3, 46, b, &FT_SM, usb ? GRN : pct < 0 ? UI_DIM : (pct <= 15 ? RED : UI_TXT), UI_PANEL, 0);
}

/* ---- status bar / footer, shared ---- */
static px_preset_t ui_ref;             /* what the current slot holds */
static int ui_dirty(void)              /* the preset differs from what its slot holds (hold PUSH to save) */
{
    static uint32_t ref_slot = ~0u, ref_gen = ~0u;
    if (ref_slot != cur_preset || ref_gen != st_gen) { st_get(cur_preset, &ui_ref); ref_slot = cur_preset; ref_gen = st_gen; }
    return memcmp(&ui_ref, &PRESET, sizeof ui_ref) != 0;
}
static void draw_status(int force)
{
    if (uc.screen != 0) return;        /* (EDIT: its band takes the top) */
    if (uc.status_kind == UI_ST_BYPASS) {                /* overload: the whole bar is the banner */
        if (force) { lcd_rect(0, 0, 240, 3, C_ALERT); ft_box(0, 3, 240, "OVERLOAD: SOUND BYPASSED", &FT_SM, WHT, C_ALERT, 1); lcd_rect(0, 18, 240, S_H - 18u, C_ALERT); }
        return;
    }
    if (force || uc.vol != master_vol) {
        char b[10] = "VOL "; num3(b + 4, master_vol);
        ft_box(0, 3, 66, b, &FT_SM, UI_DIM, UI_PANEL, 1);     /* (v36: 66 px; v35's 48 px cut "VOL 100" to "VOL 10") */
        uc.vol = master_vol;
    }
    lcd_rect(66, 0, 18, S_H, UI_PANEL); lcd_rect(158, 0, ST_X - 158u, S_H, UI_PANEL);
    draw_battery(force);
}
/* the DSP load, from the main loop: "DSP 42%", "DSP 88%!" (a block ran late), "SAFE 42%", "PEAK 150%" (bypassed). v42: which
 * cores ran is on the diagnostics page (ui_dg). Every text fits ST_W (tests/uisim.c checks the worst ones). */
static struct {                        /* the main loop's last DSP figures, for the diagnostics page */
    uint32_t load, peak, cl[2];        /* the load (the peak while bypassed), the peak since the preset, each core's (layer split) */
    uint8_t cores, split, guard, late, bypass;   /* cores: 0 C0, 1 C1, 2 both ("2C"); guard: the safety bypass's limit, % */
} ui_dg = {.guard = 85};
static void ui_load_text(char *c, uint32_t l, int safe, int bypass, int late)
{
    if (l > 999u) l = 999u;
    c[0] = 0;
    b_str(c, safe ? "SAFE " : bypass ? "PEAK " : "DSP ");
    b_num(c, l); b_str(c, "%");
    if (!safe && !bypass && late) b_str(c, "!");
}
static int ui_load_kind(uint32_t l, int safe, int bypass, int late, int loading)
{
    return loading ? UI_ST_LOADING : bypass ? UI_ST_BYPASS : late ? UI_ST_LATE : !safe && l > UI_LOAD_HIGH ? UI_ST_HIGH : UI_ST_NORM;
}
static void ui_status(const char *s, int kind)
{
    if (ui_mode == M_TUNER || ui_mode == M_SAVE || ui_mode == M_MIDI || ui_mode == M_LIST || ui_mode == M_DIAG || ui_mode == M_NAME) return;
    int same = uc.status_kind == kind;
    for (int i = 0; i < 12; i++) { if (uc.status[i] != s[i]) same = 0; if (!s[i]) break; }
    if (same && !uc.full) return;
    int was = uc.status_kind;
    for (int i = 0; i < 12; i++) { uc.status[i] = s[i]; if (!s[i]) break; }
    uc.status[11] = 0;
    uc.status_kind = kind;
    if ((was == UI_ST_BYPASS) != (kind == UI_ST_BYPASS)) { uc.full = 1; return; }   /* the banner comes or goes: redraw the page */
    if (kind == UI_ST_BYPASS) { uc.hint = -1; return; }                              /* (on PLAY the footer shows the peak; a block page's red footer has the instruction alone) */
    int ld = kind == UI_ST_LOADING && uc.screen == 0;     /* on a block page the MODEL row says it instead */
    ft_box(ST_X, uc.screen ? F_Y + 3u : 3u, ST_W, ld ? "LOADING" : s, &FT_SM, kind == UI_ST_LATE ? RED : ld || kind == UI_ST_HIGH ? UI_ACC : UI_DIM, UI_PANEL, 2);
}
static void draw_hint(int force)
{
    int h = (int)ui_mode, alert = uc.status_kind == UI_ST_BYPASS;
    int save = !alert && h == M_CHAIN && uc.dirty;       /* EDITED shows on PLAY: the footer names SAVE */
    int key = h | alert << 4 | save << 5 | (ui_amp_band() >= 0) << 6 | ui_sub << 7;
    if (!force && uc.hint == key) return;
    uc.hint = key;
    if (alert) {                                         /* overload: what to do (a block page's whole footer turns red) */
        char t[40]; t[0] = 0;
        b_str(t, uc.screen ? "OVERLOAD: TURN A BLOCK OFF" : "TURN A BLOCK OFF  ");
        if (!uc.screen) b_str(t, uc.status);
        if (uc.screen) lcd_rect(0, F_Y, 240, 3, C_ALERT);
        ft_box(0, F_Y + 3u, 240, t, &FT_SM, WHT, uc.screen ? C_ALERT : UI_PANEL, 1);
        return;
    }
    if (save) {
        ft_box(0, F_Y + 3u, 116, "PUSH: EDIT", &FT_SM, UI_DIM, UI_PANEL, 2);
        ft_box(116, F_Y + 3u, 124, "  HOLD: SAVE", &FT_SM, UI_ACC, UI_PANEL, 0);
        return;
    }
    const char *t = h == M_EDIT && ui_amp_band() >= 0 ? (ui_sub ? "KNOB: GAIN  PUSH: OK" : "KNOB: FREQ  PUSH: GAIN")
                  : h == M_PVOL ? "KNOB: PRESET VOL   PUSH: OK"
                  : h == M_MOVE ? "KNOB: MOVE    PUSH: DROP"
                  : h == M_EDIT ? "KNOB: VALUE   PUSH: OK"
                  : h == M_BLOCK ? "KNOB: SELECT  PUSH: EDIT"
                  : "KNOB: BLOCK  PUSH: EDIT  HOME: LIST";
    ft_box(0, F_Y + 3u, uc.screen ? ST_X - 2u : 240u, t, &FT_SM, UI_DIM, UI_PANEL, uc.screen ? 0 : 1);
}

/* ===================================================== PLAY ======== */
static void draw_play_top(int force)
{
    if (force || uc.preset != cur_preset) {
        char b[4]; uint32_t n = cur_preset + 1u;
        b[0] = (char)('0' + n / 10u); b[1] = (char)('0' + n % 10u); b[2] = 0;
        ft_box(6, P_NUM_Y, 84, b, &FT_XL, UI_ACC, UI_BG, 0);
        uc.preset = cur_preset;
    }
    int at = PRESET.type[PX_B_AMP], ct = PRESET.type[PX_B_CAB], ao = PRESET.on[PX_B_AMP], co = PRESET.on[PX_B_CAB];
    int am = at == PX_AMP_NAM ? px_nam_model(PRESET.p[PX_B_AMP][0]) : -1;   /* v37: a NAM amp shows its model */
    int ald = uc.status_kind == UI_ST_LOADING && at == PX_AMP_NAM;   /* a NAM model loads: the name greys until it plays */
    if (force || at != uc.amp_t || ao != uc.amp_on || am != uc.amp_m || ald != uc.amp_ld) {
        char nm[12];
        const char *an = px_type_name(PX_B_AMP, at);
        if (am >= 0) { px_nam_name(am, nm); an = nm; }
        ft_box(94, P_NUM_Y + 4u, 142, ao ? an : "NO AMP", &FT_MD, ao && !ald ? BLK_COL[PX_B_AMP] : UI_DIM, UI_BG, 0);
        uc.amp_t = at; uc.amp_on = ao; uc.amp_m = am; uc.amp_ld = ald;
    }
    int cm = ct == PX_CAB_IR ? px_ir_slot(PRESET.p[PX_B_CAB][0]) : -1;   /* v38: a user IR shows its name */
    if (force || ct != uc.cab_t || co != uc.cab_on || cm != uc.cab_m) {
        char nm[12];
        const char *cn = px_type_name(PX_B_CAB, ct);
        if (cm >= 0) { px_ir_name(cm, nm); cn = nm; }
        ft_box(94, P_NUM_Y + 29u, 88, co ? cn : "NO CAB", &FT_SM, UI_DIM, UI_BG, 0);
        uc.cab_t = ct; uc.cab_on = co; uc.cab_m = cm;
    }
    int d = ui_dirty();                                /* EDITED: the sound differs from its saved slot */
    if (force || d != uc.dirty) { ft_box(182, P_NUM_Y + 29u, 54, d ? "EDITED" : "", &FT_SM, UI_ACC, UI_BG, 2); uc.dirty = d; }
    int nchg = force;
    for (int i = 0; i < 14; i++) if (uc.name[i] != PRESET.name[i]) nchg = 1;
    if (nchg) {
        ft_box(0, P_NAME_Y, 240, PRESET.name, &FT_NM, UI_TXT, UI_BG, 1);
        for (int i = 0; i < 14; i++) uc.name[i] = PRESET.name[i];
    }
}
/* ---- chain tiles: position i shows block PRESET.order[i] ---- */
static uint32_t tile_x(int i) { return 4u + (uint32_t)(i % T_COLS) * T_PITCH; }
static uint32_t tile_y(int i) { return T_Y0 + (uint32_t)(i / T_COLS) * T_DY; }
static void draw_tile(int i, int b)
{
    uint32_t x = tile_x(i), y = tile_y(i);
    int on = PRESET.on[b] != 0, sel = (b == (int)ui_blk), mv = sel && ui_mode == M_MOVE;
    uint16_t bg = mv ? UI_ACC : (on ? BLK_COL[b] : C_OFF), fg = (on || mv) ? BLK : ft_mix(BLK_COL[b], C_OFF, 11);   /* (an off label: the block's colour at 11/15, contrast 2.6 to 7.2) */
    ui_rbox(x - 2u, y - 2u, T_W + 4u, T_H + 4u, 5, sel ? WHT : UI_BG, UI_BG);   /* selection ring (or nothing) */
    ui_rbox(x, y, T_W, T_H, 4, bg, sel ? WHT : UI_BG);
    ft_box(x + 2u, y + 5u, T_W - 4u, px_block_label(b), &FT_SM, fg, bg, 1);
    uc.tile_on[i] = (uint8_t)on; uc.tile_sel[i] = (uint8_t)sel; uc.tile_blk[i] = (uint8_t)b; uc.tile_mv[i] = (uint8_t)mv;
}
static void draw_tiles(int force)
{
    uint8_t v[PX_NBLK];
    ui_order_fix();
    const int n = ui_vis(v);
    for (int i = 0; i < PX_NBLK; i++) {
        if (i >= n) {                                  /* (OpenPocket Bass: fewer blocks, the rest of the grid empty) */
            if (force || uc.tile_blk[i] != 0xFFu) { ui_rbox(tile_x(i) - 2u, tile_y(i) - 2u, T_W + 4u, T_H + 4u, 5, UI_BG, UI_BG); uc.tile_blk[i] = 0xFFu; }
            continue;
        }
        int b = v[i], on = PRESET.on[b] != 0, sel = (b == (int)ui_blk), mv = sel && ui_mode == M_MOVE;
        if (force || uc.tile_blk[i] != b || uc.tile_on[i] != on || uc.tile_sel[i] != sel || uc.tile_mv[i] != mv) draw_tile(i, b);
    }
}
static void draw_info(int force)       /* the selected block: label, model (v42: a NAM amp's model, a user IR's name), on/off */
{
    int b = (int)ui_blk, t = PRESET.type[b], on = PRESET.on[b] != 0;
    int m = ui_list_n(b, t, 0) ? ui_list_of(b, PRESET.p[b][0]) : -1;
    if (!force && uc.info_b == b && uc.info_t == t && uc.info_on == on && uc.info_m == m) return;
    char nm[12];
    const char *tn = px_type_name(b, t);
    if (m >= 0) { ui_list_name(b, m, nm); tn = nm; }
    ft_box(0, P_INFO_Y, 8, "", &FT_MD, UI_TXT, UI_BG, 0);
    ft_box(8, P_INFO_Y, 54, px_block_label(b), &FT_MD, BLK_COL[b], UI_BG, 0);
    ft_box(62, P_INFO_Y, 132, tn, &FT_MD, on ? UI_TXT : UI_DIM, UI_BG, 0);
    lcd_rect(194, P_INFO_Y, 40, 4, UI_BG); lcd_rect(194, P_INFO_Y + 19u, 40, 4, UI_BG);
    ft_box(194, P_INFO_Y + 4u, 40, on ? "ON" : "OFF", &FT_SM, on ? BLK_COL[b] : UI_DIM, UI_BG, 2);
    lcd_rect(234, P_INFO_Y, 6, 23, UI_BG);
    uc.info_b = b; uc.info_t = t; uc.info_on = on; uc.info_m = m;
}

#if PX_BASS
/* ---- OpenPocket Bass PLAY: a pedalboard. The preset big at the top; the chain as six pedals (3 x 2, in the preset's
 * order, joined by cables), each with its LED, its model (the NAM capture, the IR's name...) and its main knob as a bar
 * (the EQ: its three bands); the input level under them. Every pedal is redrawn only when what it shows changed. ----
 *  +------------------------------+
 *  | VOL 80      [=] 72%   DSP 42%|
 *  | 07  SVT CLASSIC        EDITED|
 *  |-------------------------------|
 *  | [GATE  o]>[OCTAVE o]>[COMP  o]|
 *  | [EQ    o]>[AMP   o]>[CAB   o]|
 *  | IN ==========-------------   |
 *  | KNOB: BLOCK  PUSH: EDIT ...  |
 *  +------------------------------+ */
#define BP_X0   3u
#define BP_Y0   84u
#define BP_W    74u
#define BP_H    56u
#define BP_DX   80u
#define BP_DY   62u
#define BP_MY   212u                   /* the input meter */
#define BP_MX   30u
#define BP_MW   204u
static uint32_t bp_key[6];             /* what each pedal shows (0: not drawn) */
static int bp_meter = -1;
static const char *bp_label(int b)
{
    return b == PX_B_GATE ? "GATE" : b == PX_B_PITCH ? "OCTAVE" : b == PX_B_COMP ? "COMP" : b == PX_B_EQ ? "EQ"
         : b == PX_B_AMP ? "AMP" : b == PX_B_CAB ? "CAB" : px_block_label(b);
}
static int bp_main(int b)              /* the setting a pedal's bar shows (-1: the EQ's bands) */
{
    return b == PX_B_PITCH ? 1 : b == PX_B_AMP ? 1 : b == PX_B_CAB ? 3 : b == PX_B_EQ ? -1 : 0;
}
static void bp_model(int b, char *nm12)
{
    int t = PRESET.type[b], m = ui_list_n(b, t, 0) ? ui_list_of(b, PRESET.p[b][0]) : -1;
    const char *tn = px_type_name(b, t);
    if (m >= 0 && ui_list_name(b, m, nm12)) return;
    if (m >= 0) tn = b == PX_B_AMP ? "EMPTY" : "NO IR";
    if (b == PX_B_GATE) tn = "NOISE";                  /* ("NOISE GATE" is cut at the pedal's width) */
    int i = 0;
    for (; i < 11 && tn[i]; i++) nm12[i] = tn[i];
    nm12[i] = 0;
}
static void bp_pedal(int i, int b)
{
    const uint32_t x = BP_X0 + (uint32_t)(i % 3) * BP_DX, y = BP_Y0 + (uint32_t)(i / 3) * BP_DY;
    const int on = PRESET.on[b] != 0, sel = b == (int)ui_blk && !ui_pvol, mv = sel && ui_mode == M_MOVE, k = bp_main(b);
    const uint16_t col = BLK_COL[b], body = mv ? ui_dark(UI_ACC) : on ? ui_dark(col) : C_OFF;
    char nm[12];
    bp_model(b, nm);
    uint32_t key = 2166136261u;
    const uint8_t *q = PRESET.p[b];
    key = (key ^ (uint32_t)b) * 16777619u; key = (key ^ (uint32_t)(on | sel << 1 | mv << 2)) * 16777619u;
    key = (key ^ PRESET.type[b]) * 16777619u;
    for (int j = 0; j < PX_NPAR; j++) key = (key ^ q[j]) * 16777619u;
    for (int j = 0; nm[j]; j++) key = (key ^ (uint8_t)nm[j]) * 16777619u;
    if (!key) key = 1;
    if (bp_key[i] == key) return;
    bp_key[i] = key;
    ui_rbox(x - 2u, y - 2u, BP_W + 4u, BP_H + 4u, 6, sel ? WHT : UI_BG, UI_BG);   /* the selection ring (or nothing) */
    ui_rbox(x, y, BP_W, BP_H, 6, body, sel ? WHT : UI_BG);
    lcd_rect(x + 6u, y + 3u, BP_W - 12u, 2, on || mv ? col : ft_mix(col, C_OFF, 6));   /* the pedal's colour strip */
    ft_box(x + 6u, y + 7u, BP_W - 22u, bp_label(b), &FT_SM, on || mv ? WHT : ft_mix(col, C_OFF, 11), body, 0);
    ui_rbox(x + BP_W - 14u, y + 10u, 8, 8, 4, on ? col : ft_mix(UI_DIM, C_OFF, 6), body);   /* the LED */
    ft_box(x + 6u, y + 23u, BP_W - 10u, nm, &FT_SM, on ? UI_TXT : UI_DIM, body, 0);
    const uint32_t bx = x + 6u, bw = BP_W - 12u, by = y + 42u;
    const uint16_t fill = on ? col : ft_mix(col, C_OFF, 7), rest = ft_mix(C_BAR, body, 10);
    if (k >= 0) {                                      /* the main knob as a bar */
        uint32_t f = (uint32_t)q[k] * bw / 100u;
        lcd_rect(bx, by, bw, 3, body); lcd_rect(bx, by + 8u, bw, 3, body);
        if (f) lcd_rect(bx, by + 3u, f, 5, fill);
        if (f < bw) lcd_rect(bx + f, by + 3u, bw - f, 5, rest);
    } else {                                           /* the EQ: LOW, MID (its gain), HIGH around the middle line */
        static const uint8_t BAND[3] = {0, 2, 4};
        lcd_rect(bx, by, bw, 11, body);
        for (int j = 0; j < 3; j++) {
            const uint32_t cx = bx + (uint32_t)j * 22u, cw = 18u;
            int d = ((int)q[BAND[j]] - 50) * 5 / 50;   /* -5..+5 px */
            lcd_rect(cx, by + 5u, cw, 1, rest);
            if (d > 0) lcd_rect(cx, by + 5u - (uint32_t)d, cw, (uint32_t)d, fill);
            if (d < 0) lcd_rect(cx, by + 6u, cw, (uint32_t)-d, fill);
        }
    }
}
static void bp_cables(void)            /* the cables between the pedals of a row, and down from the row's end */
{
    for (int r = 0; r < 2; r++)
        for (int c = 0; c < 2; c++) {
            const uint32_t gx = BP_X0 + (uint32_t)c * BP_DX + BP_W + 2u, gy = BP_Y0 + (uint32_t)r * BP_DY + BP_H / 2u;
            lcd_rect(gx, gy - 1u, 2, 3, C_LINE);
        }
}
static void bp_play_top(int force)
{
    if (force || uc.preset != cur_preset) {
        char b[4]; uint32_t n = cur_preset + 1u;
        b[0] = (char)('0' + n / 10u); b[1] = (char)('0' + n % 10u); b[2] = 0;
        ft_box(6, 26, 80, b, &FT_XL, UI_ACC, UI_BG, 0);
        uc.preset = cur_preset;
    }
    int nchg = force;
    for (int i = 0; i < 14; i++) if (uc.name[i] != PRESET.name[i]) nchg = 1;
    if (nchg) {
        ft_box(90, 28, 146, PRESET.name, &FT_MD, UI_TXT, UI_BG, 0);
        for (int i = 0; i < 14; i++) uc.name[i] = PRESET.name[i];
    }
    int d = ui_dirty();
    if (force || d != uc.dirty) { ft_box(90, 54, 70, d ? "EDITED" : "", &FT_SM, UI_ACC, UI_BG, 0); uc.dirty = d; }
    /* the preset's volume (the GT page's PRESET LVL): the knob stops on it before the pedals, PUSH adjusts it */
    static int pv_key = -1;
    const int lv = PRESET.p[PX_B_GATE][PX_PATCH_LEVEL], ed = ui_mode == M_PVOL, sel = ui_pvol || ed;
    const int key = lv | sel << 8 | ed << 9;
    if (force || key != pv_key) {
        char b[16] = "VOL ";
        const int t = (lv - 50) * 3;                   /* tenths of a dB */
        b_str(b, t < 0 ? "-" : t > 0 ? "+" : ""); b_num(b, (uint32_t)(t < 0 ? -t : t) / 10u);
        b_str(b, "."); b_num(b, (uint32_t)(t < 0 ? -t : t) % 10u);
        const uint16_t bg = ed ? UI_ACC : sel ? C_CARD : UI_BG;
        ui_rbox(160, 50, 78, 22, 5, sel ? (ed ? UI_ACC : WHT) : UI_BG, UI_BG);   /* the ring when the knob is on it */
        ui_rbox(162, 52, 74, 18, 4, bg, sel ? (ed ? UI_ACC : WHT) : UI_BG);
        ft_box(165, 53, 68, b, &FT_SM, ed ? BLK : sel ? WHT : UI_DIM, bg, 1);
        pv_key = key;
    }
}
static void bp_pedals(int force)
{
    uint8_t v[PX_NBLK];
    ui_order_fix();
    const int n = ui_vis(v);
    if (force) { for (int i = 0; i < 6; i++) bp_key[i] = 0; bp_cables(); }
    for (int i = 0; i < 6; i++) {
        if (i < n) bp_pedal(i, v[i]);
        else if (bp_key[i] != 1u) {                    /* (fewer blocks: the place stays empty) */
            const uint32_t x = BP_X0 + (uint32_t)(i % 3) * BP_DX, y = BP_Y0 + (uint32_t)(i / 3) * BP_DY;
            lcd_rect(x - 2u, y - 2u, BP_W + 4u, BP_H + 4u, UI_BG); bp_key[i] = 1u;
        }
    }
}
static void bp_input(int force)        /* the input level, -60..0 dBFS: green, amber from -12, red from -3 */
{
    int db = a_level_db(0), d1 = a_level_db(1);
    if (d1 > db) db = d1;
    if (db < -60) db = -60;
    if (db > 0) db = 0;
    int w = (db + 60) * (int)BP_MW / 60;
    w -= w % 4;                                        /* (in steps of 4 px: the bar does not flicker on noise) */
    if (!force && w == bp_meter) return;
    if (force) { ft_box(4, BP_MY - 5u, 26, "IN", &FT_SM, UI_DIM, UI_BG, 0); }
    const int g = 48 * (int)BP_MW / 60, a = 57 * (int)BP_MW / 60;   /* -12 and -3 dB */
    for (int s = 0; s < (int)BP_MW; s += 4) {
        if (!force && bp_meter >= 0 && (s < (w < bp_meter ? w : bp_meter) || s >= (w > bp_meter ? w : bp_meter))) continue;
        const uint16_t c = s < g ? GRN : s < a ? YEL : RED;
        lcd_rect(BP_MX + (uint32_t)s, BP_MY, 3, 6, s < w ? c : ft_mix(c, UI_BG, 3));
    }
    bp_meter = w;
}
#endif

/* ===================================================== EDIT ======== */
static void draw_band(int force)       /* the block's name on its colour, ON / OFF pill (item 1) */
{
    int on = PRESET.on[ui_blk] != 0, sel = (UI_PAGE && ui_item == 1u);
    if (!(force || uc.blk != (int)ui_blk || uc.bnum != cur_preset || uc.on != on || (uc.item == 1) != sel)) return;
    uint16_t c = BLK_COL[ui_blk];
    if (force || uc.blk != (int)ui_blk || uc.bnum != cur_preset) {   /* (the preset number: a change over MIDI does not leave the page) */
        char n[4]; num3(n, cur_preset + 1u);
        lcd_rect(0, 0, 240, 4, c); lcd_rect(0, 27, 240, E_BAND - 27u, c); lcd_rect(0, 4, 8, 23, c);
        lcd_rect(8, 4, 24, 23, c); ui_rbox(8, 6, 24, 19, 5, BLK, c);
        ft_box(10, 8, 20, n, &FT_SM, c, BLK, 1);
        lcd_rect(32, 4, 4, 23, c);
        ft_box(36, 4, 136, ui_block_name((int)ui_blk), &FT_MD, BLK, c, 0);
        lcd_rect(172, 4, 4, 23, c); lcd_rect(232, 4, 8, 23, c);
        uc.bnum = cur_preset;
    }
    ui_rbox(176, 4, 56, 23, 6, sel ? WHT : c, c);
    ui_rbox(178, 6, 52, 19, 5, on ? BLK : ui_dark(c), sel ? WHT : c);
    ft_box(182, 8, 44, on ? "ON" : "OFF", &FT_SM, on ? c : UI_TXT, on ? BLK : ui_dark(c), 1);
}
static void draw_type(void)
{
    int sel = (UI_PAGE && ui_item == 0u), ed = sel && ui_mode == M_EDIT, many = px_ntypes((int)ui_blk) > 1 && px_type_fixed((int)ui_blk) < 0;
    uint16_t bg = sel ? C_CARD : UI_BG, fg = ed ? UI_ACC : UI_TXT;
    ui_rbox(4, E_TYPE_Y - 2u, 232, 28, 5, bg, UI_BG);
    ft_box(8, E_TYPE_Y, 20, sel && many ? "<" : "", &FT_MD, UI_ACC, bg, 1);   /* (v42: no arrows on a block of one model) */
    ft_box(28, E_TYPE_Y, 184, px_type_name((int)ui_blk, PRESET.type[ui_blk]), &FT_MD, fg, bg, 1);
    ft_box(212, E_TYPE_Y, 20, sel && many ? ">" : "", &FT_MD, UI_ACC, bg, 1);
}
static int ui_loading_row(int k)       /* the NAM amp's MODEL row while its model loads */
{
    return uc.status_kind == UI_ST_LOADING && ui_blk == PX_B_AMP && PRESET.type[PX_B_AMP] == PX_AMP_NAM && k == 0;
}
static void ui_hz_text(char *b, int off)   /* v42: USER IR's cut frequencies with their unit: "89 HZ", "6.3 KHZ", "OFF" */
{
    uint32_t hz = 0;
    for (int i = 0; b[i]; i++) hz = hz * 10u + (uint32_t)(b[i] - '0');
    b[0] = 0;
    if (off) { b_str(b, "OFF"); return; }
    if (hz < 1000u) { b_num(b, hz); b_str(b, " HZ"); return; }
    hz = (hz + 50u) / 100u;
    b_num(b, hz / 10u); b_str(b, "."); b_num(b, hz % 10u); b_str(b, " KHZ");
}
#define E_LIST_X 96u                   /* a list setting: its place at the left, the item's name from here */
static void draw_bar(int k)            /* value + bar: the only thing that moves while adjusting */
{
    uint32_t y = ui_row_y(k);
    int sel = (UI_PAGE && (int)ui_item == ui_it_of(k)), ed = sel && ui_mode == M_EDIT, t = PRESET.type[ui_blk];
    int ln = ui_list_n((int)ui_blk, t, k), ld = ui_loading_row(k);
    uint16_t bg = sel ? C_CARD : UI_BG, vc = ld ? UI_DIM : ed ? UI_ACC : UI_TXT;   /* (the new name stays grey until it plays) */
    uint16_t fc = ed ? UI_ACC : BLK_COL[ui_blk];
    uint32_t v = *ui_pv(k); if (v > 100u) v = 100u;
    char b[16];
    uc.val[k] = (uint8_t)v;
    if (ln) {                          /* "MODEL 6/14", the name, one segment per filled item */
        int cur = ui_list_of((int)ui_blk, (int)v), n, at = ui_list_place((int)ui_blk, ln, cur, &n);
        char nm[12];
        b[0] = 0; b_str(b, ui_pname(k)); b_str(b, " ");
        if (at) b_num(b, (uint32_t)at); else b_str(b, "-");
        b_str(b, "/"); b_num(b, (uint32_t)n);
        ft_box(BAR_X, y + 5u, E_LIST_X - BAR_X, ld ? "LOADING" : b, &FT_SM, ld ? UI_ACC : sel ? WHT : UI_DIM, bg, 0);
        ui_list_name((int)ui_blk, cur, nm);
        ft_box(E_LIST_X, y + 1u, 230u - E_LIST_X, nm, &FT_MD, vc, bg, 2);
        uint32_t sw = n ? BAR_W / (uint32_t)n : BAR_W;
        for (int i = 0; i < n; i++) {
            lcd_rect(BAR_X + (uint32_t)i * sw, y + 24u, sw - 1u, 4, i + 1 == at ? fc : C_BAR);
            lcd_rect(BAR_X + (uint32_t)i * sw + sw - 1u, y + 24u, 1, 4, bg);
        }
        if (sw * (uint32_t)n < BAR_W) lcd_rect(BAR_X + sw * (uint32_t)n, y + 24u, BAR_W - sw * (uint32_t)n, 4, n ? bg : C_BAR);
        return;
    }
    if (k >= PX_NPAR || !px_param_text((int)ui_blk, t, k, (int)v, b)) num3(b, v);   /* "+5TH", "440 MS"... */
    else if (ui_blk == PX_B_CAB && t == PX_CAB_IR && (k == 1 || k == 2)) ui_hz_text(b, k == 2 && v == 0u);
    else if ((ui_blk == PX_B_EQ && t == PX_EQ_BASS && k == 1) || (ui_blk == PX_B_MOD && t == PX_MOD_BASSCHO && k == 3) ||
             (ui_blk == PX_B_PITCH && t == PX_PIT_BASSOCT && k >= 3)) ui_hz_text(b, 0);   /* (OpenPocket Bass: in Hz) */
    ft_box(140, y + 1u, 90, b, &FT_MD, vc, bg, 2);
    uint32_t w = v * BAR_W / 100u;
    if (w) lcd_rect(BAR_X, y + 24u, w, 4, fc);
    if (w < BAR_W) lcd_rect(BAR_X + w, y + 24u, BAR_W - w, 4, C_BAR);
}
static void draw_row(int k)            /* one full setting row */
{
    uint32_t y = ui_row_y(k);
    if (k >= ui_np()) { if (y < F_Y) lcd_rect(0, y, 240, (y + E_ROW <= F_Y ? E_ROW : F_Y - y), UI_BG); return; }
    int sel = (UI_PAGE && (int)ui_item == ui_it_of(k));
    uint16_t bg = sel ? C_CARD : UI_BG;
    ui_rbox(4, y, 232, E_ROW - 1u, 5, bg, UI_BG);
    lcd_rect(0, y + E_ROW - 1u, 240, 1, UI_BG);
    if (sel) lcd_rect(4, y + 6u, 3, 14, UI_ACC);                 /* accent marker */
    int ld = ui_loading_row(k);
    if (!ui_list_n((int)ui_blk, PRESET.type[ui_blk], k))         /* (a list setting draws its label with its place) */
        ft_box(BAR_X, y + 5u, 130, ld ? "LOADING" : ui_pname(k), &FT_SM, ld ? UI_ACC : sel ? WHT : UI_DIM, bg, 0);
    if (k == 0) uc.row_ld = ld;
    draw_bar(k);
}
static void draw_item(int it)
{
    if (it == 1) draw_band(1);
    else if (ui_k_of((uint32_t)it) >= 0) draw_row(ui_k_of((uint32_t)it));
    else draw_type();
}
#if PX_BASS_UI
static void amp_page(int force);
static void eq_page(int force);
#endif
static void draw_edit(int force)
{
#if PX_BASS_UI
    if (ui_blk == PX_B_AMP) { amp_page(force); return; }
    if (ui_blk == PX_B_EQ) { eq_page(force); return; }
#endif
    int type = PRESET.type[ui_blk], np = ui_np(), on = PRESET.on[ui_blk] != 0;
    if (force || uc.blk != (int)ui_blk || uc.type != type) {
        draw_band(1);
        if (!ui_fixed()) { draw_type(); lcd_rect(0, E_TYPE_Y + 26u, 240, E_P0 - E_TYPE_Y - 26u, UI_BG); }   /* (else no model row) */
        for (int k = 0; k < UI_MAXROW; k++) draw_row(k);
        { const uint32_t end = ui_row_y(UI_MAXROW); if (end < F_Y) lcd_rect(0, end, 240, F_Y - end, UI_BG); }
        uc.blk = (int)ui_blk; uc.type = type; uc.np = np; uc.item = (int)ui_item; uc.mode = (int)ui_mode; uc.on = on;
        return;
    }
    if (uc.item != (int)ui_item || uc.mode != (int)ui_mode) {   /* highlight moved: 2 items only */
        int old = uc.item;
        uc.item = (int)ui_item; uc.mode = (int)ui_mode;
        if (old != (int)ui_item) draw_item(old);
        draw_item((int)ui_item);
    }
    if (uc.on != on || uc.bnum != cur_preset) { draw_band(1); uc.on = on; }
    if (np > 0 && ui_loading_row(0) != uc.row_ld) draw_row(0);
    for (int k = 0; k < np; k++) if (uc.val[k] != *ui_pv(k)) draw_bar(k);
}

#if PX_BASS_UI
/* ---- OpenPocket Bass: the AMP and EQ pages as graphs. One graph for both, 30 Hz..8 kHz (log) by +-12 dB, the curve
 * worked out from the filters the engine itself makes (px_tone_bq, px_bq_db), drawn in one pixel stream (no flicker),
 * its points on top; under it the frequencies, a line naming what the knob is on, then the AMP's GAIN and LEVEL knobs or
 * the EQ's five values. ---- */
#define GR_X  4u
#define GR_Y  64u
#define GR_W  232u
#define GR_H  80u
#define GR_PX0 10                      /* the plot's first and last columns (screen x) */
#define GR_PX1 229
#define GR_DB 3.0f                     /* px a dB (+-12 dB: +-36 px) */
#define GR_BG 0x0861
#define GR_INFO_Y 161u
#define GR_LOW_Y 177u                  /* the knobs or the values */
static float gr_hz(int x) { return 30.0f * powf(8000.0f / 30.0f, (float)(x - GR_PX0) / (float)(GR_PX1 - GR_PX0)); }
static int gr_x(float hz)
{
    int x = GR_PX0 + (int)((float)(GR_PX1 - GR_PX0) * logf(hz / 30.0f) / logf(8000.0f / 30.0f) + 0.5f);
    return x < GR_PX0 ? GR_PX0 : x > GR_PX1 ? GR_PX1 : x;
}
static int gr_yc(void) { return (int)(GR_Y + GR_H / 2u); }
static int gr_ydb(float db) { int y = gr_yc() - (int)(db * GR_DB + (db >= 0 ? 0.5f : -0.5f)); return y < (int)GR_Y + 1 ? (int)GR_Y + 1 : y > (int)(GR_Y + GR_H) - 2 ? (int)(GR_Y + GR_H) - 2 : y; }
static float gr_db(const px_bq *f, int n, float hz) { float d = 0.0f; for (int i = 0; i < n; i++) d += px_bq_db(&f[i], hz); return d; }
static void hz_text(char *b, float hz)  /* "80 HZ", "2.5 KHZ" */
{
    char t[12]; uint32_t v = (uint32_t)(hz + 0.5f), i = 0;
    if (v >= 1000u) { v = (v + 50u) / 100u; num3(t, v / 10u); while (t[i]) i++; if (v % 10u) { t[i++] = '.'; t[i++] = (char)('0' + v % 10u); } t[i] = 0; }
    else num3(t, v);
    b_str(b, t); b_str(b, v >= 1000u || hz >= 999.5f ? " KHZ" : " HZ");
}
static void db_text(char *b, int v)     /* a 0..100 gain as "+4 DB" */
{
    const int d = (v - 50) * 24 / 100;
    b_str(b, d > 0 ? "+" : d < 0 ? "-" : ""); b_num(b, (uint32_t)(d < 0 ? -d : d)); b_str(b, " DB");
}
/* the graph of block b's tone: points at hz[0..n), sel the one the knob is on (-1 none), ed: it is being changed; the
 * shaded band [alo, ahi] (the EQ's middle: its width), 0 = none */
static void graph_draw(int b, const float *hz, int n, int sel, int ed, float alo, float ahi)
{
    px_bq f[5];
    const int nf = px_tone_bq(b, &PRESET, f), yc = gr_yc();
    const uint16_t col = BLK_COL[b], shade = ft_mix(col, GR_BG, 4), grid = 0x2104, zero = 0x39E7, tick = 0x2945;
    static int16_t yv[240];
    for (int x = GR_PX0; x <= GR_PX1; x++) yv[x] = (int16_t)gr_ydb(gr_db(f, nf, gr_hz(x)));
    static const float TK[6] = {40.0f, 100.0f, 250.0f, 1000.0f, 2500.0f, 6000.0f};
    uint8_t vt[240] = {0};
    for (int i = 0; i < 6; i++) vt[gr_x(TK[i])] = 1;
    const int s0 = alo > 0.0f ? gr_x(alo) : 999, s1 = ahi > 0.0f ? gr_x(ahi) : -1;
    lcd_win(GR_X, GR_Y, GR_W, GR_H);
    for (int y = (int)GR_Y; y < (int)(GR_Y + GR_H); y++) {
        const int hl = y == yc ? 2 : (y == gr_ydb(6.0f) || y == gr_ydb(-6.0f) || y == gr_ydb(12.0f) || y == gr_ydb(-12.0f)) ? 1 : 0;
        for (int x = (int)GR_X; x < (int)(GR_X + GR_W); x++) {
            uint16_t c = GR_BG;
            if (x >= GR_PX0 && x <= GR_PX1) {
                const int y0 = yv[x], y1 = x > GR_PX0 ? yv[x - 1] : y0, lo = (y0 < y1 ? y0 : y1) - 1, hi = y0 > y1 ? y0 : y1;
                if (y >= lo && y <= hi) c = col;
                else if (x >= s0 && x <= s1 && ((y > y0 && y < yc) || (y < y0 && y > yc))) c = shade;
                else if (hl == 2) c = zero;
                else if (hl || vt[x]) c = vt[x] && hl ? tick : grid;
            }
            lcd_px(c);
        }
    }
    lcd_end();
    for (int i = 0; i < n; i++) {                      /* the points */
        const int x = gr_x(hz[i]), y = gr_ydb(gr_db(f, nf, hz[i]));
        if (i == sel) { ui_rbox((uint32_t)x - 5u, (uint32_t)y - 5u, 11, 11, 5, ed ? UI_ACC : WHT, GR_BG); ui_rbox((uint32_t)x - 3u, (uint32_t)y - 3u, 7, 7, 3, ed ? BLK : col, ed ? UI_ACC : WHT); }
        else ui_rbox((uint32_t)x - 3u, (uint32_t)y - 3u, 7, 7, 3, col, GR_BG);
    }
}
static void graph_ticks(void)          /* the frequencies under the graph (drawn with the page) */
{
    static const float TK[6] = {40.0f, 100.0f, 250.0f, 1000.0f, 2500.0f, 6000.0f};
    static const char *const TT[6] = {"40", "100", "250", "1K", "2.5K", "6K"};
    lcd_rect(0, GR_Y + GR_H, 240, 15, UI_BG);
    for (int i = 0; i < 6; i++) { const int x = gr_x(TK[i]); ft_box((uint32_t)(x - 16 < 0 ? 0 : x - 16), GR_Y + GR_H + 1u, 32, TT[i], &FT_SM, UI_DIM, UI_BG, 1); }
}
static float ui_atan2(float y, float x)   /* (no atan2 in the pedal's maths: within 0.01 rad) */
{
    const float ax = x < 0 ? -x : x, ay = y < 0 ? -y : y, a = (ax < ay ? ax : ay) / ((ax > ay ? ax : ay) + 1e-9f), s = a * a;
    float r = ((-0.0464964749f * s + 0.15931422f) * s - 0.327622764f) * s * a + a;
    if (ay > ax) r = 1.57079637f - r;
    if (x < 0) r = 3.14159274f - r;
    return y < 0 ? -r : r;
}
/* a round knob, 40 x 40 at (x, y): its arc (270 degrees from bottom left) filled to v, the value in its face */
static void knob_draw(uint32_t x0, uint32_t y0, int v, uint16_t col, uint16_t bg)
{
    const float av = (135.0f + 2.7f * (float)v) * 0.0174532925f, ca = cosf(av), sa = sinf(av);
    lcd_win(x0, y0, 40, 40);
    for (int j = 0; j < 40; j++) for (int i = 0; i < 40; i++) {
        const float dx = (float)i - 19.5f, dy = (float)j - 19.5f, d2 = dx * dx + dy * dy;
        uint16_t c = bg;
        if (d2 <= 19.5f * 19.5f && d2 >= 15.0f * 15.0f) {
            float t = ui_atan2(dy, dx) * 57.29578f - 135.0f;
            while (t < 0.0f) t += 360.0f;
            if (t <= 270.0f) c = t <= 2.7f * (float)v ? col : 0x3186;
        } else if (d2 <= 13.0f * 13.0f) {
            c = 0x18E3;
            const float along = dx * ca + dy * sa, across = dx * sa - dy * ca;
            if (along > 7.0f && across < 1.3f && across > -1.3f) c = WHT;
        }
        lcd_px(c);
    }
    lcd_end();
    char b[6]; num3(b, (uint32_t)v);
    ft_box(x0 + 11u, y0 + 13u, 18, b, &FT_SM, UI_TXT, 0x18E3, 1);
}
static uint32_t pg_key;                /* what the graph shows now (redrawn when it changes) */
static void amp_page(int force)
{
    const uint8_t *q = PRESET.p[PX_B_AMP], *f = PRESET.p[PX_AMP_LVL_B];
    const int on = PRESET.on[ui_blk] != 0, band = ui_amp_band(), ed = ui_mode == M_EDIT;
    if (force || uc.blk != (int)ui_blk) {
        draw_band(1);
        lcd_rect(0, E_BAND, 240, GR_Y - E_BAND, UI_BG);
        lcd_rect(0, GR_Y, GR_X, GR_H, UI_BG); lcd_rect(GR_X + GR_W, GR_Y, 240u - GR_X - GR_W, GR_H, UI_BG);
        lcd_rect(0, GR_INFO_Y - 2u, 240, F_Y - GR_INFO_Y + 2u, UI_BG);
        graph_ticks();
        uc.blk = (int)ui_blk; uc.type = PRESET.type[ui_blk]; uc.item = -1; uc.mode = -1; uc.on = on; pg_key = 0;
        for (int k = 0; k < PX_NPAR + 1; k++) uc.val[k] = 0xFF;
        force = 1;
    }
    if (uc.on != on || uc.bnum != cur_preset || (uc.item == 1) != (ui_item == 1u)) { int was = uc.item; uc.item = (int)ui_item; draw_band(1); uc.item = was; uc.on = on; }
    const int msel = ui_item == 0u;                    /* MODEL: the list row (draw_row 0 at the top) */
    if (force || uc.val[0] != q[0] || (uc.item == 0) != msel || uc.mode != (int)ui_mode || ui_loading_row(0) != uc.row_ld) {
        const int ld = ui_loading_row(0);
        draw_row(0); uc.row_ld = ld;
    }
    uint32_t key = 2166136261u;
    const uint8_t kb[8] = {q[2], q[3], q[4], f[1], f[2], f[3], (uint8_t)(band + 1), (uint8_t)(ed * 2 + ui_sub)};
    for (int i = 0; i < 8; i++) key = (key ^ kb[i]) * 16777619u;
    if (force || key != pg_key) {
        float hz[3];
        for (int i = 0; i < 3; i++) hz[i] = px_amp_band_hz(i, f[1 + i]);
        graph_draw(PX_B_AMP, hz, 3, band, ed && band >= 0, 0.0f, 0.0f);
        char b[40] = "";
        if (band >= 0) {
            static const char *const BN[3] = {"BASS ", "MID ", "TREBLE "};
            b_str(b, BN[band]); hz_text(b, hz[band]); b_str(b, "   "); db_text(b, q[2 + band]);
        } else { hz_text(b, hz[0]); b_str(b, " / "); hz_text(b, hz[1]); b_str(b, " / "); hz_text(b, hz[2]); }
        ft_box(0, GR_INFO_Y, 240, b, &FT_SM, band >= 0 ? (ed ? UI_ACC : WHT) : UI_DIM, UI_BG, 1);
        pg_key = key;
    }
    for (int i = 0; i < 2; i++) {                      /* GAIN (k 1) and LEVEL (k 5) */
        const int k = i ? 5 : 1, v = *ui_pv(k), sel = (int)ui_item == ui_it_of(k), e = sel && ed;
        const int was = uc.item == ui_it_of(k);
        if (!force && uc.val[k] == v && sel == was && uc.mode == (int)ui_mode) continue;
        const uint32_t x = i ? 124u : 8u;
        const uint16_t bg = sel ? C_CARD : UI_BG;
        ui_rbox(x, GR_LOW_Y, 108, 44, 6, sel ? (e ? UI_ACC : WHT) : UI_BG, UI_BG);
        ui_rbox(x + 1u, GR_LOW_Y + 1u, 106, 42, 5, bg, sel ? (e ? UI_ACC : WHT) : UI_BG);
        knob_draw(x + 3u, GR_LOW_Y + 2u, v, e ? UI_ACC : BLK_COL[PX_B_AMP], bg);
        ft_box(x + 48u, GR_LOW_Y + 6u, 56, i ? "LEVEL" : "GAIN", &FT_SM, sel ? WHT : UI_DIM, bg, 0);
        char b[12] = "";
        if (i) { const int t = (v - 50) * 24; b_str(b, t > 0 ? "+" : t < 0 ? "-" : ""); b_num(b, (uint32_t)((t < 0 ? -t : t) / 100)); b_str(b, " DB"); }
        ft_box(x + 48u, GR_LOW_Y + 23u, 56, b, &FT_SM, UI_DIM, bg, 0);
        uc.val[k] = (uint8_t)v;
    }
    uc.item = (int)ui_item; uc.mode = (int)ui_mode;
}
/* the EQ: its points (BASS PEQ / PARAMETRIC: the low shelf, the middle, the high shelf; the others: their 5 bands) */
static int eq_marks(float *hz, float *alo, float *ahi)
{
    const int t = PRESET.type[PX_B_EQ] % PX_EQ_COUNT;
    const uint8_t *q = PRESET.p[PX_B_EQ];
    static const float B5[5] = {100.0f, 400.0f, 1000.0f, 2500.0f, 6000.0f}, BG[5] = {80.0f, 240.0f, 750.0f, 2200.0f, 6600.0f};
    *alo = *ahi = 0.0f;
    if (t == PX_EQ_5BAND || t == PX_EQ_GEQ) { for (int i = 0; i < 5; i++) hz[i] = t == PX_EQ_5BAND ? B5[i] : BG[i]; return 5; }
    const int bass = t == PX_EQ_BASS;
    const float x = (float)q[1] * 0.01f, fm = bass ? 40.0f * powf(100.0f, x) : 150.0f * powf(33.0f, x);
    const float qq = 0.4f + (float)q[3] * 0.01f * 5.6f, bw = 0.5f / qq;   /* the middle's -3 dB edges, about f * 2^(+-1/(2Q)) */
    hz[0] = bass ? 80.0f : 100.0f; hz[1] = fm; hz[2] = bass ? 3500.0f : 5000.0f;
    *alo = fm * powf(2.0f, -bw); *ahi = fm * powf(2.0f, bw);
    return 3;
}
static void eq_page(int force)
{
    const int t = PRESET.type[PX_B_EQ] % PX_EQ_COUNT, on = PRESET.on[ui_blk] != 0, ed = ui_mode == M_EDIT;
    const uint8_t *q = PRESET.p[PX_B_EQ];
    const int k = ui_k_of(ui_item), five = t == PX_EQ_5BAND || t == PX_EQ_GEQ;
    if (force || uc.blk != (int)ui_blk || uc.type != t) {
        draw_band(1);
        lcd_rect(0, E_BAND, 240, E_TYPE_Y - 2u - E_BAND, UI_BG);
        lcd_rect(0, E_TYPE_Y + 26u, 240, GR_Y - E_TYPE_Y - 26u, UI_BG);
        lcd_rect(0, GR_Y, GR_X, GR_H, UI_BG); lcd_rect(GR_X + GR_W, GR_Y, 240u - GR_X - GR_W, GR_H, UI_BG);
        lcd_rect(0, GR_INFO_Y - 2u, 240, F_Y - GR_INFO_Y + 2u, UI_BG);
        graph_ticks();
        uc.blk = (int)ui_blk; uc.type = t; uc.item = -1; uc.mode = -1; uc.on = on; pg_key = 0;
        for (int i = 0; i < PX_NPAR + 1; i++) uc.val[i] = 0xFF;
        force = 1;
    }
    if (uc.on != on || uc.bnum != cur_preset || (uc.item == 1) != (ui_item == 1u)) { int was = uc.item; uc.item = (int)ui_item; draw_band(1); uc.item = was; uc.on = on; }
    if (force || (uc.item == 0) != (ui_item == 0u) || (ui_item == 0u && uc.mode != (int)ui_mode)) draw_type();
    float hz[5], alo, ahi;
    const int n = eq_marks(hz, &alo, &ahi);
    const int sel = k < 0 ? -1 : five ? k : k == 0 ? 0 : k == 4 ? 2 : 1;
    uint32_t key = 2166136261u;
    for (int i = 0; i < PX_NPAR; i++) key = (key ^ q[i]) * 16777619u;
    key = (key ^ (uint32_t)(t + 8 * (sel + 1) + 64 * ed)) * 16777619u;
    if (force || key != pg_key) {
        graph_draw(PX_B_EQ, hz, n, sel, ed && sel >= 0, five ? 0.0f : alo, five ? 0.0f : ahi);
        char b[40] = "", v[12];
        if (!five && k >= 1 && k <= 3) {               /* the middle: where, how wide, how much */
            b_str(b, "MID "); hz_text(b, hz[1]); b_str(b, "  Q ");
            v[0] = 0; px_param_text(PX_B_EQ, t, 3, q[3], v); b_str(b, v); b_str(b, "  "); db_text(b, q[2]);
        } else if (k >= 0) {
            b_str(b, five ? "" : k == 0 ? "LOW " : "HIGH "); hz_text(b, hz[sel]); b_str(b, "  "); db_text(b, q[k]);
        }
        ft_box(0, GR_INFO_Y, 240, b, &FT_SM, k >= 0 ? (ed ? UI_ACC : WHT) : UI_DIM, UI_BG, 1);
        pg_key = key;
    }
    static const char *const LB[4][5] = {{"100", "400", "1K", "2.5K", "6K"}, {"80", "240", "750", "2.2K", "6.6K"},
                                          {"LOW", "FREQ", "GAIN", "Q", "HIGH"}, {"LOW", "FREQ", "GAIN", "Q", "HIGH"}};
    for (int i = 0; i < 5; i++) {                      /* the five values */
        const int s1 = (int)ui_item == i + 2, was = uc.item == i + 2;
        if (!force && uc.val[i] == q[i] && s1 == was && uc.mode == (int)ui_mode) continue;
        const uint32_t x = 4u + (uint32_t)i * 47u;
        const int e = s1 && ed;
        const uint16_t bg = s1 ? C_CARD : UI_BG, ring = s1 ? (e ? UI_ACC : WHT) : UI_BG;
        ui_rbox(x, GR_LOW_Y, 44, 44, 6, ring, UI_BG);
        ui_rbox(x + 1u, GR_LOW_Y + 1u, 42, 42, 5, bg, ring);
        ft_box(x + 2u, GR_LOW_Y + 4u, 40, LB[t][i], &FT_SM, s1 ? WHT : UI_DIM, bg, 1);
        char b[12] = "";
        if (!px_param_text(PX_B_EQ, t, i, q[i], b)) num3(b, q[i]);
        ft_box(x + 2u, GR_LOW_Y + 21u, 40, b, &FT_SM, e ? UI_ACC : UI_TXT, bg, 1);
        uc.val[i] = q[i];
    }
    uc.item = (int)ui_item; uc.mode = (int)ui_mode;
}
#endif

/* ---- render: sends only what changed to the screen ---- */
static void ui_fix_types(void)          /* OpenPocket Bass: AMP is NAM, CAB is USER IR, in every preset shown */
{
    for (int b = 0; b < PX_NBLK; b++) if (px_type_fixed(b) >= 0) PRESET.type[b] = (uint8_t)px_type_fixed(b);
}
static void ui_render(void)
{
    ui_fix_types();
    if (ui_mode == M_TUNER || ui_mode == M_DIAG) return;   /* (the main loop draws them every 100 ms: tuner_draw, diag_draw) */
    if (ui_mode == M_SAVE) { save_draw(); return; }
    if (ui_mode == M_NAME) { name_draw(); return; }
    if (ui_mode == M_MIDI) { midi_draw(); return; }
    if (ui_mode == M_LIST) { list_draw(); return; }
    int scr = UI_PAGE ? 1 : 0;
    if (scr != uc.screen) { uc.screen = scr; uc.full = 1; }
    int f = uc.full;
    if (f) {                                                     /* page change: background drawn once */
#if PX_BASS
        if (scr == 0) {
            lcd_rect(0, 0, 240, S_H, UI_PANEL);
            lcd_rect(0, S_H, 240, F_Y - S_H, UI_BG);
            lcd_rect(12, 76, 216, 1, C_LINE);
        } else
#endif
        if (scr == 0) {
            lcd_rect(0, 0, 240, S_H, UI_PANEL);
            lcd_rect(0, S_H, 240, P_NUM_Y - S_H, UI_BG);
            lcd_rect(0, P_NUM_Y, 6, 45, UI_BG); lcd_rect(90, P_NUM_Y, 4, 45, UI_BG);
            lcd_rect(94, P_NUM_Y, 142, 4, UI_BG); lcd_rect(94, P_NUM_Y + 27u, 142, 2, UI_BG); lcd_rect(94, P_NUM_Y + 44u, 142, 1, UI_BG);
            lcd_rect(236, P_NUM_Y, 4, 45, UI_BG);
            lcd_rect(0, P_NUM_Y + 45u, 240, P_NAME_Y - P_NUM_Y - 45u, UI_BG);
            lcd_rect(0, P_NAME_Y + 32u, 240, T_Y0 - 2u - P_NAME_Y - 32u, UI_BG);
            lcd_rect(16, P_NAME_Y + 36u, 208, 1, C_LINE);
            lcd_rect(0, T_Y0 - 2u, 2, 2u * T_DY + 2u, UI_BG); lcd_rect(236, T_Y0 - 2u, 4, 2u * T_DY + 2u, UI_BG);
            lcd_rect(0, T_Y0 + T_H + 2u, 240, T_DY - T_H - 4u, UI_BG);     /* between the two rows of tiles */
            lcd_rect(0, T_Y0 + T_DY + T_H + 2u, 240, P_INFO_Y - (T_Y0 + T_DY + T_H + 2u), UI_BG);
            lcd_rect(0, P_INFO_Y + 23u, 240, F_Y - P_INFO_Y - 23u, UI_BG);
        } else {
            lcd_rect(0, E_BAND, 240, 4, UI_BG);
            lcd_rect(0, E_BAND + 4u, 4, F_Y - E_BAND - 4u, UI_BG); lcd_rect(236, E_BAND + 4u, 4, F_Y - E_BAND - 4u, UI_BG);
        }
        lcd_rect(0, F_Y, 240, 3, UI_PANEL); lcd_rect(0, F_Y + 18u, 240, 240u - F_Y - 18u, UI_PANEL);
        if (scr) { lcd_rect(ST_X - 2u, F_Y + 3u, 2, 15, UI_PANEL); lcd_rect(ST_X + ST_W, F_Y + 3u, 240u - ST_X - ST_W, 15, UI_PANEL); }
        uc.full = 0;
    }
    if (scr == 0) {
        draw_status(f);
#if PX_BASS
        bp_play_top(f);
        bp_pedals(f);
        bp_input(f);
#else
        draw_play_top(f);
        draw_tiles(f);
        draw_info(f);
#endif
    } else draw_edit(f);
    draw_hint(f);
    if (f) { char s[12]; for (int i = 0; i < 12; i++) s[i] = uc.status[i]; uc.full = 1; ui_status(s, uc.status_kind); uc.full = 0; }
}

/* ---- actions ---- */
static void ui_enter_block(uint32_t b)
{
    if (!px_block_used((int)b)) return;               /* (OpenPocket Bass: a block taken out has no page) */
    ui_pvol = 0;
    if (ui_blk == b && UI_PAGE) PRESET.on[b] ^= 1u;   /* 2nd press on the same button: on/off */
    ui_blk = b; ui_mode = M_BLOCK; ui_item = 0;
}
static uint8_t mv_orig[PX_NBLK];            /* the order before the move (HOME puts it back) */
static void ui_move_start(void)
{
    ui_order_fix();
    for (int i = 0; i < PX_NBLK; i++) mv_orig[i] = PRESET.order[i];
    ui_mode = M_MOVE;
}
static void ui_move_drop(void) { ui_mode = M_CHAIN; }
static void ui_move_cancel(void) { for (int i = 0; i < PX_NBLK; i++) PRESET.order[i] = mv_orig[i]; ui_mode = M_CHAIN; }
static void ui_knob(int dd, int fast)
{
    if (ui_mode == M_CHAIN) {                          /* along the chain, in the preset's order */
        uint8_t v[PX_NBLK];
        ui_order_fix();
        const int n = ui_vis(v);
        if (PX_BASS_UI) {                              /* the preset's VOL, then the pedals, round */
            int at = ui_pvol ? -1 : ui_vpos_of(v, n, ui_blk), to = at + (dd > 0 ? 1 : -1);
            if (to < -1) to = n - 1;
            if (to >= n) to = -1;
            ui_pvol = to < 0;
            if (to >= 0) ui_blk = v[to];
        } else ui_blk = v[(ui_vpos_of(v, n, ui_blk) + n + dd) % n];
    } else if (ui_mode == M_MOVE) {                    /* the grabbed block swaps with its neighbour (a shown one) */
        uint8_t v[PX_NBLK];
        const int n = ui_vis(v), k = ui_vpos_of(v, n, ui_blk) + (dd > 0 ? 1 : -1);
        if (k >= 0 && k < n) {
            int i = ui_pos_of(ui_blk), j = ui_pos_of(v[k]);
            uint8_t t = PRESET.order[i]; PRESET.order[i] = PRESET.order[j]; PRESET.order[j] = t;
        }
    } else if (ui_mode == M_BLOCK && PX_BASS_UI && ui_blk == PX_B_AMP) {   /* the AMP page: ON, MODEL, the 3 points, GAIN, LEVEL */
        static const uint8_t ORD[7] = {1, 0, 3, 4, 5, 2, 6};
        int at = 0;
        for (int i = 0; i < 7; i++) if (ORD[i] == ui_item) at = i;
        ui_item = ORD[(at + 7 + (dd > 0 ? 1 : -1)) % 7];
    } else if (ui_mode == M_BLOCK) {                   /* v42: in the order drawn: ON (item 1) at the top, MODEL, the settings */
        int n = ui_nitems(), p = ui_item < 2u ? 1 - (int)ui_item : (int)ui_item;
        p = (p + n + dd) % n;
        ui_item = (uint32_t)(p < 2 ? 1 - p : p);
    } else if (ui_mode == M_PVOL) {                    /* the preset's level: 50 = 0 dB, 0.3 dB a step */
        int v = PRESET.p[PX_B_GATE][PX_PATCH_LEVEL] + dd * (fast ? 5 : 1);
        PRESET.p[PX_B_GATE][PX_PATCH_LEVEL] = (uint8_t)(v < 0 ? 0 : v > 100 ? 100 : v);
    } else if (ui_mode == M_EDIT) {
        const int k = ui_k_of(ui_item);
        if (ui_item == 0u && k < 0) {
            int nt = px_ntypes((int)ui_blk);
            PRESET.type[ui_blk] = (uint8_t)((PRESET.type[ui_blk] + nt + dd) % nt);
        } else if (k >= 0) {
            int t = PRESET.type[ui_blk], ln = ui_list_n((int)ui_blk, t, k);
            if (ln) {                                  /* one filled item a detent, at any speed; it stops at the ends */
                char nm[12];
                for (int i = ui_list_of((int)ui_blk, PRESET.p[ui_blk][k]) + dd; i >= 0 && i < ln; i += dd)
                    if (ui_list_name((int)ui_blk, i, nm)) { PRESET.p[ui_blk][k] = (uint8_t)ui_list_val((int)ui_blk, i); break; }
                return;
            }
            uint8_t *pv = ui_amp_band() >= 0 && ui_sub == 0 ? &PRESET.p[PX_AMP_LVL_B][1 + ui_amp_band()] : ui_pv(k);   /* (a band: its frequency first) */
            int v = *pv + dd * (fast ? 5 : 1);
            if (v < 0) v = 0;
            if (v > 100) v = 100;
            *pv = (uint8_t)v;
        }
    }
}
static void ui_push(void)
{
    if (ui_mode == M_MOVE) { ui_move_drop(); return; }
    if (ui_mode == M_PVOL) { ui_mode = M_CHAIN; return; }
    if (ui_mode == M_CHAIN && ui_pvol) { ui_mode = M_PVOL; return; }
    if (ui_mode == M_CHAIN) { ui_mode = M_BLOCK; ui_item = 0; }
    else if (ui_mode == M_BLOCK) {
        if (ui_item == 1u) PRESET.on[ui_blk] ^= 1u;
        else { ui_mode = M_EDIT; ui_sub = 0; }
    } else if (ui_amp_band() >= 0 && ui_sub == 0) ui_sub = 1;   /* an AMP band: its frequency, then its gain */
    else ui_mode = M_BLOCK;
}
static void ui_home(void) { if (ui_mode == M_MOVE) ui_move_cancel(); ui_mode = M_CHAIN; ui_item = 0; }

/* ===================================================== save page ======== */
/* hold PUSH: SAVE. KNOB picks the slot (01..52, then "FACTORY": this slot back to its factory preset), PUSH saves,
 * HOME cancels. Saving to another slot copies the sound there and moves to it. */
#define SV_FACTORY 52u
static uint32_t sv_dest, sv_shown = ~0u;
static void save_page(void)            /* the SAVE page drawn, its slot kept (back from NAME) */
{
    ui_mode = M_SAVE; sv_shown = ~0u;
    lcd_rect(0, 0, 240, 240, UI_BG);
    lcd_rect(0, 0, 240, 4, UI_ACC); lcd_rect(0, 27, 240, 3, UI_ACC); lcd_rect(0, 4, 8, 23, UI_ACC);
    ft_box(8, 4, 232, "SAVE PRESET", &FT_MD, BLK, UI_ACC, 0);
    ft_box(0, 40, 240, PRESET.name, &FT_MD, UI_TXT, UI_BG, 1);
    lcd_rect(20, 70, 200, 1, C_LINE);
    lcd_rect(0, F_Y, 240, 240u - F_Y, UI_PANEL);
}
static void save_open(void) { sv_dest = cur_preset; save_page(); }
static void save_draw(void)
{
    if (sv_shown == sv_dest) return;
    sv_shown = sv_dest;
    char b[20];
    int restore = sv_dest == SV_FACTORY, can = !restore || st_saved(cur_preset);   /* (v42: nothing to restore: PUSH does nothing) */
    ft_box(0, F_Y + 3u, 240, !can ? "KNOB: SLOT   HOME: NO" : restore ? "PUSH: RESTORE   HOME: NO" : "KNOB: SLOT  PUSH: SAVE  HOME: NO", &FT_SM, UI_DIM, UI_PANEL, 1);
    ft_box(0, 186, 240, restore ? "" : "FX: RENAME", &FT_SM, UI_DIM, UI_BG, 1);   /* (OpenPocket Bass: the pedal renames) */
    if (restore) {
        ft_box(0, 82, 240, "", &FT_XL, UI_ACC, UI_BG, 1);
        ft_box(0, 132, 240, "RESTORE FACTORY", &FT_MD, UI_TXT, UI_BG, 1);
        b[0] = 'S'; b[1] = 'L'; b[2] = 'O'; b[3] = 'T'; b[4] = ' ';
        b[5] = (char)('0' + (cur_preset + 1u) / 10u); b[6] = (char)('0' + (cur_preset + 1u) % 10u); b[7] = 0;
        ft_box(0, 160, 240, can ? b : "ALREADY THE FACTORY SOUND", &FT_SM, UI_DIM, UI_BG, 1);
        return;
    }
    px_preset_t d;
    st_get(sv_dest, &d);
    b[0] = (char)('0' + (sv_dest + 1u) / 10u); b[1] = (char)('0' + (sv_dest + 1u) % 10u); b[2] = 0;
    ft_box(0, 82, 240, b, &FT_XL, UI_ACC, UI_BG, 1);
    ft_box(0, 132, 240, sv_dest == cur_preset ? "THIS SLOT" : "REPLACES", &FT_MD, sv_dest == cur_preset ? UI_TXT : UI_DIM, UI_BG, 1);
    ft_box(0, 160, 240, sv_dest == cur_preset ? "" : d.name, &FT_SM, UI_TXT, UI_BG, 1);
}
static void save_close(void)
{
    ui_mode = M_CHAIN; ui_item = 0;
    lcd_rect(0, 0, 240, 240, UI_BG);
    uc.screen = -1;
    ui_invalidate();
}
static void save_knob(int dd)
{
    sv_dest = (sv_dest + SV_FACTORY + 1u + (uint32_t)(dd > 0 ? 1 : SV_FACTORY)) % (SV_FACTORY + 1u);
}
static void save_confirm(void)
{
    int rc;
    if (sv_dest == SV_FACTORY && !st_saved(cur_preset)) return;
    ft_box(0, 186, 240, "SAVING...", &FT_MD, UI_ACC, UI_BG, 1);
    if (sv_dest == SV_FACTORY) { rc = st_reset(cur_preset); }
    else { rc = st_save(sv_dest, &PRESET); if (!rc) cur_preset = sv_dest; }
    st_gen++;
    if (!rc) load_preset(cur_preset);
    {
        char b[16] = "ERROR -";
        if (rc) { b[7] = (char)('0' + (uint32_t)(-rc) % 10u); b[8] = 0; }
        ft_box(0, 186, 240, rc ? b : "SAVED", &FT_MD, rc ? RED : GRN, UI_BG, 1);
    }
    for (int i = 0; i < 7; i++) { fm1_wdt_feed(); fm1_delay_ms(100); }
    save_close();
}
/* ===================================================== NAME page ======== */
/* OpenPocket Bass: FX on the SAVE page renames the preset on the pedal. KNOB moves along the 13 letters and OK; PUSH on a
 * letter picks it (the KNOB then changes it, PUSH again keeps it); PUSH on OK takes the name back to SAVE; HOME leaves it
 * as it was. The name is saved with the preset (PUSH on SAVE). */
#define NM_LEN 13
static const char NM_SET[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-+&#/.!";
static char nm_old[14];
static int nm_at, nm_ed, nm_drawn;
static int nm_idx(char c) { for (int i = 0; NM_SET[i]; i++) if (NM_SET[i] == c) return i; return 0; }
static void name_open(void)
{
    ui_mode = M_NAME; nm_at = 0; nm_ed = 0; nm_drawn = 0;
    for (int i = 0; i < 14; i++) nm_old[i] = PRESET.name[i];
    for (int i = 0; i < NM_LEN; i++) {                 /* letters the set has not (lower case...) become theirs or a space */
        char c = PRESET.name[i];
        if (c >= 'a' && c <= 'z') c = (char)(c - 32);
        if (!c) { for (int j = i; j < NM_LEN; j++) PRESET.name[j] = ' '; break; }
        PRESET.name[i] = NM_SET[nm_idx(c)];
    }
    PRESET.name[NM_LEN] = 0;
    lcd_rect(0, 0, 240, 240, UI_BG);
    lcd_rect(0, 0, 240, 4, UI_ACC); lcd_rect(0, 27, 240, 3, UI_ACC); lcd_rect(0, 4, 8, 23, UI_ACC);
    ft_box(8, 4, 232, "PRESET NAME", &FT_MD, BLK, UI_ACC, 0);
    lcd_rect(0, F_Y, 240, 240u - F_Y, UI_PANEL);
}
static void name_draw(void)
{
    static int last_at = -1, last_ed = -1;
    static char last[14];
    const int full = !nm_drawn;
    for (int i = 0; i < NM_LEN; i++) {                 /* 13 cells of 17 px */
        const int sel = i == nm_at, was = i == last_at;
        if (!full && last[i] == PRESET.name[i] && sel == was && (!sel || nm_ed == last_ed)) continue;
        const uint32_t x = 9u + (uint32_t)i * 17u;
        const uint16_t bg = sel ? (nm_ed ? UI_ACC : C_CARD) : UI_BG;
        char c[2] = {PRESET.name[i], 0};
        ui_rbox(x, 86, 16, 26, 3, bg, UI_BG);
        ft_box(x + 1u, 91, 14, c[0] == ' ' ? "" : c, &FT_SM, sel && nm_ed ? BLK : UI_TXT, bg, 1);
        lcd_rect(x + 2u, 115, 12, 2, sel ? (nm_ed ? UI_ACC : WHT) : (c[0] == ' ' ? C_LINE : UI_BG));
    }
    int chg = full;                                    /* the whole name, as PLAY shows it */
    for (int i = 0; i < 14; i++) if (last[i] != PRESET.name[i]) chg = 1;
    if (chg) ft_box(0, 44, 240, PRESET.name, &FT_MD, UI_TXT, UI_BG, 1);
    if (full || (nm_at == NM_LEN) != (last_at == NM_LEN)) {
        const int sel = nm_at == NM_LEN;
        ui_rbox(80, 140, 80, 34, 6, sel ? UI_ACC : C_CARD, UI_BG);
        ft_box(84, 145, 72, "OK", &FT_MD, sel ? BLK : UI_TXT, sel ? UI_ACC : C_CARD, 1);
    }
    if (full || nm_ed != last_ed || (nm_at == NM_LEN) != (last_at == NM_LEN))
        ft_box(0, F_Y + 3u, 240, nm_ed ? "KNOB: LETTER   PUSH: KEEP" : nm_at == NM_LEN ? "PUSH: DONE   HOME: CANCEL"
               : "KNOB: MOVE  PUSH: EDIT  HOME: NO", &FT_SM, UI_DIM, UI_PANEL, 1);
    for (int i = 0; i < 14; i++) last[i] = PRESET.name[i];
    last_at = nm_at; last_ed = nm_ed; nm_drawn = 1;
}
static void name_knob(int dd)
{
    if (nm_ed) {
        const int n = (int)sizeof NM_SET - 1, k = (nm_idx(PRESET.name[nm_at]) + n + (dd > 0 ? 1 : -1)) % n;
        PRESET.name[nm_at] = NM_SET[k];
    } else {
        nm_at += dd > 0 ? 1 : -1;
        if (nm_at < 0) nm_at = 0;
        if (nm_at > NM_LEN) nm_at = NM_LEN;
    }
}
static void name_done(int keep)
{
    if (keep) {                                        /* the spaces at the end dropped */
        int e = NM_LEN;
        while (e > 0 && PRESET.name[e - 1] == ' ') e--;
        for (int i = e; i < 14; i++) PRESET.name[i] = 0;
        if (!e) { PRESET.name[0] = '-'; }
    } else for (int i = 0; i < 14; i++) PRESET.name[i] = nm_old[i];
    save_page();
}
static void name_push(void)
{
    if (nm_at == NM_LEN) name_done(1);
    else nm_ed = !nm_ed;
}

/* ===================================================== MIDI page ======== */
/* hold FX: the MIDI settings (docs/MIDI.md). KNOB changes the selected row, PUSH moves to the next row and, after the
 * last one, leaves; HOME leaves. The settings are saved (a flash write, as SAVE) only if something changed.
 * v42: a fourth row, USB AUDIO (docs/USB-AUDIO.md): the row shows mi_ua, and the setting (px_ua_on) follows only on
 * leaving, because a change takes the pedal off USB for a second. */
#if OP_UAUDIO
enum { MI_CHAN, MI_SEND, MI_CLOCK, MI_UAUDIO, MI_ROWS };
#else
enum { MI_CHAN, MI_SEND, MI_CLOCK, MI_ROWS };
#endif
static uint32_t mi_row, mi_shown_hash = ~0u;
static uint8_t mi_orig, mi_ua;
static void midi_persist(void);                 /* openpocket.c: writes the settings to the store */
static void midi_open(void)
{
    ui_mode = M_MIDI; mi_row = 0; mi_shown_hash = ~0u; mi_orig = px_midi_cfg_pack(&mid_cfg); mi_ua = px_ua_on;
    lcd_rect(0, 0, 240, 240, UI_BG);
    lcd_rect(0, 0, 240, 4, UI_ACC); lcd_rect(0, 27, 240, 3, UI_ACC); lcd_rect(0, 4, 8, 23, UI_ACC);
    ft_box(8, 4, 232, "MIDI", &FT_MD, BLK, UI_ACC, 0);
    lcd_rect(0, F_Y, 240, 240u - F_Y, UI_PANEL);
    ft_box(0, F_Y + 3u, 240, "KNOB: SET   PUSH: NEXT   HOME: OK", &FT_SM, UI_DIM, UI_PANEL, 1);
}
static void midi_draw(void)
{
    uint32_t h = mi_row | (uint32_t)px_midi_cfg_pack(&mid_cfg) << 4 | (uint32_t)mid_usb_up << 12 |
                 (uint32_t)mid_last[0] << 13 | (uint32_t)mid_last[1] << 16 | (uint32_t)mid_last[2] << 23 | (uint32_t)mi_ua << 31;
    h ^= (uint32_t)mid_c1_off * 0x27D4EB2Fu;
    if (h == mi_shown_hash) return;
    mi_shown_hash = h;
#if OP_UAUDIO
    static const char *const NAME[MI_ROWS] = {"CHANNEL", "SEND", "CLOCK > DELAY", "USB AUDIO"};
#else
    static const char *const NAME[MI_ROWS] = {"CHANNEL", "SEND", "CLOCK > DELAY"};
#endif
    char v[MI_ROWS][8];
    if (mid_cfg.chan) { v[0][0] = (char)(mid_cfg.chan >= 10 ? '0' + mid_cfg.chan / 10 : ' '); v[0][1] = (char)('0' + mid_cfg.chan % 10); v[0][2] = 0; }
    else { v[0][0] = 'O'; v[0][1] = 'M'; v[0][2] = 'N'; v[0][3] = 'I'; v[0][4] = 0; }
    v[1][0] = v[2][0] = 0;
    b_str(v[1], mid_cfg.send ? "ON" : "OFF");
    b_str(v[2], mid_cfg.clock ? "ON" : "OFF");
#if OP_UAUDIO
    v[MI_UAUDIO][0] = 0;
    b_str(v[MI_UAUDIO], mi_ua ? "ON" : "OFF");
#endif
    for (uint32_t r = 0; r < MI_ROWS; r++) {       /* (v42: four rows of 30 px; three of 36 before) */
        uint32_t y = 36u + r * 30u;
        uint16_t bg = r == mi_row ? C_CARD : UI_BG;
        lcd_rect(0, y, 240, 28, bg);
        ft_box(8, y + 6u, 130, NAME[r], &FT_SM, r == mi_row ? UI_TXT : UI_DIM, bg, 0);
        ft_box(140, y + 2u, 92, v[r], &FT_MD, r == mi_row ? UI_ACC : UI_TXT, bg, 2);
    }
    ft_box(0, 162, 240, mid_usb_up ? "USB: HOST CONNECTED" : "USB: NO HOST", &FT_SM, mid_usb_up ? GRN : UI_DIM, UI_BG, 1);
    char l[24]; l[0] = 0;
    if (mid_last[0] == PXM_PC) { b_str(l, "LAST: PC "); b_num(l, mid_last[1] + 1u); }
    else if (mid_last[0] == PXM_CC) { b_str(l, "LAST: CC "); b_num(l, mid_last[1]); b_str(l, " = "); b_num(l, mid_last[2]); }
    else b_str(l, "LAST: NONE");
    ft_box(0, 186, 240, l, &FT_SM, UI_TXT, UI_BG, 1);
    /* v41: the second core retired (one core left until a power cycle: presets that need both go to BYP), and why */
    char c[32]; c[0] = 0;
    if (mid_c1_off) {
        static const char *const WHY[] = {"", "MISREAD", "LATE", "UNTIL POWER-OFF", "FAULT", "SILENT"};
        b_str(c, "CORE 2 OFF: "); b_str(c, mid_c1_off <= 5u ? WHY[mid_c1_off] : "?");
    }
    ft_box(0, 204, 240, c, &FT_SM, RED, UI_BG, 1);
}
static void midi_close(void)
{
    ui_mode = M_CHAIN; ui_item = 0;
    if (px_midi_cfg_pack(&mid_cfg) != mi_orig || mi_ua != px_ua_on) { px_ua_on = mi_ua; midi_persist(); }
    lcd_rect(0, 0, 240, 240, UI_BG);
    uc.screen = -1;
    ui_invalidate();
}
static void midi_knob(int dd)
{
    if (mi_row == MI_CHAN) mid_cfg.chan = (uint8_t)((mid_cfg.chan + 17u + (uint32_t)(dd > 0 ? 1 : 16)) % 17u);
    else if (mi_row == MI_SEND) mid_cfg.send ^= 1u;
    else if (mi_row == MI_CLOCK) mid_cfg.clock ^= 1u;
    else mi_ua ^= 1u;
}
static void midi_push(void) { if (++mi_row >= MI_ROWS) midi_close(); }

/* v42: one unsaved edit survives a preset change by foot or from the list: stepping away keeps it, coming back to its slot
 * gives it back (still EDITED). An edit elsewhere takes its place; it is dropped if its slot was saved over meanwhile. */
static px_preset_t ui_kept;
static uint32_t ui_kept_slot = ~0u, ui_kept_sum;     /* (the sum of what the slot held when the edit was kept) */
static uint32_t ui_sum(const px_preset_t *p)
{
    const uint8_t *b = (const uint8_t *)p;
    uint32_t h = 2166136261u;
    for (uint32_t i = 0; i < sizeof *p; i++) h = (h ^ b[i]) * 16777619u;
    return h;
}
static void ui_goto(uint32_t n)
{
    if (ui_dirty()) { ui_kept = PRESET; ui_kept_slot = cur_preset; ui_kept_sum = ui_sum(&ui_ref); }
    cur_preset = n;
    load_preset(n);
    if (ui_kept_slot == n) {
        if (ui_sum(&PRESET) == ui_kept_sum) PRESET = ui_kept;
        ui_kept_slot = ~0u;
    }
}
static int ui_stepped;                 /* the last footswitch's step (+1, -1; 0: none): both footswitches take it back */
static void ui_preset_step(int dd)
{
    int n = px_preset_count();
    ui_goto((uint32_t)(((int)cur_preset + n + dd) % n));
    ui_stepped = dd;
    if (UI_PAGE || ui_mode == M_LIST) { ui_mode = M_CHAIN; ui_item = 0; }   /* a footswitch on a block page goes back to PLAY, where the preset shows */
}
/* the FX button's blocks (the FX light follows them: px_lights.h) */
static const uint8_t UI_FXB[7] = {PX_B_DRIVE, PX_B_SFX, PX_B_SYN, PX_B_PITCH, PX_B_COMP, PX_B_GATE, PX_B_EQ};
static int ui_is_fx(uint32_t b) { for (int i = 0; i < 7; i++) if (UI_FXB[i] == b) return px_block_used((int)b); return 0; }
static void ui_fx_cycle(void)          /* FX: the next of its blocks along the preset's chain, as the tiles read (v42), else the first */
{
    ui_order_fix();
    ui_pvol = 0;
    int from = ui_is_fx(ui_blk) ? ui_pos_of(ui_blk) + 1 : 0;
    for (int i = 0; i < PX_NBLK; i++) {
        uint32_t b = PRESET.order[(from + i) % PX_NBLK];
        if (ui_is_fx(b)) { ui_blk = b; break; }
    }
    ui_mode = M_BLOCK; ui_item = 0;
}

/* ===================================================== preset list ======== */
/* v42: HOME on PLAY: the knob browses the presets by name without loading them, PUSH loads the one in the middle, HOME goes
 * back. The playing preset's number is amber. */
#define LS_Y0  38u
#define LS_ROW 36u
static uint32_t ls_at, ls_shown = ~0u;
static void list_open(void)
{
    ui_mode = M_LIST; ls_at = cur_preset; ls_shown = ~0u;
    lcd_rect(0, 0, 240, 240, UI_BG);
    lcd_rect(0, 0, 240, 4, UI_ACC); lcd_rect(0, 27, 240, 3, UI_ACC); lcd_rect(0, 4, 8, 23, UI_ACC);
    ft_box(8, 4, 232, "PRESETS", &FT_MD, BLK, UI_ACC, 0);
    lcd_rect(0, F_Y, 240, 240u - F_Y, UI_PANEL);
    ft_box(0, F_Y + 3u, 240, "KNOB: PRESET   PUSH: LOAD", &FT_SM, UI_DIM, UI_PANEL, 1);
    uc.screen = -1;
}
static void list_draw(void)
{
    uint32_t key = ls_at | cur_preset << 8 | st_gen << 16;
    if (key == ls_shown) return;
    ls_shown = key;
    int n = px_preset_count();
    for (uint32_t r = 0; r < 5u; r++) {
        uint32_t k = (uint32_t)(((int)ls_at + n + (int)r - 2) % n), y = LS_Y0 + r * LS_ROW;
        int sel = r == 2u;
        uint16_t bg = sel ? C_CARD : UI_BG;
        px_preset_t q;
        char b[4];
        st_get(k, &q);
        b[0] = (char)('0' + (k + 1u) / 10u); b[1] = (char)('0' + (k + 1u) % 10u); b[2] = 0;
        ui_rbox(4, y, 232, LS_ROW - 2u, 5, bg, UI_BG);
        if (sel) lcd_rect(4, y + 10u, 3, 14, UI_ACC);
        ft_box(12, y + 5u, 40, b, &FT_MD, k == cur_preset ? UI_ACC : sel ? UI_TXT : UI_DIM, bg, 0);
        ft_box(52, y + 5u, 180, q.name, &FT_MD, sel ? WHT : UI_DIM, bg, 0);
    }
}
static void list_knob(int dd) { int n = px_preset_count(); ls_at = (uint32_t)(((int)ls_at + n + dd) % n); }
static void list_close(void) { ui_mode = M_CHAIN; ui_item = 0; uc.screen = -1; ui_invalidate(); }
static void list_load(void) { if (ls_at != cur_preset) ui_goto(ls_at); list_close(); }

/* ===================================================== tuner ======== */
/* v42: the tuner only; the readings for a photo are on the diagnostics page, HOME held on past the tuner */
static const char *NOTE_N[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
static int tn_last_note = -2, tn_last_x = -1, tn_last_c = 1000, tn_last_dir = 2;
#define TN_BAR_Y 172u
static void tuner_open(void)
{
    ui_mode = M_TUNER;
    a_tmute = 1;                                              /* v36: silent while tuning */
    lcd_rect(0, 0, 240, 240, UI_BG);
    lcd_rect(0, 0, 240, 30, GRN);
    ft_box(8, 4, 140, "TUNER", &FT_MD, BLK, GRN, 0);
    ft_box(148, 8, 88, "A = 440", &FT_SM, BLK, GRN, 2);
    ui_rbox(18, TN_BAR_Y - 2u, 204, 14, 5, C_BAR, UI_BG);
    lcd_rect(110, TN_BAR_Y - 2u, 20, 14, ui_dark(GRN));
    lcd_rect(119, TN_BAR_Y - 10u, 2, 30, UI_TXT);
    ft_box(10, TN_BAR_Y + 18u, 40, "-50", &FT_SM, UI_DIM, UI_BG, 1);
    ft_box(100, TN_BAR_Y + 18u, 40, "0", &FT_SM, UI_DIM, UI_BG, 1);
    ft_box(190, TN_BAR_Y + 18u, 40, "+50", &FT_SM, UI_DIM, UI_BG, 1);
    lcd_rect(0, F_Y, 240, 240u - F_Y, UI_PANEL);
    ft_box(0, F_Y + 3u, 240, "ANY SWITCH: EXIT  HOLD HOME: DIAG", &FT_SM, UI_DIM, UI_PANEL, 1);
    tn_last_note = -2; tn_last_x = -1; tn_last_c = 1000; tn_last_dir = 2;
}
static void tuner_close(void) { a_tmute = 0; ui_mode = M_CHAIN; lcd_rect(0, 0, 240, 240, UI_BG); uc.screen = -1; ui_invalidate(); }
static void tuner_draw(float hz)       /* hz <= 0: no note */
{
    int note = -1, c = 0; float cents = 0.0f;
    if (hz > 20.0f) {
        float st = 12.0f * logf(hz / 440.0f) * 1.44269504f;   /* semitones from A4 = 440 Hz */
        int r = (int)(st + (st >= 0 ? 0.5f : -0.5f));
        cents = (st - (float)r) * 100.0f;
        note = ((r % 12) + 12 + 9) % 12;                      /* A = 9 */
        c = (int)(cents + (cents >= 0 ? 0.5f : -0.5f));
    }
    int ok = (note >= 0 && c > -5 && c < 5);
    if (note != tn_last_note || (note >= 0 && (ok != (tn_last_c > -5 && tn_last_c < 5)))) {
        ft_box(56, 60, 128, note < 0 ? "-" : NOTE_N[note], &FT_LG, note < 0 ? UI_DIM : (ok ? GRN : UI_TXT), UI_BG, 1);
        tn_last_note = note;
    }
    int dir = note < 0 || ok ? 0 : c < 0 ? -1 : 1;           /* which way to turn: an arrow toward in tune, on the side the note is */
    if (dir != tn_last_dir) {
        ft_box(10, 76, 46, dir < 0 ? ">>" : "", &FT_MD, UI_ACC, UI_BG, 1);
        ft_box(184, 76, 46, dir > 0 ? "<<" : "", &FT_MD, UI_ACC, UI_BG, 1);
        tn_last_dir = dir;
    }
    if (c != tn_last_c || note < 0) {
        char b[8] = "";
        if (note >= 0) { int a = c < 0 ? -c : c; b[0] = (char)(c < 0 ? '-' : '+'); num3(b + 1, (uint32_t)a); }
        int i = 0; while (b[i]) i++;
        if (note >= 0) { b[i++] = ' '; b[i++] = 'c'; b[i++] = 't'; b[i] = 0; }
        ft_box(60, 128, 120, b, &FT_MD, ok ? GRN : UI_TXT, UI_BG, 1);
        tn_last_c = note < 0 ? 1000 : c;
    }
    int x = -1;
    if (note >= 0) { x = 120 + c * 2; if (x < 22) x = 22; if (x > 216) x = 216; }
    if (x != tn_last_x) {                                     /* needle: erase the old one, draw the new one */
        if (tn_last_x >= 0) {
            uint16_t under = (tn_last_x >= 108 && tn_last_x <= 132) ? ui_dark(GRN) : C_BAR;
            lcd_rect((uint32_t)tn_last_x - 3u, TN_BAR_Y - 2u, 7, 14, under);
            if (tn_last_x >= 107 && tn_last_x <= 133) { lcd_rect(110, TN_BAR_Y - 2u, 20, 14, ui_dark(GRN)); }
            lcd_rect(119, TN_BAR_Y - 10u, 2, 30, UI_TXT);
        }
        if (x >= 0) lcd_rect((uint32_t)x - 3u, TN_BAR_Y - 2u, 7, 14, (x >= 110 && x <= 130) ? GRN : UI_ACC);
        tn_last_x = x;
    }
}

/* ===================================================== diagnostics ======== */
/* v42 (on the tuner page before): what a bug report needs a photo of, as pictures with every figure written beside them.
 * HOME held on past the tuner; the knob turns the page (1: the DSP, 2: input and power); any control leaves. The sound
 * plays. Colours: green, amber above UI_LOAD_HIGH, red when a block ran late or the effects are bypassed. */
#define DG_X   8u
#define DG_W   228u
#define DG_BLK 4                       /* the blocks shown in "CPU PER BLOCK": the costliest, the first one highlighted */
static uint32_t dg_page, dg_hash[4], dg_clip_seen, dg_clip_hold;
static uint16_t dg_col(uint32_t pct, int bad) { return bad ? RED : pct > UI_LOAD_HIGH ? UI_ACC : GRN; }
static uint32_t dg_w(uint32_t v, uint32_t full, uint32_t w) { return v >= full ? w : v * w / full; }
/* a bar of w px for v of full, and a white tick at tick (0: none) with 2 px above and below it */
static void dg_bar(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t v, uint32_t full, uint16_t col, uint32_t tick)
{
    uint32_t f = dg_w(v, full, w);
    lcd_rect(x, y - 2u, w, 2, UI_BG); lcd_rect(x, y + h, w, 2, UI_BG);
    if (f) lcd_rect(x, y, f, h, col);
    if (f < w) lcd_rect(x + f, y, w - f, h, C_BAR);
    if (tick) { uint32_t t = dg_w(tick, full, w); lcd_rect(x + (t ? t - 1u : 0u), y - 2u, 2, h + 4u, WHT); }
}
static void dg_chip(uint32_t x, uint32_t y, uint32_t w, const char *s, uint16_t col)   /* a counter: its text on a dark chip */
{
    ui_rbox(x, y, w, 18, 4, C_CARD, UI_BG);
    ft_box(x + 3u, y + 2u, w - 6u, s, &FT_SM, col, C_CARD, 1);
}
static int dg_top(uint8_t *idx, int n)  /* the costliest blocks (a_pf; PX_NBLK = the NAM network), most first: how many */
{
    uint8_t used[PX_NBLK + 1] = {0};
    int c = 0;
    for (; c < n; c++) {
        int best = -1;
        for (int k = 0; k <= PX_NBLK; k++) if (!used[k] && a_pf[k] && (best < 0 || a_pf[k] > a_pf[best])) best = k;
        if (best < 0) break;
        used[best] = 1; idx[c] = (uint8_t)best;
    }
    return c;
}
static void dg_pct(char *b, const char *label, uint32_t v) { b[0] = 0; b_str(b, label); b_num(b, v > 999u ? 999u : v); b_str(b, "%"); }
static int dg_changed(int k, uint32_t h) { if (h == dg_hash[k]) return 0; dg_hash[k] = h; return 1; }
static uint32_t dg_mix(uint32_t h, uint32_t v) { return (h ^ v) * 16777619u; }
static void diag_dsp(void)             /* page 1: the load against the guard, each core, the counters, the CPU per block */
{
    static const char *const CORES[3] = {"C0", "C1", "2C"};
    uint32_t late = (uint32_t)bat_raw[2], clip = (uint32_t)bat_raw[3], pk = (uint32_t)bat_raw[4];
    int bad = ui_dg.late || ui_dg.bypass;
    uint16_t c = dg_col(ui_dg.load, bad);
    char b[32];
    uint32_t h = 2166136261u;
    h = dg_mix(h, ui_dg.load); h = dg_mix(h, ui_dg.peak); h = dg_mix(h, ui_dg.cores); h = dg_mix(h, ui_dg.guard); h = dg_mix(h, (uint32_t)bad);
    if (dg_changed(0, h)) {
        dg_pct(b, "DSP ", ui_dg.load);
        ft_box(DG_X, 34, 120, b, &FT_MD, c, UI_BG, 0);
        dg_pct(b, "PEAK ", ui_dg.peak);
        ft_box(128, 38, 104, b, &FT_SM, UI_TXT, UI_BG, 2);
        dg_bar(10, 60, 220, 6, ui_dg.load, 100, c, ui_dg.guard);
        b[0] = 0; b_str(b, "CORES "); b_str(b, CORES[ui_dg.cores < 3u ? ui_dg.cores : 0u]);
        ft_box(DG_X, 70, 80, b, &FT_SM, UI_TXT, UI_BG, 0);
        dg_pct(b, "GUARD ", ui_dg.guard);
        ft_box(152, 70, 80, b, &FT_SM, UI_DIM, UI_BG, 2);
    }
    h = 2166136261u;
    for (int k = 0; k < 2; k++) h = dg_mix(h, ui_dg.cl[k]);
    h = dg_mix(h, ui_dg.split); h = dg_mix(h, ui_dg.guard);
    if (dg_changed(1, h)) {
        for (uint32_t k = 0; k < 2u; k++) {                  /* each core (with the layer split; else "--") */
            uint32_t y = 88u + k * 16u;
            b[0] = 0; b_str(b, k ? "CPU1 " : "CPU0 ");
            if (ui_dg.split) { b_num(b, ui_dg.cl[k]); b_str(b, "%"); } else b_str(b, "--");
            ft_box(DG_X, y, 78, b, &FT_SM, ui_dg.split ? UI_TXT : UI_DIM, UI_BG, 0);
            dg_bar(88, y + 5u, 144, 6, ui_dg.split ? ui_dg.cl[k] : 0u, 100, dg_col(ui_dg.cl[k], 0), ui_dg.guard);
        }
    }
    h = dg_mix(dg_mix(dg_mix(dg_mix(2166136261u, late), clip), pk), ui_dg.bypass);
    if (dg_changed(2, h)) {                                /* the counters since boot, red (or amber) once not zero */
        dg_pct(b, "PK ", pk);                              /* (the highest block load since boot) */
        ft_box(88, 70, 64, b, &FT_SM, dg_col(pk, pk >= 100u), UI_BG, 1);
        b[0] = 0; b_str(b, "LATE "); b_num(b, late > 9999u ? 9999u : late); dg_chip(2, 122, 76, b, late ? RED : GRN);
        b[0] = 0; b_str(b, "CLIP "); b_num(b, clip > 9999u ? 9999u : clip); dg_chip(79, 122, 76, b, clip ? UI_ACC : GRN);
        dg_chip(156, 122, 82, ui_dg.bypass ? "BYPASS ON" : "BYPASS OFF", ui_dg.bypass ? RED : GRN);
    }
    uint8_t top[DG_BLK];
    int n = dg_top(top, DG_BLK);
    h = dg_mix(2166136261u, a_wcut);
    for (int i = 0; i < n; i++) h = dg_mix(dg_mix(h, top[i]), a_pf[top[i]]);
    if (dg_changed(3, dg_mix(h, (uint32_t)n))) {
        ft_box(DG_X, 143, 140, "CPU PER BLOCK %", &FT_SM, UI_DIM, UI_BG, 0);
        b[0] = 0;
        if (a_wcut) { b_str(b, "CUT "); b_num(b, a_wcut > 9999u ? 9999u : a_wcut); }   /* v39: wet blocks cut short */
        ft_box(148, 143, 84, b, &FT_SM, RED, UI_BG, 2);
        for (int i = 0; i < DG_BLK; i++) {                   /* sorted, the heaviest in amber */
            uint32_t y = 160u + (uint32_t)i * 15u;
            if (i >= n) { lcd_rect(0, y, 240, 15, UI_BG); continue; }
            const char *l = top[i] == PX_NBLK ? "NAM" : px_block_label(top[i]);
            ft_box(DG_X, y, 42, l, &FT_SM, i ? UI_TXT : WHT, UI_BG, 0);
            dg_bar(52, y + 5u, 148, 5, a_pf[top[i]], 100, i ? UI_DIM : UI_ACC, 0);
            b[0] = 0; b_num(b, a_pf[top[i]]);
            ft_box(202, y, 34, b, &FT_SM, i ? UI_TXT : UI_ACC, UI_BG, 2);
        }
    }
}
#define DG_DB_MIN 90                    /* the input meters: -90 dB to full scale; the last 3 dB are the clip zone */
static int r_str(char *b, int32_t v) { if (v < 0) { b[0] = '-'; b[1] = '-'; return 2; } char t[8]; t[0] = 0; b_num(t, (uint32_t)v); int i = 0; while (t[i]) { b[i] = t[i]; i++; } return i; }
static void diag_power(void)           /* page 2: the input levels, the output's soft clip, the battery, the raw readings */
{
    char b[40];
    int lv[2] = {a_level_db(0), a_level_db(1)};
    uint32_t clip = (uint32_t)bat_raw[3];
    if (clip != dg_clip_seen) { dg_clip_seen = clip; dg_clip_hold = 20u; }   /* lit for 20 draws (2 s) after a clip */
    else if (dg_clip_hold) dg_clip_hold--;
    uint32_t h = dg_mix(dg_mix(2166136261u, (uint32_t)lv[0]), (uint32_t)lv[1]);
    if (dg_changed(0, h)) {
        for (int ch = 0; ch < 2; ch++) {                     /* "L -62 DB" and a meter: green, amber above -18, red above -6 */
            uint32_t y = 54u + (uint32_t)ch * 18u;
            int v = lv[ch] < -DG_DB_MIN ? -DG_DB_MIN : lv[ch] > 0 ? 0 : lv[ch];
            b[0] = 0; b_str(b, ch ? "R " : "L ");
            if (lv[ch] <= -199) b_str(b, "---"); else { if (lv[ch] < 0) b_str(b, "-"); b_num(b, (uint32_t)(lv[ch] < 0 ? -lv[ch] : lv[ch])); }
            b_str(b, " DB");
            ft_box(DG_X, y, 74, b, &FT_SM, UI_TXT, UI_BG, 0);
            uint32_t w = 150u, zone = w * 3u / DG_DB_MIN, f = dg_w((uint32_t)(v + DG_DB_MIN), DG_DB_MIN, w);
            lcd_rect(82, y + 2u, w, 2, UI_BG); lcd_rect(82, y + 12u, w, 2, UI_BG);
            if (f) lcd_rect(82, y + 4u, f, 8, v > -6 ? RED : v > -18 ? UI_ACC : GRN);
            if (f < w - zone) lcd_rect(82 + f, y + 4u, w - zone - f, 8, C_BAR);
            if (f < w) { uint32_t s = f > w - zone ? f : w - zone; lcd_rect(82 + s, y + 4u, w - s, 8, ui_dark(RED)); }   /* the clip zone */
        }
    }
    h = dg_mix(dg_mix(2166136261u, clip), dg_clip_hold ? 1u : 0u);
    if (dg_changed(1, h)) {                                  /* the output: a marker lit while it reaches the soft clipper */
        lcd_rect(DG_X, 112, 14, 14, dg_clip_hold ? RED : C_BAR);
        b[0] = 0; b_str(b, "SOFT CLIP: "); b_num(b, clip); b_str(b, " BLOCKS");
        ft_box(28, 111, 204, b, &FT_SM, dg_clip_hold ? RED : UI_TXT, UI_BG, 0);
    }
    int usb = bat_usb_mv > 4300, pct = bat_pct >= 0 ? bat_pct : bat_mv ? bat_percent(bat_mv) : -1;
    h = dg_mix(dg_mix(dg_mix(2166136261u, (uint32_t)bat_mv), (uint32_t)usb), (uint32_t)pct);
    if (dg_changed(2, h)) {                                  /* the battery: a cell, its volts, its percent (or USB) */
        uint16_t c = usb ? GRN : pct <= 15 ? RED : pct <= 30 ? UI_ACC : GRN;
        uint32_t f = pct < 0 ? 0u : (uint32_t)pct * 40u / 100u;
        lcd_rect(DG_X, 156, 46, 22, UI_TXT); lcd_rect(DG_X + 2u, 158, 42, 18, UI_BG); lcd_rect(DG_X + 46u, 162, 3, 10, UI_TXT);
        if (f) lcd_rect(DG_X + 3u, 159, f, 16, c);
        if (f < 40u) lcd_rect(DG_X + 3u + f, 159, 40u - f, 16, UI_BG);
        b[0] = 0;
        if (bat_mv) { b_num(b, (uint32_t)bat_mv / 1000u); b_str(b, "."); b_num(b, (uint32_t)bat_mv / 100u % 10u); b_num(b, (uint32_t)bat_mv / 10u % 10u); b_str(b, " V"); }
        else b_str(b, "-- V");
        ft_box(62, 156, 96, b, &FT_MD, UI_TXT, UI_BG, 0);
        b[0] = 0;
        if (usb) b_str(b, "USB"); else if (pct >= 0) { b_num(b, (uint32_t)pct); b_str(b, "%"); } else b_str(b, "--");
        ft_box(158, 160, 74, b, &FT_SM, c, UI_BG, 2);
    }
    h = dg_mix(dg_mix(2166136261u, (uint32_t)bat_raw[0]), (uint32_t)bat_raw[1]);
    if (dg_changed(3, h)) {                                  /* the battery's raw reading (PC9) and the status bit beside it */
        int i = 0; b[0] = 0; b_str(b, "RAW  C9 "); while (b[i]) i++;
        i += r_str(b + i, bat_raw[0]); b[i] = 0; b_str(b, "  CH "); while (b[i]) i++; i += r_str(b + i, bat_raw[1]); b[i] = 0;
        ft_box(DG_X, 190, DG_W, b, &FT_SM, UI_DIM, UI_BG, 0);
    }
}
static void diag_frame(void)           /* the page's fixed parts */
{
    lcd_rect(0, 0, 240, 240, UI_BG);
    lcd_rect(0, 0, 240, 30, UI_DIM);
    ft_box(8, 4, 150, "DIAGNOSTICS", &FT_MD, BLK, UI_DIM, 0);
    {
        char b[16]; b[0] = 0;
        b_str(b, dg_page ? "2/2" : "1/2");
#ifdef OP_VERSION
        b_str(b, "  " OP_VERSION);
#endif
        ft_box(158, 8, 78, b, &FT_SM, BLK, UI_DIM, 2);
    }
    if (dg_page) {
        ft_box(DG_X, 36, DG_W, "INPUT (DB BELOW FULL SCALE)", &FT_SM, UI_DIM, UI_BG, 0);
        ft_box(DG_X, 94, DG_W, "OUTPUT", &FT_SM, UI_DIM, UI_BG, 0);
        ft_box(DG_X, 138, DG_W, "BATTERY", &FT_SM, UI_DIM, UI_BG, 0);
    }
    lcd_rect(0, F_Y, 240, 240u - F_Y, UI_PANEL);
    ft_box(0, F_Y + 3u, 240, dg_page ? "KNOB: PAGE 1   PHOTO BOTH PAGES" : "KNOB: PAGE 2   PHOTO BOTH PAGES", &FT_SM, UI_DIM, UI_PANEL, 1);
    for (int k = 0; k < 4; k++) dg_hash[k] = ~0u;
}
static void diag_open(void)
{
    ui_mode = M_DIAG;
    a_tmute = 0;                                              /* (the sound plays: the figures are of it) */
    dg_page = 0;
    dg_clip_seen = (uint32_t)bat_raw[3]; dg_clip_hold = 0;   /* (the marker lights for a clip from now on) */
    diag_frame();
}
static void diag_turn(void) { dg_page ^= 1u; diag_frame(); }
static void diag_close(void) { ui_mode = M_CHAIN; ui_item = 0; lcd_rect(0, 0, 240, 240, UI_BG); uc.screen = -1; ui_invalidate(); }
static void diag_draw(void) { if (dg_page) diag_power(); else diag_dsp(); }

/* ===================================================== controls ======== */
/* What each control does on each page, in one place: the main loop (openpocket.c) and the simulator (tests/edsim.c) give
 * ui_buttons the debounced buttons and ui_turn the knob; the simulator's named gestures call ui_action. */
enum { UB_FSUP, UB_FSDN, UB_PUSH, UB_HOME, UB_FX, UB_MOD, UB_AMP, UB_CAB, UB_DLY, UB_REV, UB_N };   /* a button: bit 1 << UB_x */
enum { UA_SAVE = UB_N, UA_MOVE, UA_TUNER, UA_MIDI, UA_FEET };   /* the gestures (a button's plain press is its UB_x) */
#define UI_HOLD_PUSH 800u              /* hold PUSH: SAVE */
#define UI_HOLD_HOME 700u              /* hold HOME: the tuner; held on as long again: the diagnostics */
#define UI_HOLD_FX   800u              /* hold FX: MIDI */
#define UI_CLICK2    350u              /* PUSH again within this, after a click opened a block from PLAY: MOVE that block */
#define UI_FEET      150u              /* the other footswitch within this: both, the tuner */
static struct {
    uint32_t held, t[UB_N], clk_t, fs_t;
    uint8_t fired[UB_N], clk, clk_blk;
    int8_t fs, fs_dir;                 /* the last footswitch pressed (-1: none) and the step it made */
} ug = {.fs = -1};
static void ui_action(int a)
{
#if PX_BASS
    static const uint8_t BLK_OF[5] = {PX_B_EQ, PX_B_AMP, PX_B_CAB, PX_B_PITCH, PX_B_COMP};   /* MOD: EQ, DLY: OCTAVE, REV: COMP */
#else
    static const uint8_t BLK_OF[5] = {PX_B_MOD, PX_B_AMP, PX_B_CAB, PX_B_DELAY, PX_B_REVERB};
#endif
    switch (ui_mode) {
    case M_SAVE: if (a == UB_PUSH) save_confirm(); else if (a == UB_HOME) save_close(); else if (a == UB_FX && sv_dest != SV_FACTORY) name_open(); return;
    case M_NAME: if (a == UB_PUSH) name_push(); else if (a == UB_HOME) name_done(0); return;
    case M_MIDI: if (a == UB_PUSH) midi_push(); else if (a == UB_HOME) midi_close(); return;
    case M_TUNER: if (a == UA_TUNER) diag_open(); else tuner_close(); return;   /* any control leaves; HOME held on: diagnostics */
    case M_DIAG: if (a != UA_TUNER) diag_close(); return;
    case M_MOVE: if (a == UB_PUSH) ui_move_drop(); else if (a == UB_HOME) ui_move_cancel(); return;
    }
    switch (a) {
    case UB_FSUP: ui_preset_step(+1); break;
    case UB_FSDN: ui_preset_step(-1); break;
    case UB_PUSH: if (ui_mode == M_LIST) list_load(); else ui_push(); break;
    case UB_HOME: if (ui_mode == M_CHAIN) list_open(); else if (ui_mode == M_LIST) list_close(); else ui_home(); break;
    case UB_FX: ui_fx_cycle(); break;
    case UA_SAVE: save_open(); break;
    case UA_MOVE: if (ui_mode == M_CHAIN && !ui_pvol) ui_move_start(); break;
    case UA_TUNER: case UA_FEET: tuner_open(); break;
    case UA_MIDI: midi_open(); break;
    default: ui_enter_block(BLK_OF[a - UB_MOD]);
    }
}
static void ui_turn(int dd, int fast)  /* the PARA knob, one detent (fast: turned quickly) */
{
    ug.clk = 0;
    if (ui_mode == M_SAVE) save_knob(dd);
    else if (ui_mode == M_NAME) name_knob(dd);
    else if (ui_mode == M_MIDI) midi_knob(dd);
    else if (ui_mode == M_LIST) list_knob(dd);
    else if (ui_mode == M_DIAG) diag_turn();
    else if (ui_mode != M_TUNER) ui_knob(dd, fast);
}
static void ui_press(int b, uint32_t now)   /* a button's plain press: at its press, PUSH HOME FX at a short release */
{
    if (b == UB_FSUP || b == UB_FSDN) {        /* v42: both footswitches = the tuner. The first acts at once, as always; the
                                                * other one soon after, while the first is still down, takes its step back
                                                * and opens the tuner */
        ug.clk = 0;
        if (ug.fs >= 0 && ug.fs != b && (ug.held >> ug.fs & 1u) && now - ug.fs_t < UI_FEET) {
            int d = ug.fs_dir;
            ug.fs = -1;
            if (d) { ui_preset_step(-d); ui_action(UA_FEET); }
            return;
        }
        ug.fs = (int8_t)b; ug.fs_t = now; ui_stepped = 0;
        ui_action(b);
        ug.fs_dir = (int8_t)ui_stepped;
        return;
    }
    if (b == UB_PUSH) {                        /* v42: a click on PLAY opens the block at once; a 2nd one soon after: MOVE it */
        if (ug.clk && ui_mode == M_BLOCK && ui_item == 0u && ui_blk == ug.clk_blk) {
            ug.clk = 0;
            ui_mode = M_CHAIN;
            ui_action(UA_MOVE);
            return;
        }
        int chain = ui_mode == M_CHAIN;
        ui_action(UB_PUSH);
        ug.clk = (uint8_t)(chain && ui_mode == M_BLOCK); ug.clk_t = now; ug.clk_blk = (uint8_t)ui_blk;
        return;
    }
    ug.clk = 0;
    ui_action(b);
}
static void ui_buttons(uint32_t now, uint32_t held)   /* every pass: which buttons are down (debounced), bit 1 << UB_x */
{
    uint32_t dn = held & ~ug.held, up = ug.held & ~held;
    ug.held = held;
    if (ug.clk && now - ug.clk_t >= UI_CLICK2) ug.clk = 0;
    for (int b = 0; b < UB_N; b++) {
        int timed = b == UB_PUSH || b == UB_HOME || b == UB_FX;   /* (these have a hold: they act at the release) */
        uint32_t hold = b == UB_PUSH ? UI_HOLD_PUSH : b == UB_HOME ? UI_HOLD_HOME : UI_HOLD_FX;
        if (dn >> b & 1u) { ug.t[b] = now; ug.fired[b] = 0; if (!timed) ui_press(b, now); }
        else if (timed && (held >> b & 1u) && (b == UB_HOME || !ug.fired[b]) && now - ug.t[b] >= hold) {
            ug.fired[b] = 1; ug.t[b] = now; ug.clk = 0;              /* (HOME: each further hold as long goes one page on) */
            ui_action(b == UB_PUSH ? UA_SAVE : b == UB_HOME ? UA_TUNER : UA_MIDI);
        }
        if ((up >> b & 1u) && timed && !ug.fired[b]) ui_press(b, now);
    }
}
