/* SPDX-License-Identifier: GPL-3.0-only OR MPL-2.0 */
/* The stand-ins that cannot be empty (as in ble_stubs.c): the low-power manager's registration, whose handle the library
 * asserts (a failed assertion never returns). The pedal never sleeps; the handle is only tested. */
#include <stdint.h>
void *low_power_get(void *priv, void *ops) { static uint8_t handle; (void)priv; (void)ops; return &handle; }

/* the radio's transmit trims the library keeps (analog.c), as in ble_stubs.c (ble_rf.c declares them int) */
int keep_BT_CMPUK1, keep_BT_CMPUK2, keep_BT_CMPUK3, keep_BT_OFSI, keep_BT_OFSQ;

/* The double-precision helpers the controller's AFH code calls (lmp.c: a channel map's percentage), which the firmware never
 * needed: the pedal's FPU is single precision. On bit patterns (a double is passed like a 64-bit integer here), rounding to
 * nearest (ties up). */
static int bt_clz64(uint64_t v) { int n = 0; if (!v) return 64; while (!(v >> 63)) { v <<= 1; n++; } return n; }
uint64_t __floatunsidf(uint32_t u)
{
    int e;
    uint64_t m;
    if (!u) return 0;
    e = 63 - bt_clz64(u);                                       /* the top bit */
    m = ((uint64_t)u << (52 - e)) & ((1ull << 52) - 1);
    return ((uint64_t)(1023 + e) << 52) | m;
}
uint64_t __floatsidf(int32_t i)
{
    return i < 0 ? __floatunsidf((uint32_t)-i) | (1ull << 63) : __floatunsidf((uint32_t)i);
}
uint64_t __muldf3(uint64_t a, uint64_t b)
{
    uint64_t s = (a ^ b) & (1ull << 63), ma, mb, hi, lo, p0, p1, p2, p3, mid;
    int ea = (int)((a >> 52) & 0x7FF), eb = (int)((b >> 52) & 0x7FF), e;
    if (!ea || !eb) return s;                                   /* zero (denormals: as zero) */
    ma = (a & ((1ull << 52) - 1)) | (1ull << 52);
    mb = (b & ((1ull << 52) - 1)) | (1ull << 52);
    /* the 106-bit product from four 32 x 32 */
    p0 = (ma & 0xFFFFFFFFu) * (mb & 0xFFFFFFFFu); p1 = (ma >> 32) * (mb & 0xFFFFFFFFu);
    p2 = (ma & 0xFFFFFFFFu) * (mb >> 32); p3 = (ma >> 32) * (mb >> 32);
    mid = (p0 >> 32) + (p1 & 0xFFFFFFFFu) + (p2 & 0xFFFFFFFFu);
    lo = (p0 & 0xFFFFFFFFu) | (mid << 32);
    hi = p3 + (p1 >> 32) + (p2 >> 32) + (mid >> 32);
    e = ea + eb - 1023;
    /* the product is in [2^104, 2^106): keep 53 bits */
    if (hi & (1ull << 41)) { ma = (hi << 11) | (lo >> 53); e++; if ((lo >> 52) & 1u) ma++; }   /* (to nearest) */
    else { ma = (hi << 12) | (lo >> 52); if ((lo >> 51) & 1u) ma++; }
    if (ma >> 53) { ma >>= 1; e++; }
    if (e <= 0) return s;
    if (e >= 0x7FF) return s | (0x7FFull << 52);
    return s | ((uint64_t)e << 52) | (ma & ((1ull << 52) - 1));
}
int32_t __fixdfsi(uint64_t a)
{
    int e = (int)((a >> 52) & 0x7FF) - 1023;
    uint64_t m = (a & ((1ull << 52) - 1)) | (1ull << 52);
    int32_t v;
    if (e < 0) return 0;
    if (e > 30) return (a >> 63) ? (int32_t)0x80000000 : 0x7FFFFFFF;
    v = (int32_t)(m >> (52 - e));
    return (a >> 63) ? -v : v;
}
uint32_t __fixunsdfsi(uint64_t a) { return (a >> 63) ? 0u : (uint32_t)__fixdfsi(a); }
