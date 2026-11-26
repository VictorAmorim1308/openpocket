/* SPDX-License-Identifier: GPL-3.0-only OR MPL-2.0 */
/* The stand-ins that cannot be empty (as in ble_stubs.c): the low-power manager's registration, whose handle the library
 * asserts (a failed assertion never returns). The pedal never sleeps; the handle is only tested. */
#include <stdint.h>
void *low_power_get(void *priv, void *ops) { static uint8_t handle; (void)priv; (void)ops; return &handle; }

/* the radio's transmit trims the library keeps (analog.c), as in ble_stubs.c (ble_rf.c declares them int) */
int keep_BT_CMPUK1, keep_BT_CMPUK2, keep_BT_CMPUK3, keep_BT_OFSI, keep_BT_OFSQ;
