/**
 * @file bt_task.c
 * @brief
 * @author JoyLee
 * @version 2.0.0
 * @date 2023-04-06
 *
 * @copyright Copyright (c) 2023 Westberry Technology Corp., Ltd
 */

#include "common/bts_lib.h"
#include "config.h"
#include "gpio.h"
#include "m78.h"
#include "quantum.h"
#include "rgb_matrix.h"
#include "uart.h"
#include "usb_main.h"
#include "common/bt_task.h"

#ifdef BT_DEBUG_MODE
#    define BT_DEBUG_INFO(fmt, ...) dprintf(fmt, ##__VA_ARGS__)
#else
#    define BT_DEBUG_INFO(fmt, ...)
#endif

// ===========================================
// 函数声明
// ===========================================
static void long_pressed_keys_hook(void);
static void long_pressed_keys_cb(uint16_t keycode);
static bool process_record_other(uint16_t keycode, keyrecord_t *record);
static void bt_scan_mode(void);
static void bt_used_pin_init(void);
static void handle_blink_effects(void);
static void handle_charging_indication(void);
static void handle_low_battery_warning(void);
static void handle_low_battery_shutdow(void);
static void handle_battery_query_display(void);
static void handle_bt_indicate_led(void);
static void handle_usb_indicate_led(void);
#ifdef RGB_MATRIX_ENABLE
static void led_off_standby(void);
static void open_rgb(void);
static void close_rgb(void);
#endif

// Helper functions for better code organization
static bool    is_bt_device(uint8_t device);
static bool    validate_device_type(uint8_t device);
static void    reset_bt_connection_state(void);
static void    send_device_vendor_command(uint8_t device);
static uint8_t keycode_to_device_type(uint16_t keycode);

extern keymap_config_t keymap_config;

// ===========================================
// 常量定义
// ===========================================
/* Wireless connection timing constants */
#define WL_CONN_TIMEOUT_MS (30 * 1000)           // 30 seconds
#define WL_PAIR_TIMEOUT_MS ((1 * 60 - 7) * 1000) // 53 seconds
#define WL_PAIR_INTVL_MS (200)                   // 5Hz blink for pairing
#define WL_CONN_INTVL_MS (500)                   // 2Hz blink for connecting
#define WL_CONNECTED_LAST_MS (3 * 1000)          // Show connected status for 3s
#define USB_CONNECTED_LAST_MS (3 * 1000)

/* Sleep and standby timeouts */
#define LED_OFF_STANDBY_TIMEOUT_MS ((5 * 60 - 45) * 1000) // 5 minutes
#define ENTRY_SLEEP_TIMEOUT_MS (30 * 60 * 1000)           // 30 minutes

/* Array size calculations */
#define NUM_LONG_PRESS_KEYS (sizeof(long_pressed_keys) / sizeof(long_pressed_keys_t))

/* Hardware validation helpers */
#define IS_BT_DEVICE(dev) ((dev) >= DEVS_HOST1 && (dev) <= DEVS_HOST3)
#define IS_VALID_DEVICE(dev) ((dev) >= DEVS_USB && (dev) <= DEVS_2_4G)

// ===========================================
// 结构体定义
// ===========================================
typedef struct {
    uint32_t press_time;
    uint16_t keycode;
    void (*event_cb)(uint16_t);
} long_pressed_keys_t;

// ===========================================
// 全局变量
// ===========================================
uint32_t   bt_init_time = 0;
dev_info_t dev_info     = {0};
bts_info_t bts_info     = {
        .bt_name        = {"YIGIIX GK81_BT$", "YIGIIX GK81_BT$", "YIGIIX GK81_BT$"},
        .uart_init      = uart_init,
        .uart_read      = uart_read,
        .uart_transmit  = uart_transmit,
        .uart_receive   = uart_receive,
        .uart_available = uart_available,
        .timer_read32   = timer_read32,
};

// clang-format off
// 长按键配置
long_pressed_keys_t long_pressed_keys[] = {
    {.keycode = BT_HOST1, .press_time = 0, .event_cb = long_pressed_keys_cb},
    {.keycode = BT_HOST2, .press_time = 0, .event_cb = long_pressed_keys_cb},
    {.keycode = BT_HOST3, .press_time = 0, .event_cb = long_pressed_keys_cb},
    {.keycode = BT_2_4G, .press_time = 0, .event_cb = long_pressed_keys_cb},
    {.keycode = SLEEP_TOGGLE, .press_time = 0, .event_cb = long_pressed_keys_cb},
    {.keycode = RGB_TEST, .press_time = 0, .event_cb = long_pressed_keys_cb},
};
// clang-format on
// 指示器状态
indicator_state_t indicator_status          = INDICATOR_CONNECTING;
bool              indicator_reset_last_time = false;

