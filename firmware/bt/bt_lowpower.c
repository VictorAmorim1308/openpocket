/* SPDX-License-Identifier: GPL-3.0-only OR MPL-2.0 */
/* The stand-ins that cannot be empty (as in ble_stubs.c): the low-power manager's registration, whose handle the library
 * asserts (a failed assertion never returns). The pedal never sleeps; the handle is only tested. */
#include <stdint.h>
void *low_power_get(void *priv, void *ops) { static uint8_t handle; (void)priv; (void)ops; return &handle; }
