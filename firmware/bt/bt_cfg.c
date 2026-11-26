/* SPDX-License-Identifier: Apache-2.0 */
/* Controller and host configuration of the Bluetooth audio image (OP_BT_AUDIO): the values the JieLi SDK's own
 * apps/common/config/log_config/lib_btctrler_config.c, lib_btstack_config.c and bt_profile_config.c (fw-AC79_AIoT_SDK at
 * the commit third_party/jieli-ble pins, Apache-2.0) give a classic-only build, reduced to one phone, the A2DP sink and
 * AVRCP (play / pause and the phone's volume). The names and types are the library's; the values for features we do not
 * build are the SDK's defaults or 0. */
#include <stdint.h>

#define BIT(n) (1u << (n))
enum { BT_MODULE_CLASSIC = BIT(0), BT_MODULE_LE = BIT(1) };
enum { BT_NORMAL = 0x01 };
enum { BT_BTSTACK_CLASSIC = BIT(0), BT_BTSTACK_LE_ADV = BIT(1), BT_BTSTACK_LE = BIT(2) };

const int config_btctler_modules = BT_MODULE_CLASSIC;
int config_btctler_mode = BT_NORMAL;
int config_btctler_hci_standard = 0;
const int config_bt_function = 0;
const int config_btctler_le_tws = 0;
const int CONFIG_BTCTLER_TWS_ENABLE = 0;
const int CONFIG_TWS_AFH_ENABLE = 0;
const int CONFIG_WIFI_DETECT_ENABLE = 0;
const int ESCO_FORWARD_ENABLE = 0;
const int CONFIG_LOW_LATENCY_ENABLE = 0;
const int CONFIG_ESCO_MUX_RX_BULK_ENABLE = 0;
const int CONFIG_BTCTLER_FAST_CONNECT_ENABLE = 0;
const int CONFIG_LMP_NAME_REQ_ENABLE = 1;
const int CONFIG_LMP_PASSKEY_ENABLE = 1;
const int CONFIG_LMP_MASTER_ESCO_ENABLE = 0;                   /* no calls (eSCO): the SDK's 1 only matters with HFP */
const int CONFIG_INQUIRY_PAGE_OFFSET_ADJUST = 0;
const int config_force_bt_pwr_tab_using_normal_level = 0;
unsigned char config_btctler_bredr_master = 0;
const int config_btctler_dual_a2dp = 0;
const int config_btctler_role_switch_fail_disconnect = 1;
const int config_btctler_role_switch_fail_back_to_slave = 0;
const int config_bredr_afh_user = 0;
const uint8_t rx_fre_offset_adjust_enable = 1;
const int config_fix_fre_enable = 0;
const int ble_disable_wait_enable = 1;
const int config_delete_link_key = 1;                          /* a phone that lost the bond pairs again */
const int config_btstask_auto_exit_sniff = 1;
const int CONFIG_TEST_DUT_CODE = 0;                            /* the SDK's 1: the factory's test modes, not built */
const int CONFIG_TEST_FCC_CODE = 0;
const int CONFIG_TEST_DUT_ONLY_BOX_CODE = 0;
const int CONFIG_BREDR_INQUIRY = 0;
const int CONFIG_TWS_SUPER_TIMEOUT = 0;

/* LE: not built (config_btctler_modules); the values the shared members read */
const uint64_t config_btctler_le_features = 0;
const int config_btctler_le_roles = 0;
const int config_btctler_le_afh_en = 0;
const int config_btctler_le_hw_ram_use_static = 1;
const int config_btctler_le_master_multilink = 0;
const int config_multilink_support_esco_enalbe = 0;
const int config_btctler_le_slave_multilink = 0;
const int config_btctler_le_hw_nums = 0;
const int config_btctler_le_rx_nums = 0;
const int config_btctler_le_acl_packet_length = 27;
const int config_btctler_le_acl_total_nums = 0;
const uint32_t config_vendor_le_bb = 0;
const int config_btctler_le_slave_conn_update_winden = 2500;
const int config_btctler_single_carrier_en = 0;
const int sniff_support_reset_anchor_point = 0;
const int config_bt_security_vulnerability = 0;
const int config_le_hci_connection_num = 0;
const int config_le_sm_support_enable = 0;
const int config_le_gatt_server_num = 0;
const int config_le_gatt_client_num = 0;
const int config_le_sm_sub_sc_enable = 0;
const int config_le_sm_sub_sc_bridge_edr_enable = 0;

/* the host (lib_btstack_config.c, bt_profile_config.c) */
const int config_stack_modules = BT_BTSTACK_CLASSIC;
const int CONFIG_BTSTACK_BIG_FLASH_ENABLE = 1;
const int CONFIG_BTSTACK_SUPPORT_AAC = 0;                      /* SBC only: the phone then always sends SBC */
const int config_rcsp_stack_enable = 0;
const int config_update_mode = 0;
uint8_t a2dp_profile_support = 1;                              /* the A2DP sink */
uint8_t acp_profile_support = 1;                               /* AVRCP: play / pause, and the phone's volume */
const uint8_t a2dp_mutual_support = 0;                         /* one phone */
const uint8_t more_addr_reconnect_support = 0;
const uint8_t more_avctp_cmd_support = 1;
const uint8_t hci_inquiry_support = 0;
const uint8_t btstack_emitter_support = 0;
const uint8_t adt_profile_support = 0;
const uint8_t pbg_support_enable = 0;
const uint8_t more_hfp_cmd_support = 0;
const uint8_t sdp_get_remote_pnp_info = 0;
const uint8_t hid_conn_depend_on_dev_company = 0;

/* how much SBC the controller keeps before it tells the host to slow down or catch up (lmp_get_a2dp_data_cache_*: read
 * by the decoder side, which here is OpenPocket's own: bt_a2dp.c keeps its own fill level) */
const int CONFIG_A2DP_DATA_CACHE_LOW_SBC = 1500;
const int CONFIG_A2DP_DATA_CACHE_HI_SBC = 5000;
const int CONFIG_A2DP_DATA_CACHE_LOW_AAC = 1500;
const int CONFIG_A2DP_DATA_CACHE_HI_AAC = 5000;

/* the service records a phone reads (SDP): the A2DP sink, and AVRCP as controller and target (the target is what lets the
 * phone send its volume). The records themselves are the library's (sdp_util.c); the table is collected by firmware/app.ld. */
typedef struct { void *offset_item; uint32_t service_record_handle; const uint8_t *service_record; } service_record_item_t;
extern const uint8_t sdp_a2dp_service_data[], sdp_avctp_ct_service_data[], sdp_avctp_ta_service_data[];
#define SDP_RECORD(name) const service_record_item_t name __attribute__((used, section(".sdp_record_item")))
SDP_RECORD(a2dp_sdp_record_item) = { 0, 0x00010001, sdp_a2dp_service_data };
SDP_RECORD(arp_ct_sdp_record_item) = { 0, 0x00010002, sdp_avctp_ct_service_data };
SDP_RECORD(arp_ta_sdp_record_item) = { 0, 0x00010005, sdp_avctp_ta_service_data };

/* objects that only the tables above reach (the linker takes an archive member only for a name it needs) */
extern const uint8_t a2dp_sink_event_handler[], acp_a2dp_event_handler[];
const void *const bt_keep[] __attribute__((used)) = { a2dp_sink_event_handler, acp_a2dp_event_handler };