// RGB控制
static uint32_t key_press_time;
static uint32_t close_rgb_time;
static bool     rgb_status_save = false;
static bool     bak_rgb_toggle  = false;
static bool     sober           = true;
bool            kb_sleep_flag   = false;
extern bool     led_inited;
extern void     led_config_all(void);
extern void     led_deconfig_all(void);

// 设备指示配置
static const uint8_t rgb_index_table[] = {USB_INDEX, BT_HOST1_INDEX, BT_HOST2_INDEX, BT_HOST3_INDEX, RF_INDEX};
// clang-format off
static const uint8_t rgb_index_color_table[][3] = {
    {100, 100, 100},
    {0, 0, 100},
    {0, 0, 100},
    {0, 0, 100},
    {0, 100, 0},
};
// clang-format on

// RGB测试相关
static const uint8_t rgb_test_color_table[][3] = {
    {100, 0, 0},
    {0, 100, 0},
    {0, 0, 100},
    {100, 100, 100},
};
static uint8_t  rgb_test_index = 0;
static bool     rgb_test_en    = false;
static uint32_t rgb_test_time  = 0;

// 闪烁效果相关
static uint8_t  all_blink_cnt;
static uint32_t all_blink_time;
static RGB      all_blink_color;
static uint8_t  single_blink_cnt;
static uint8_t  single_blink_index;
static RGB      single_blink_color;
static uint32_t single_blink_time;

// 电量查询
static bool query_vol_flag       = false;
static bool query_vol_processing = false;
/* 电量显示LED */
uint8_t query_index[10] = {26, 25, 24, 23, 22, 21, 20, 19, 18, 17};

// USB相关
uint32_t USB_switch_time;
uint8_t  USB_blink_cnt;

uint32_t last_total_time = 0;

// ===========================================
// 线程定义
// ===========================================
static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION(Thread1, arg) {
    (void)arg;
    chRegSetThreadName("blinker");
    while (true) {
        bts_task(dev_info.devs);
        chThdSleepMilliseconds(1);
    }
}

// ===========================================
// 初始化函数
// ===========================================
void bt_init(void) {
    bts_init(&bts_info);
    bt_used_pin_init();

    // 读取用户配置
    dev_info.raw = eeconfig_read_user();
    if (!dev_info.raw) {
        dev_info.devs      = DEVS_USB;
        dev_info.last_devs = DEVS_HOST1;
        eeconfig_update_user(dev_info.raw);
    }

    bt_init_time = timer_read32();
    chThdCreateStatic(waThread1, sizeof(waThread1), HIGHPRIO, Thread1, NULL);
    bt_scan_mode();

    if (dev_info.devs != DEVS_USB) {
        usbDisconnectBus(&USB_DRIVER);
        usbStop(&USB_DRIVER);
    }

    setPinOutput(A14);
    if (dev_info.devs == DEVS_USB) {
        writePinLow(A14);
    } else {
        writePinHigh(A14);
    }

    rgb_status_save = rgb_matrix_config.enable;
}

// ===========================================
// 蓝牙任务函数
// ===========================================
void bt_task(void) {
    static uint32_t last_time = 0;

    // Handle initialization sequence after delay
    if ((bt_init_time != 0) && (timer_elapsed32(bt_init_time) >= BT_INIT_DELAY_MS)) {
        bt_init_time = 0;

        bts_send_name(DEVS_HOST1);
        bts_send_vendor(v_en_sleep_bt);
        bts_send_vendor(v_en_sleep_wl);

        // Send appropriate vendor command for current device
        send_device_vendor_command(dev_info.devs);

        // Fallback to USB if invalid device
        if (!validate_device_type(dev_info.devs)) {
            dev_info.devs = DEVS_USB;
            eeconfig_update_user(dev_info.raw);
        }
    }

    // Update task at regular intervals
    if (timer_elapsed32(last_time) >= TASK_UPDATE_INTERVAL_MS) {
        last_time = timer_read32();

        if (dev_info.devs != DEVS_USB) {
            uint8_t keyboard_led_state = 0;
            led_t  *kb_leds            = (led_t *)&keyboard_led_state;
            kb_leds->raw               = bts_info.bt_info.indictor_rgb_s;
            usb_device_state_set_leds(keyboard_led_state);

#ifdef RGB_MATRIX_ENABLE
            if (dev_info.sleep_mode_enabled || bts_info.bt_info.low_vol) {
                close_rgb();
            }
#endif
        }
    }

    long_pressed_keys_hook();
    bt_scan_mode();
}

