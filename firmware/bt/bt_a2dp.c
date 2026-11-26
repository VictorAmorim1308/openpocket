/* SPDX-License-Identifier: GPL-3.0-only OR MPL-2.0 */
/* OpenPocket Bass, Bluetooth audio (OP_BT_AUDIO): the phone side of JieLi's classic stack (third_party/jieli-bt). The pedal
 * is a speaker a phone pairs with ("OpenPocket Bass", no PIN): A2DP sink for the music, AVRCP for play / pause and the
 * phone's volume. This file starts the host with the SDK example's settings (apps/common/example/bluetooth/edr/bt_music),
 * follows its events, and takes the music's packets out of the stack (bt_audio_poll, from the main loop): the SBC frames go
 * to bt_music.h, which decodes them into the music ring the audio interrupt mixes. Takes the place of ble_midi.c in the
 * Bluetooth test image: the same names for the status line (ble_conn, ble_midi_line, ble_trace_line). */
#include <stdint.h>
#include "bt_stub.h"

/* the stack's API (include_lib/btstack/avctp_user.h, a2dp_media_codec.h; values checked against the SDK's enums) */
enum { USER_CTRL_WRITE_SCAN_DISABLE = 12, USER_CTRL_WRITE_SCAN_ENABLE = 13, USER_CTRL_WRITE_CONN_DISABLE = 14,
       USER_CTRL_WRITE_CONN_ENABLE = 15, USER_CTRL_AVCTP_OPID_PLAY = 50, USER_CTRL_AVCTP_OPID_PAUSE = 51 };
enum { BT_STATUS_INIT_OK = 3, BT_STATUS_START_CONNECTED = 5, BT_STATUS_FIRST_CONNECTED = 6, BT_STATUS_ENCRY_COMPLETE = 8,
       BT_STATUS_FIRST_DISCONNECT = 9, BT_STATUS_AVRCP_INCOME_OPID = 37, BT_STATUS_CONN_A2DP_CH = 39,
       BT_STATUS_A2DP_MEDIA_START = 48, BT_STATUS_A2DP_MEDIA_STOP = 49 };
enum { BT_EV_CON_STATUS = 3 };                                  /* bt_event_update_to_user's type for the BT_STATUS_* above */
extern uint32_t user_send_cmd_prepare(int cmd, uint16_t len, uint8_t *param);
extern void __set_simple_pair_flag(int on);
extern void __set_simple_pair_param(uint8_t io_cap, uint8_t oob, uint8_t mitm);
extern void __set_a2dp_auto_play_flag(uint8_t on);
extern void __set_support_msbc_flag(int on);
extern void __set_support_aac_flag(int on);
extern void __bt_set_update_battery_time(uint8_t s);
extern void __set_page_timeout_value(uint16_t ms);
extern void __set_super_timeout_value(uint16_t ms);
extern void music_vol_change_handle_register(void (*set)(int vol), int (*get)(void));
extern int a2dp_media_try_get_packet(uint8_t **frame);
extern void a2dp_media_free_packet(void *packet);
extern int a2dp_media_get_packet_num(void);

struct bt_event { uint8_t event; uint8_t args[7]; uint32_t value; };   /* the library's struct bt_event (12 bytes) */

static uint16_t ble_conn;                     /* 1: a phone is connected (the name the test image's status code reads) */
static volatile uint8_t bt_ready, bt_playing, bt_a2dp;
static volatile int bt_phone_vol = 100;       /* the phone's volume (AVRCP absolute volume), 0..100 */
static uint32_t bt_links, bt_pkts, bt_bytes, bt_bad, bt_last_ev, bt_evs;
static uint8_t bt_peer[6];

/* the name and the address the library asks for (hci_vendor.c: weak defaults) */
const char *bt_get_local_name(void) { return "OpenPocket Bass"; }
const uint8_t *bt_get_mac_addr(void)
{   /* a locally administered address; TODO: from the unit's calibration record, so that two pedals differ */
    static const uint8_t a[6] = {0x02, 0x4F, 0x50, 0x42, 0x41, 0x53};
    return a;
}

#define BT_MUSIC_PEDAL 80                       /* the pedal's own music level (0..100), until it has a setting: x0.64 */
static void bt_vol_set(int vol)                  /* the phone moved its volume (AVRCP absolute volume): 0..127 */
{
    if (vol < 0 || vol > 127) return;
    bt_phone_vol = vol * 100 / 127;
    bt_music_level(bt_phone_vol, BT_MUSIC_PEDAL);
}
static int bt_vol_get(void) { return bt_phone_vol * 127 / 100; }

