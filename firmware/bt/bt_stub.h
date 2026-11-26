/* SPDX-License-Identifier: GPL-3.0-only OR MPL-2.0 */
/* The stand-ins' counters (bt_stubs_gen.c), with the names ble_test.c and the status line already read (ble_stubs.c's). */
#pragma once
#include <stdint.h>

uint32_t ble_stub_calls;
const char *ble_stub_last, *ble_stub_seen[12];                  /* the different ones called, in order: the trace line */
#define BT_STUB(name) int name(void) { ble_stub_calls++; ble_stub_last = #name; return 0; }
static void ble_stub_collect(void)                              /* from the main loop */
{
    const char *name = ble_stub_last;
    uint32_t i;
    for (i = 0; i < 12u; i++) {
        if (ble_stub_seen[i] == name) return;
        if (!ble_stub_seen[i]) { ble_stub_seen[i] = name; return; }
    }
}