// ===========================================
// 按键处理函数
// ===========================================
bool process_record_bt(uint16_t keycode, keyrecord_t *record) {
    bool retval = true;

    if (record->event.pressed) {
        BT_DEBUG_INFO("\n\nkeycode = [0x%x], pressed time: [%d]\n\n", keycode, record->event.time);
        BT_DEBUG_INFO("\n devs     = [%d] \
                    \n sleeped       = [%d] \
                    \n low_vol       = [%d] \
                    \n low_vol_offed = [%d] \
                    \n normal_vol    = [%d] \
                    \n pairing       = [%d] \
                    \n paired        = [%d] \
                    \n come_back     = [%d] \
                    \n come_back_err = [%d] \
                    \n mode_switched = [%d] \
                    \n pvol          = [%d]\n\n\n",
                      dev_info.devs, bts_info.bt_info.sleeped, bts_info.bt_info.low_vol, bts_info.bt_info.low_vol_offed, bts_info.bt_info.normal_vol, bts_info.bt_info.pairing, bts_info.bt_info.paired, bts_info.bt_info.come_back, bts_info.bt_info.come_back_err, bts_info.bt_info.mode_switched, bts_info.bt_info.pvol);

        if (!rgb_matrix_config.enable) {
            if (rgb_status_save) {
                rgb_matrix_enable_noeeprom();
            }
        }
    }

    retval = process_record_other(keycode, record);

    if (dev_info.devs != DEVS_USB) {
        if (retval != false) {
            while (bts_is_busy()) {
                wait_ms(1);
            }
            if ((keycode > QK_MODS) && (keycode <= QK_MODS_MAX)) {
                if (QK_MODS_GET_MODS(keycode) & 0x1) {
                    if (QK_MODS_GET_MODS(keycode) & 0x10)
                        bts_process_keys(KC_RCTL, record->event.pressed, dev_info.devs, keymap_config.no_gui);
                    else
                        bts_process_keys(KC_LCTL, record->event.pressed, dev_info.devs, keymap_config.no_gui);
                }
                if (QK_MODS_GET_MODS(keycode) & 0x2) {
                    if (QK_MODS_GET_MODS(keycode) & 0x10)
                        bts_process_keys(KC_RSFT, record->event.pressed, dev_info.devs, keymap_config.no_gui);
                    else
                        bts_process_keys(KC_LSFT, record->event.pressed, dev_info.devs, keymap_config.no_gui);
                }
                if (QK_MODS_GET_MODS(keycode) & 0x4) {
                    if (QK_MODS_GET_MODS(keycode) & 0x10)
                        bts_process_keys(KC_RALT, record->event.pressed, dev_info.devs, keymap_config.no_gui);
                    else
                        bts_process_keys(KC_LALT, record->event.pressed, dev_info.devs, keymap_config.no_gui);
                }
                if (QK_MODS_GET_MODS(keycode) & 0x8) {
                    if (QK_MODS_GET_MODS(keycode) & 0x10)
                        bts_process_keys(KC_RGUI, record->event.pressed, dev_info.devs, keymap_config.no_gui);
                    else
                        bts_process_keys(KC_LGUI, record->event.pressed, dev_info.devs, keymap_config.no_gui);
                }
                retval = bts_process_keys(QK_MODS_GET_BASIC_KEYCODE(keycode), record->event.pressed, dev_info.devs, keymap_config.no_gui);
            } else {
                retval = bts_process_keys(keycode, record->event.pressed, dev_info.devs, keymap_config.no_gui);
            }
        }
    }

#ifdef RGB_MATRIX_ENABLE
    open_rgb();
#endif

    return retval;
}