/* before btstack_init (ble_test.c), as bt_function_select_init + bredr_handle_register in the SDK example */
static void bt_pre_init(void)
{
    __set_a2dp_auto_play_flag(0);                /* the phone decides when to play */
    __set_support_msbc_flag(0);
    __set_support_aac_flag(0);
    __bt_set_update_battery_time(0);
    __set_page_timeout_value(8000);
    __set_super_timeout_value(8000);
    __set_simple_pair_flag(1);                   /* Secure Simple Pairing, no PIN */
    __set_simple_pair_param(3, 0, 2);            /* NoInputNoOutput, no OOB, the SDK's authentication requirements */
    music_vol_change_handle_register(bt_vol_set, bt_vol_get);
    bt_music_level(bt_phone_vol, BT_MUSIC_PEDAL);
}

static void bt_visible(int on)
{
    user_send_cmd_prepare(on ? USER_CTRL_WRITE_SCAN_ENABLE : USER_CTRL_WRITE_SCAN_DISABLE, 0, 0);
    user_send_cmd_prepare(on ? USER_CTRL_WRITE_CONN_ENABLE : USER_CTRL_WRITE_CONN_DISABLE, 0, 0);
}

static void bt_post_init(void) { }               /* (visible once the host says INIT_OK: bt_event_notify) */

/* every event the host passes to the application (user_interface.c bt_event_update_to_user) */
int bt_event_notify(uint32_t type, struct bt_event *e)
{
    bt_evs++;
    bt_last_ev = (type << 16) | e->event;
    if (type != BT_EV_CON_STATUS) return 0;
    switch (e->event) {
    case BT_STATUS_INIT_OK:
        bt_ready = 1; ble_state = BLE_ADV; bt_visible(1); break;
    case BT_STATUS_FIRST_CONNECTED:
        ble_conn = 1; bt_links++;
        { int i; for (i = 0; i < 6; i++) bt_peer[i] = e->args[i]; }
        break;
    case BT_STATUS_FIRST_DISCONNECT:
        ble_conn = 0; bt_a2dp = bt_playing = 0; bt_visible(1); break;
    case BT_STATUS_CONN_A2DP_CH: bt_a2dp = 1; break;
    case BT_STATUS_A2DP_MEDIA_START: bt_playing = 1; break;
    case BT_STATUS_A2DP_MEDIA_STOP: bt_playing = 0; break;
    default: break;
    }
    return 0;
}

/* the RTP header in front of the SBC frames (bt_decode.c get_rtp_header_len: 12 bytes, the CSRCs and the extension, then the
 * SBC media payload header's byte): the first SBC sync word (0x9C) after it */
static int bt_rtp_skip(const uint8_t *p, int len)
{
    int n;
    if (len < 13) return len;
    n = 12 + (p[0] & 0x0F) * 4;
    if (p[0] & 0x10) { if (n + 4 > len) return len; n += 4 + ((p[n + 2] << 8) | p[n + 3]) * 4; }
    n++;                                         /* the media payload header (fragmentation, the number of frames) */
    while (n < len && p[n] != 0x9C) n++;
    return n;
}

/* the main loop's turn: every packet the stack holds, to the music ring */
static void bt_audio_poll(void)
{
    uint8_t *f;
    int len, k = 0;
    while (k++ < 8 && (len = a2dp_media_try_get_packet(&f)) > 0 && f) {
        int at = bt_rtp_skip(f, len);
        bt_pkts++;
        if (at < len) { bt_bytes += (uint32_t)(len - at); bt_music_sbc(f + at, len - at); }
        else bt_bad++;
        a2dp_media_free_packet(f);
    }
}

static char *bt__put(char *o, const char *s) { while (*s) *o++ = *s++; return o; }
static char *bt__num(char *o, uint32_t v) { char t[11]; int n = 0; do { t[n++] = (char)('0' + v % 10u); v /= 10u; } while (v); while (n) *o++ = t[--n]; return o; }
static char *bt__hex(char *o, uint32_t v, int d) { while (d--) *o++ = "0123456789ABCDEF"[(v >> (d * 4)) & 15u]; return o; }

/* the status line while visible or connected (ble_test.c scrolls it; the log channel's third line) */
void ble_midi_line(char *out)
{
    char *o = out;
    o = bt__put(o, ble_conn ? (bt_playing ? "PLAYING" : "CONNECTED") : bt_ready ? "VISIBLE" : "STARTING");
    o = bt__put(o, " L"); o = bt__num(o, bt_links);
    o = bt__put(o, " P"); o = bt__num(o, bt_pkts);
    o = bt__put(o, " B"); o = bt__num(o, bt_bytes >> 10);
    o = bt__put(o, "K X"); o = bt__num(o, bt_bad);
    o = bt__put(o, " V"); o = bt__num(o, (uint32_t)bt_phone_vol);
    o = bt__put(o, " "); o = bt_music_text(o);
    *o = 0;
}

void ble_trace_line(char *out)
{
    char *o = out;
    o = bt__put(o, "EV "); o = bt__num(o, bt_evs); o = bt__put(o, " "); o = bt__hex(o, bt_last_ev, 6);
    o = bt__put(o, " ST "); o = bt__num(o, ble_stub_calls);
    if (ble_stub_last) { o = bt__put(o, " "); o = bt__put(o, ble_stub_last); }
    *o = 0;
}