// ===========================================
// 设备切换函数
// ===========================================
void bt_switch_mode(uint8_t last_mode, uint8_t now_mode, uint8_t reset) {
    // Validate device types
    if (!validate_device_type(last_mode) || !validate_device_type(now_mode)) {
        return;
    }

    // Enable RGB if it was previously enabled
    if (!rgb_matrix_config.enable && rgb_status_save) {
        rgb_matrix_enable_noeeprom();
    }

    // Handle USB driver state changes
    bool usb_sws = !!last_mode ? !now_mode : !!now_mode;
    if (usb_sws) {
        if (!!now_mode) {
            usbDisconnectBus(&USB_DRIVER);
            usbStop(&USB_DRIVER);
        } else {
            init_usb_driver(&USB_DRIVER);
        }
    }

    // Reset wireless indicator timer if switching between different devices
    if (dev_info.devs != dev_info.last_devs) {
        extern uint32_t last_total_time;
        last_total_time = timer_read32();
    }

    // Update device state
    dev_info.devs = now_mode;
    if (dev_info.devs != DEVS_USB) {
        dev_info.last_devs = dev_info.devs;
    } else {
        USB_switch_time = timer_read32();
        USB_blink_cnt   = 0;
    }

    // Set hardware control pin
    if (dev_info.devs == DEVS_USB) {
        writePinLow(A14);
    } else {
        writePinHigh(A14);
    }

    // Reset BT connection state
    reset_bt_connection_state();
    eeconfig_update_user(dev_info.raw);

    // Handle indicator status and send vendor commands
    if (is_bt_device(dev_info.devs) || dev_info.devs == DEVS_2_4G) {
        if (reset) {
            indicator_status          = INDICATOR_PAIRING; // Pairing mode
            indicator_reset_last_time = true;
            bts_send_vendor(v_pair);
        } else {
            indicator_status          = INDICATOR_CONNECTED; // Connecting mode
            indicator_reset_last_time = true;
            send_device_vendor_command(dev_info.devs);
        }
    } else if (dev_info.devs == DEVS_USB) {
        indicator_status          = INDICATOR_CONNECTED;
        indicator_reset_last_time = true;
        bts_send_vendor(v_usb);
    }
}

// ===========================================
// 其他按键处理
// ===========================================
static bool process_record_other(uint16_t keycode, keyrecord_t *record) {
    // 更新长按键时间
    for (uint8_t i = 0; i < NUM_LONG_PRESS_KEYS; i++) {
        if (keycode == long_pressed_keys[i].keycode) {
            if (record->event.pressed) {
                long_pressed_keys[i].press_time = timer_read32();
            } else {
                long_pressed_keys[i].press_time = 0;
            }
            break;
        }
    }

    // 硬件开关检查
    if (readPin(BT_MODE_SW_PIN)) {
        if (keycode >= BT_HOST1 && keycode <= BT_HOST3) {
            return false;
        }
    }
    if (readPin(RF_MODE_SW_PIN)) {
        if (keycode == BT_2_4G) {
            return false;
        }
    }
    if (!readPin(BT_MODE_SW_PIN) || !readPin(RF_MODE_SW_PIN)) {
        if (keycode == BT_USB || keycode == BT_2_4G) {
            return false;
        }
    }

    switch (keycode) {
        case BT_HOST1:
        case BT_HOST2:
        case BT_HOST3:
        case BT_2_4G:
        case BT_USB: {
            if (record->event.pressed) {
                uint8_t target_devs = keycode_to_device_type(keycode);
                if (dev_info.devs != target_devs) {
                    bt_switch_mode(dev_info.devs, target_devs, false);
                }
            }
        } break;

        case BT_VOL: {
            if (record->event.pressed) {
                bts_send_vendor(v_query_vol);
                query_vol_flag = true;
            } else {
                query_vol_flag = false;
            }
        } break;

        case SW_OS: {
            if (record->event.pressed) {
                if (get_highest_layer(default_layer_state) == 0) {
                    set_single_persistent_default_layer(2);
                    keymap_config.no_gui = 0;
                    eeconfig_update_keymap(&keymap_config);
                    all_blink_cnt   = 6;
                    all_blink_color = (RGB){0, 0, 100};
                    all_blink_time  = timer_read32();
                } else if (get_highest_layer(default_layer_state) == 2) {
                    set_single_persistent_default_layer(0);
                    all_blink_cnt   = 6;
                    all_blink_color = (RGB){100, 100, 100};
                    all_blink_time  = timer_read32();
                }
            }
        } break;

        case RGB_TEST: {
            if (record->event.pressed) {
                if (rgb_test_en) {
                    rgb_test_en    = false;
                    rgb_test_index = 0;
                }
            }
        } break;

        default:
            return true;
    }

    return false;
}

// ===========================================
// 长按键处理
// ===========================================
static void long_pressed_keys_cb(uint16_t keycode) {
    switch (keycode) {
        case BT_HOST1:
        case BT_HOST2:
        case BT_HOST3:
        case BT_2_4G: {
            uint8_t target_dev = keycode - BT_HOST1 + DEVS_HOST1;
            if (dev_info.devs == target_dev) {
                bt_switch_mode(dev_info.devs, target_dev, true);
            }
        } break;

        case SLEEP_TOGGLE: {
            if (dev_info.sleep_mode_enabled) {
                dev_info.sleep_mode_enabled = false;
                bts_send_vendor(v_en_sleep_bt);
                bts_send_vendor(v_en_sleep_wl);
                all_blink_cnt   = 6;
                all_blink_color = (RGB){100, 100, 100};
                all_blink_time  = timer_read32();
            } else {
                dev_info.sleep_mode_enabled = true;
                bts_send_vendor(v_dis_sleep_bt);
                bts_send_vendor(v_dis_sleep_wl);
                all_blink_cnt   = 6;
                all_blink_color = (RGB){0, 100, 0};
                all_blink_time  = timer_read32();
            }
            eeconfig_update_user(dev_info.raw);
        } break;

        case RGB_TEST: {
            if (rgb_test_en != true) {
                rgb_test_en    = true;
                rgb_test_index = 1;
                rgb_test_time  = timer_read32();
            }
        } break;

        default:
            break;
    }
}

static void long_pressed_keys_hook(void) {
    for (uint8_t i = 0; i < NUM_LONG_PRESS_KEYS; i++) {
        if ((long_pressed_keys[i].press_time != 0) && (timer_elapsed32(long_pressed_keys[i].press_time) >= LONG_PRESS_DURATION_MS)) {
            long_pressed_keys[i].event_cb(long_pressed_keys[i].keycode);
            long_pressed_keys[i].press_time = 0;
        }
    }
}

// ===========================================
// 硬件管理函数
// ===========================================
static void bt_used_pin_init(void) {
#if defined(BT_MODE_SW_PIN) && defined(RF_MODE_SW_PIN)
    setPinInputHigh(BT_MODE_SW_PIN);
    setPinInputHigh(RF_MODE_SW_PIN);
#endif

#if defined(CABLE_PLUG_PIN) && defined(CHARGE_STATE_PIN)
    setPinInputHigh(CABLE_PLUG_PIN);
    setPinInput(CHARGE_STATE_PIN);
#endif
}

static void bt_scan_mode(void) {
#if defined(BT_MODE_SW_PIN) && defined(RF_MODE_SW_PIN)
    if (readPin(BT_MODE_SW_PIN) && !readPin(RF_MODE_SW_PIN)) {
        if (dev_info.devs != DEVS_2_4G) bt_switch_mode(dev_info.devs, DEVS_2_4G, false); // 2_4G mode
    }
    if (!readPin(BT_MODE_SW_PIN) && readPin(RF_MODE_SW_PIN)) {
        if (dev_info.last_devs != DEVS_HOST1 && dev_info.last_devs != DEVS_HOST2 && dev_info.last_devs != DEVS_HOST3) dev_info.last_devs = dev_info.after_sw_last_devs;
        if ((dev_info.devs == DEVS_USB) || (dev_info.devs == DEVS_2_4G)) bt_switch_mode(dev_info.devs, dev_info.last_devs, false); // BT mode
    }
    if (readPin(BT_MODE_SW_PIN) && readPin(RF_MODE_SW_PIN)) {
        if (dev_info.devs == DEVS_HOST1 || dev_info.devs == DEVS_HOST2 || dev_info.devs == DEVS_HOST3) {
            dev_info.after_sw_last_devs = dev_info.devs;
            eeconfig_update_user(dev_info.raw);
        }
        if (dev_info.devs != DEVS_USB) bt_switch_mode(dev_info.devs, DEVS_USB, false); // usb mode
    }
#endif
}

// ===========================================
// RGB关闭函数
// ===========================================
static void led_off_standby(void) {
    if (timer_elapsed32(key_press_time) >= LED_OFF_STANDBY_TIMEOUT_MS) {
        rgb_matrix_disable_noeeprom();
    } else {
        rgb_status_save = rgb_matrix_config.enable;
    }
}

// ===========================================
// 无操作休眠函数
// ===========================================
static void close_rgb(void) {
    if (!key_press_time) {
        key_press_time = timer_read32();
        return;
    }

    led_off_standby();

    if (sober) {
        if (kb_sleep_flag || (timer_elapsed32(key_press_time) >= ENTRY_SLEEP_TIMEOUT_MS)) {
            bak_rgb_toggle = rgb_status_save;
            sober          = false;
            close_rgb_time = timer_read32();
            rgb_matrix_disable_noeeprom();
        }
    } else {
        if (!rgb_matrix_config.enable) {
            if (timer_elapsed32(close_rgb_time) >= ENTRY_STOP_TIMEOUT) {
                if (led_inited) {
                    led_deconfig_all();
                }
#ifdef ENTRY_STOP_MODE
                lp_system_sleep();
#endif
                open_rgb();
            }
        }
    }
}

// ===========================================
// 打开RGB函数
// ===========================================
static void open_rgb(void) {
    key_press_time = timer_read32();
    if (!sober) {
        if (!led_inited) {
            led_config_all();
        }
        if (bak_rgb_toggle) {
            extern bool low_vol_offed_sleep;
            kb_sleep_flag       = false;
            low_vol_offed_sleep = false;
            rgb_matrix_enable_noeeprom();
        }
        sober = true;
    }
    if (!dev_info.sleep_mode_enabled) {
        if (kb_sleep_flag) {
            kb_sleep_flag = false;
        }
    }
}

// ===========================================
// 指示灯函数
// ===========================================
static void handle_bt_indicate_led(void) {
    uint8_t         rgb_index      = rgb_index_table[dev_info.devs];
    static uint32_t last_time      = 0;
    static uint32_t last_long_time = 0;
    static uint8_t  last_status    = 0;
    static bool     rgb_flip       = false;
    static RGB      rgb            = {0};

    if (last_status != indicator_status) {
        last_status     = indicator_status;
        last_total_time = timer_read32();
    }

    if (indicator_reset_last_time != false) {
        indicator_reset_last_time = false;
        last_time                 = 0;
    }

    switch (indicator_status) {
        case INDICATOR_PAIRING: { // 闪烁模式 5Hz 重置
            if ((last_time == 0) || (timer_elapsed32(last_time) >= WL_PAIR_INTVL_MS)) {
                last_time = timer_read32();
                rgb_flip  = !rgb_flip;
                if (rgb_flip) {
                    rgb.r = rgb_index_color_table[dev_info.devs][0];
                    rgb.g = rgb_index_color_table[dev_info.devs][1];
                    rgb.b = rgb_index_color_table[dev_info.devs][2];
                } else {
                    rgb = (RGB){.r = 0, .g = 0, .b = 0};
                }
            }

            if (bts_info.bt_info.paired) {
                last_long_time   = timer_read32();
                indicator_status = INDICATOR_CONNECTED;
                break;
            }

            if (timer_elapsed32(last_total_time) >= WL_PAIR_TIMEOUT_MS) {
                indicator_status = INDICATOR_OFF;
                kb_sleep_flag    = true;
            }
        } break;

        case INDICATOR_CONNECTING: { // 闪烁模式 2Hz 回连
            if ((last_time == 0) || (timer_elapsed32(last_time) >= WL_CONN_INTVL_MS)) {
                last_time = timer_read32();
                rgb_flip  = !rgb_flip;
                if (rgb_flip) {
                    rgb.r = rgb_index_color_table[dev_info.devs][0];
                    rgb.g = rgb_index_color_table[dev_info.devs][1];
                    rgb.b = rgb_index_color_table[dev_info.devs][2];
                } else {
                    rgb = (RGB){.r = 0, .g = 0, .b = 0};
                }
            }

            if (bts_info.bt_info.paired) {
                last_long_time   = timer_read32();
                indicator_status = INDICATOR_CONNECTED;
                break;
            }

            if (timer_elapsed32(last_total_time) >= WL_CONN_TIMEOUT_MS) {
                indicator_status = INDICATOR_OFF;
                kb_sleep_flag    = true;
            }
        } break;

        case INDICATOR_CONNECTED: { // 长亮模式
            if ((timer_elapsed32(last_long_time) < WL_CONNECTED_LAST_MS)) {
                rgb.r = rgb_index_color_table[dev_info.devs][0];
                rgb.g = rgb_index_color_table[dev_info.devs][1];
                rgb.b = rgb_index_color_table[dev_info.devs][2];
            } else {
                indicator_status = INDICATOR_OFF;
            }
        } break;

        case INDICATOR_DISABLED: { // 长灭模式
            rgb = (RGB){.r = 0, .g = 0, .b = 0};
        } break;

        default: {
            rgb_flip = false;
            if (!kb_sleep_flag) {
                if (!bts_info.bt_info.paired) {
                    if (!bts_info.bt_info.pairing) {
                        indicator_status = INDICATOR_CONNECTING;
                        break;
                    }
                    indicator_status = INDICATOR_CONNECTING;
                    if ((dev_info.devs != DEVS_USB) && (dev_info.devs != DEVS_2_4G)) {
                        bt_switch_mode(DEVS_USB, dev_info.last_devs, false);
                    }
                    if (dev_info.devs == DEVS_2_4G) {
                        bt_switch_mode(DEVS_USB, DEVS_2_4G, false);
                    }
                }
            }
        } break;
    }

    if (indicator_status) {
        rgb_matrix_set_color(rgb_index, rgb.r, rgb.g, rgb.b);
    }
}

static void handle_usb_indicate_led(void) {
    static uint32_t USB_blink_time;
    static bool     USB_blink;

    if ((USB_DRIVER.state != USB_ACTIVE)) {
        if (USB_blink_cnt <= USB_CONN_BLINK_COUNT) {
            if (timer_elapsed32(USB_blink_time) >= 500) {
                USB_blink_cnt++;
                USB_blink      = !USB_blink;
                USB_blink_time = timer_read32();
            }
            if (USB_blink) {
                rgb_matrix_set_color(rgb_index_table[DEVS_USB], 100, 100, 100);
            } else {
                rgb_matrix_set_color(rgb_index_table[DEVS_USB], 0, 0, 0);
            }
        }
        USB_switch_time = timer_read32();
    } else {
        if (timer_elapsed32(USB_switch_time) < USB_CONNECTED_LAST_MS) {
            rgb_matrix_set_color(rgb_index_table[DEVS_USB], 100, 100, 100);
        }
    }
}

// ===========================================
// RGB指示器处理函数
// ===========================================
static void handle_blink_effects(void) {
    // 全键闪烁
    if (all_blink_cnt) {
        rgb_matrix_set_color_all(0, 0, 0);
        if (timer_elapsed32(all_blink_time) > 500) {
            all_blink_time = timer_read32();
            all_blink_cnt--;
        }
        if (all_blink_cnt % 2) {
            rgb_matrix_set_color_all(all_blink_color.r, all_blink_color.g, all_blink_color.b);
        } else {
            rgb_matrix_set_color_all(0, 0, 0);
        }
    }

    // 单键闪烁
    if (single_blink_cnt) {
        if (timer_elapsed32(single_blink_time) > 500) {
            single_blink_time = timer_read32();
            single_blink_cnt--;
        }
        if (single_blink_cnt % 2) {
            rgb_matrix_set_color(single_blink_index, single_blink_color.r, single_blink_color.g, single_blink_color.b);
        } else {
            rgb_matrix_set_color(single_blink_index, 0, 0, 0);
        }
    }
}

static void handle_low_battery_warning(void) {
    static bool     Low_power_bink = false;
    static uint32_t Low_power_time = 0;

    if (bts_info.bt_info.low_vol) {
        rgb_matrix_set_color_all(0, 0, 0);
        if (timer_elapsed32(Low_power_time) >= 500) {
            Low_power_bink = !Low_power_bink;
            Low_power_time = timer_read32();
        }
        if (Low_power_bink) {
            rgb_matrix_set_color(0, 100, 0, 0);
        } else {
            rgb_matrix_set_color(0, 0, 0, 0);
        }
    } else {
        Low_power_bink = 0;
    }
}

static void handle_charging_indication(void) {
    // 充电接入检测
    static uint32_t charging_time = 0;
    static uint32_t full_time     = 0;

    bool cable_connected = !readPin(CABLE_PLUG_PIN);
    bool is_charging     = !readPin(CHARGE_STATE_PIN);

    if (cable_connected) {
        if (is_charging) {
            // 正在充电
            if (timer_elapsed32(charging_time) >= 2000) {
                rgb_matrix_set_color(0, 100, 0, 0); // 红灯
                full_time = timer_read32();
            }
        } else {
            // 充满
            if (timer_elapsed32(full_time) >= 2000) {
                rgb_matrix_set_color(0, 0, 100, 0); //
                charging_time = timer_read32();
            }
        }
    }
}

static void handle_low_battery_shutdow(void) {
    extern bool low_vol_offed_sleep;
    if (bts_info.bt_info.low_vol_offed) {
        kb_sleep_flag       = true;
        low_vol_offed_sleep = true;
    }
}

static void handle_battery_query_display(void) {
    static uint32_t query_vol_time = 0;

    // 定期查询电量
    if (!kb_sleep_flag && bts_info.bt_info.paired && timer_elapsed32(query_vol_time) >= 10000) {
        query_vol_time = timer_read32();
        bts_send_vendor(v_query_vol);
    }

    uint8_t pvol = bts_info.bt_info.pvol;
    // 计算LED数量（至少2个，最多10个）
    uint8_t led_count = (pvol < 20) ? 2 : ((pvol / 10) > 10 ? 10 : (pvol / 10));

    if (query_vol_flag) {
        query_vol_processing = true;

        // 清空显示区域
        rgb_matrix_set_color_all(0, 0, 0);

        // 根据电量确定颜色
        RGB color;
        if (pvol < 20) {
            color = (RGB){100, 0, 0}; // 红色
        } else {
            color = (RGB){0, 100, 0}; // 绿色
        }

        // 点亮LED
        for (uint8_t i = 0; i < led_count; i++) {
            rgb_matrix_set_color(query_index[i], color.r, color.g, color.b);
        }
    } else {
        if (query_vol_processing) {
            query_vol_processing = false;
        }
    }
}

// ===========================================
// 主RGB指示器函数
// ===========================================
bool bt_indicator_rgb(uint8_t led_min, uint8_t led_max) {
    // 闪烁效果处理
    handle_blink_effects();

    // 设备状态指示
    extern uint8_t factory_reset_count;
    if (dev_info.devs != DEVS_USB && !factory_reset_count) {
        handle_bt_indicate_led();
    }

    if (dev_info.devs == DEVS_USB) {
        handle_usb_indicate_led();
    }

    // 充电状态指示
    handle_charging_indication();

#if defined(CABLE_PLUG_PIN)
    bool is_charging = !readPin(CABLE_PLUG_PIN);
#endif
    if (!is_charging) {
        // 非充电状态下的其他指示
        if (dev_info.devs != DEVS_USB) {
            handle_battery_query_display();
            handle_low_battery_warning();
            handle_low_battery_shutdow();
        }
    }

    // rgb test
    if (rgb_test_en) {
        if (timer_elapsed32(rgb_test_time) >= 1000) {
            rgb_test_time = timer_read32();
            rgb_test_index++;
            if (rgb_test_index > 4) {
                rgb_test_index = 1;
            }
        }

        for (uint8_t i = 0; i < 84; i++) {
            rgb_matrix_set_color(i, rgb_test_color_table[rgb_test_index - 1][0], rgb_test_color_table[rgb_test_index - 1][1], rgb_test_color_table[rgb_test_index - 1][2]);
        }
    }

    return true;
}

// ===========================================
// Helper Functions
// ===========================================
static bool is_bt_device(uint8_t device) {
    return (device >= DEVS_HOST1 && device <= DEVS_HOST3);
}

static bool validate_device_type(uint8_t device) {
    return (device >= DEVS_USB && device <= DEVS_2_4G);
}

static void reset_bt_connection_state(void) {
    bts_info.bt_info.pairing        = false;
    bts_info.bt_info.paired         = false;
    bts_info.bt_info.come_back      = false;
    bts_info.bt_info.come_back_err  = false;
    bts_info.bt_info.mode_switched  = false;
    bts_info.bt_info.indictor_rgb_s = 0;
}

static void send_device_vendor_command(uint8_t device) {
    static const uint8_t vendor_cmds[] = {v_host1, v_host2, v_host3, v_2_4g};

    if (!validate_device_type(device)) {
        return;
    }

    switch (device) {
        case DEVS_HOST1:
        case DEVS_HOST2:
        case DEVS_HOST3:
        case DEVS_2_4G:
            bts_send_vendor(vendor_cmds[device - 1]);
            break;
        case DEVS_USB:
            bts_send_vendor(v_usb);
            break;
        default:
            break;
    }
}

static uint8_t keycode_to_device_type(uint16_t keycode) {
    switch (keycode) {
        case BT_HOST1:
            return DEVS_HOST1;
        case BT_HOST2:
            return DEVS_HOST2;
        case BT_HOST3:
            return DEVS_HOST3;
        case BT_2_4G:
            return DEVS_2_4G;
        case BT_USB:
            return DEVS_USB;
        default:
            return DEVS_USB;
    }
}
