/**
 * @file bt_task.c
 * @brief
 * @author JoyLee
 * @version 2.0.0
 * @date 2023-04-06
 *
 * @copyright Copyright (c) 2023 Westberry Technology Corp., Ltd
 */

#include QMK_KEYBOARD_H

#include "bts_lib.h"
#include "bt_task.h"
#include "quantum.h"
#include "uart.h"
#include "usb_main.h"

#ifdef BT_DEBUG_MODE
#    define BT_DEBUG_INFO(fmt, ...) dprintf(fmt, ##__VA_ARGS__)
#else
#    define BT_DEBUG_INFO(fmt, ...)
#endif

// ===========================================
// 函数声明
// ===========================================
static void bt_long_pressed_keys_hook(void);
static void bt_long_pressed_keys_cb(uint16_t keycode);
static bool bt_process_record_other(uint16_t keycode, keyrecord_t *record);
static void bt_scan_mode(void);
static void bt_blink_effects(void);
static void bt_charging_indication(void);
static void bt_bat_low_level_warning(void);
static void bt_bat_low_level_shutdown(void);
static void bt_bat_query_period(void);
static void bt_bat_level_display(void);
static void show_device_state(void);
static void bt_indicate_led(void);
static void factory_reset_indicate_led(void);
static void usb_indicate_led(void);
#ifdef RGB_MATRIX_ENABLE
static void led_off_standby(void);
static void open_rgb(void);
static void close_rgb(void);
static void led_config_all(void);
static void led_deconfig_all(void);
#endif

// ===========================================
// 常量定义
// ===========================================
/* Wireless connection timing constants */
#define WL_CONN_TIMEOUT_MS (10 * 1000)   // 10 seconds
#define WL_PAIR_TIMEOUT_MS (30 * 1000)   // 30 seconds
#define WL_PAIR_INTVL_MS (200)           // 5Hz blink for pairing
#define WL_CONN_INTVL_MS (500)           // 2Hz blink for connecting
#define WL_CONNECTED_LAST_MS (3 * 1000)  // Show wl connected status for 3s
#define USB_CONN_INTVL_MS (500)          // Show USB connected status for 3s
#define USB_CONNECTED_LAST_MS (3 * 1000) // Show USB connected status for 3s
#define USB_CONN_BLINK_COUNT 20          // USB blink count when connected

/* Sleep and standby timeouts */
#define LED_OFF_STANDBY_TIMEOUT_MS (5 * 60 * 1000) // 5 minutes
#define ENTRY_SLEEP_TIMEOUT_MS (30 * 60 * 1000)    // 30 minutes

/* Array size calculations */
#define NUM_LONG_PRESS_KEYS (sizeof(long_pressed_keys) / sizeof(long_pressed_keys_t))
#define LONG_PRESS_DURATION_MS (3 * 1000)

/* Hardware validation helpers */
#define IS_BT_DEVICE(dev) ((dev) >= DEVS_HOST1 && (dev) <= DEVS_HOST3)
#define IS_VALID_DEVICE(dev) ((dev) >= DEVS_USB && (dev) <= DEVS_2_4G)

#define TASK_UPDATE_INTERVAL_MS 1
#define BT_INIT_WAIT_MS 2000

// ===========================================
// 结构体定义
// ===========================================
typedef struct {
    uint32_t press_time;
    uint16_t keycode;
    void (*event_cb)(uint16_t);
} long_pressed_keys_t;

typedef enum {
    BATTERY_STATE_UNPLUGGED = 0, // No cable connected
    BATTERY_STATE_CHARGING,      // Cable connected, charging
    BATTERY_STATE_CHARGED_FULL   // Cable connected, fully charged
} battery_charge_state_t;
static battery_charge_state_t get_battery_charge_state(void);

// 指示器状态
typedef enum {
    INDICATOR_OFF        = 0,
    INDICATOR_PAIRING    = 1,
    INDICATOR_CONNECTING = 2,
    INDICATOR_CONNECTED  = 3,
    INDICATOR_DISABLED   = 4,
} indicator_state_t;
static indicator_state_t indicator_status = INDICATOR_CONNECTING;

// ===========================================
// 全局变量
// ===========================================

extern keymap_config_t keymap_config;

extern bool low_vol_offed_sleep;

uint32_t   bt_init_time = 0;
dev_info_t dev_info     = {0};
bts_info_t bts_info     = {
        .bt_name        = {MM_BT_HOST1_NAME, MM_BT_HOST2_NAME, MM_BT_HOST3_NAME},
        .uart_init      = uart_init,
        .uart_read      = uart_read,
        .uart_transmit  = uart_transmit,
        .uart_receive   = uart_receive,
        .uart_available = uart_available,
        .timer_read32   = timer_read32,
};

// 长按键配置
// clang-format off
static long_pressed_keys_t long_pressed_keys[] = {
    {.keycode = BT_HOST1, .press_time = 0, .event_cb = bt_long_pressed_keys_cb},
    {.keycode = BT_HOST2, .press_time = 0, .event_cb = bt_long_pressed_keys_cb},
    {.keycode = BT_HOST3, .press_time = 0, .event_cb = bt_long_pressed_keys_cb},
    {.keycode = BT_2_4G, .press_time = 0, .event_cb = bt_long_pressed_keys_cb},
    {.keycode = EE_CLR, .press_time = 0, .event_cb = bt_long_pressed_keys_cb},
    {.keycode = RGB_TEST, .press_time = 0, .event_cb = bt_long_pressed_keys_cb},
};
// clang-format on
static bool indicator_reset_last_time = false;

// RGB控制
static uint32_t key_press_time = 0;
static uint32_t close_rgb_time = 0;

static bool led_inited      = false;
static bool rgb_status_save = false;
static bool bak_rgb_toggle  = false;
static bool sober           = true;
static bool kb_sleep_flag   = false;

// 设备指示配置
static const uint8_t rgb_index_table[] = {MM_BLINK_USB_INDEX, MM_BLINK_HOST1_INDEX, MM_BLINK_HOST2_INDEX, MM_BLINK_HOST3_INDEX, MM_BLINK_2G4_INDEX};
// clang-format off
static const uint8_t rgb_index_color_table[][3] = {
    {MM_BLINK_USB_COLOR},
    {MM_BLINK_HOST1_COLOR},
    {MM_BLINK_HOST2_COLOR},
    {MM_BLINK_HOST3_COLOR},
    {MM_BLINK_2G4_COLOR},
};
// clang-format on

// RGB测试相关
static const uint8_t rgb_test_color_table[][3] = {
    {100, 0, 0},
    {0, 100, 0},
    {0, 0, 100},
    {100, 100, 100},
};
static uint8_t  rgb_test_index  = 0;
static bool     rgb_test_en     = false;
static uint32_t rgb_test_time   = 0;
static bool     query_vol_flag  = false;
static uint32_t last_total_time = 0;

// 闪烁效果相关
static uint8_t  all_blink_cnt      = 0;
static uint32_t all_blink_time     = 0;
static RGB      all_blink_color    = {0};
static uint8_t  single_blink_cnt   = 0;
static uint8_t  single_blink_index = 0;
static RGB      single_blink_color = {0};
static uint32_t single_blink_time  = 0;

static uint32_t EE_CLR_press_cnt  = 0;
static uint32_t EE_CLR_press_time = 0;
static bool     EE_CLR_flag       = false;

// 电量查询
/* 电量显示LED */
uint8_t query_index[10] = BAT_LEVEL_DISPLAY_INDEX;

// USB相关
static uint32_t USB_switch_time = 0;
static uint8_t  USB_blink_cnt   = 0;

#include "command.h"
#include "action.h"

void register_mouse(uint8_t mouse_keycode, bool pressed);
/** \brief Utilities for actions. (FIXME: Needs better description)
 *
 * FIXME: Needs documentation.
 */
__attribute__((weak)) void register_code(uint8_t code) {
    if (dev_info.devs) {
        bts_process_keys(code, 1, dev_info.devs, keymap_config.no_gui);
        bts_task(dev_info.devs);
        while (bts_is_busy()) {
        }
    } else {
        if (code == KC_NO) {
            return;

#ifdef LOCKING_SUPPORT_ENABLE
        } else if (KC_LOCKING_CAPS_LOCK == code) {
#    ifdef LOCKING_RESYNC_ENABLE
            // Resync: ignore if caps lock already is on
            if (host_keyboard_leds() & (1 << USB_LED_CAPS_LOCK)) return;
#    endif
            add_key(KC_CAPS_LOCK);
            send_keyboard_report();
            wait_ms(TAP_HOLD_CAPS_DELAY);
            del_key(KC_CAPS_LOCK);
            send_keyboard_report();

        } else if (KC_LOCKING_NUM_LOCK == code) {
#    ifdef LOCKING_RESYNC_ENABLE
            if (host_keyboard_leds() & (1 << USB_LED_NUM_LOCK)) return;
#    endif
            add_key(KC_NUM_LOCK);
            send_keyboard_report();
            wait_ms(100);
            del_key(KC_NUM_LOCK);
            send_keyboard_report();

        } else if (KC_LOCKING_SCROLL_LOCK == code) {
#    ifdef LOCKING_RESYNC_ENABLE
            if (host_keyboard_leds() & (1 << USB_LED_SCROLL_LOCK)) return;
#    endif
            add_key(KC_SCROLL_LOCK);
            send_keyboard_report();
            wait_ms(100);
            del_key(KC_SCROLL_LOCK);
            send_keyboard_report();
#endif

        } else if (IS_BASIC_KEYCODE(code)) {
            // TODO: should push command_proc out of this block?
            if (command_proc(code)) return;

            // Force a new key press if the key is already pressed
            // without this, keys with the same keycode, but different
            // modifiers will be reported incorrectly, see issue #1708
            if (is_key_pressed(code)) {
                del_key(code);
                send_keyboard_report();
            }
            add_key(code);
            send_keyboard_report();
        } else if (IS_MODIFIER_KEYCODE(code)) {
            add_mods(MOD_BIT(code));
            send_keyboard_report();

#ifdef EXTRAKEY_ENABLE
        } else if (IS_SYSTEM_KEYCODE(code)) {
            host_system_send(KEYCODE2SYSTEM(code));
        } else if (IS_CONSUMER_KEYCODE(code)) {
            host_consumer_send(KEYCODE2CONSUMER(code));
#endif

        } else if (IS_MOUSE_KEYCODE(code)) {
            register_mouse(code, true);
        }
    }
}

/** \brief Utilities for actions. (FIXME: Needs better description)
 *
 * FIXME: Needs documentation.
 */
__attribute__((weak)) void unregister_code(uint8_t code) {
    if (dev_info.devs) {
        bts_process_keys(code, 0, dev_info.devs, keymap_config.no_gui);
        bts_task(dev_info.devs);
        while (bts_is_busy()) {
        }
    } else {
        if (code == KC_NO) {
            return;

#ifdef LOCKING_SUPPORT_ENABLE
        } else if (KC_LOCKING_CAPS_LOCK == code) {
#    ifdef LOCKING_RESYNC_ENABLE
            // Resync: ignore if caps lock already is off
            if (!(host_keyboard_leds() & (1 << USB_LED_CAPS_LOCK))) return;
#    endif
            add_key(KC_CAPS_LOCK);
            send_keyboard_report();
            del_key(KC_CAPS_LOCK);
            send_keyboard_report();

        } else if (KC_LOCKING_NUM_LOCK == code) {
#    ifdef LOCKING_RESYNC_ENABLE
            if (!(host_keyboard_leds() & (1 << USB_LED_NUM_LOCK))) return;
#    endif
            add_key(KC_NUM_LOCK);
            send_keyboard_report();
            del_key(KC_NUM_LOCK);
            send_keyboard_report();

        } else if (KC_LOCKING_SCROLL_LOCK == code) {
#    ifdef LOCKING_RESYNC_ENABLE
            if (!(host_keyboard_leds() & (1 << USB_LED_SCROLL_LOCK))) return;
#    endif
            add_key(KC_SCROLL_LOCK);
            send_keyboard_report();
            del_key(KC_SCROLL_LOCK);
            send_keyboard_report();
#endif

        } else if (IS_BASIC_KEYCODE(code)) {
            del_key(code);
            send_keyboard_report();
        } else if (IS_MODIFIER_KEYCODE(code)) {
            del_mods(MOD_BIT(code));
            send_keyboard_report();

#ifdef EXTRAKEY_ENABLE
        } else if (IS_SYSTEM_KEYCODE(code)) {
            host_system_send(0);
        } else if (IS_CONSUMER_KEYCODE(code)) {
            host_consumer_send(0);
#endif

        } else if (IS_MOUSE_KEYCODE(code)) {
            register_mouse(code, false);
        }
    }
}

void do_code16(uint16_t code, void (*f)(uint8_t));

__attribute__((weak)) void register_code16(uint16_t code) {
    if (dev_info.devs) {
        if (QK_MODS_GET_MODS(code) & 0x1) {
            if (QK_MODS_GET_MODS(code) & 0x10)
                bts_process_keys(KC_RCTL, 1, dev_info.devs, keymap_config.no_gui);
            else
                bts_process_keys(KC_LCTL, 1, dev_info.devs, keymap_config.no_gui);
        }
        if (QK_MODS_GET_MODS(code) & 0x2) {
            if (QK_MODS_GET_MODS(code) & 0x10)
                bts_process_keys(KC_RSFT, 1, dev_info.devs, keymap_config.no_gui);
            else
                bts_process_keys(KC_LSFT, 1, dev_info.devs, keymap_config.no_gui);
        }
        if (QK_MODS_GET_MODS(code) & 0x4) {
            if (QK_MODS_GET_MODS(code) & 0x10)
                bts_process_keys(KC_RALT, 1, dev_info.devs, keymap_config.no_gui);
            else
                bts_process_keys(KC_LALT, 1, dev_info.devs, keymap_config.no_gui);
        }
        if (QK_MODS_GET_MODS(code) & 0x8) {
            if (QK_MODS_GET_MODS(code) & 0x10)
                bts_process_keys(KC_RGUI, 1, dev_info.devs, keymap_config.no_gui);
            else
                bts_process_keys(KC_LGUI, 1, dev_info.devs, keymap_config.no_gui);
        }
        bts_process_keys(QK_MODS_GET_BASIC_KEYCODE(code), 1, dev_info.devs, keymap_config.no_gui);
    } else {
        if (IS_MODIFIER_KEYCODE(code) || code == KC_NO) {
            do_code16(code, register_mods);
        } else {
            do_code16(code, register_weak_mods);
        }
        register_code(code);
    }
}

__attribute__((weak)) void unregister_code16(uint16_t code) {
    if (dev_info.devs) {
        if (QK_MODS_GET_MODS(code) & 0x1) {
            if (QK_MODS_GET_MODS(code) & 0x10)
                bts_process_keys(KC_RCTL, 0, dev_info.devs, keymap_config.no_gui);
            else
                bts_process_keys(KC_LCTL, 0, dev_info.devs, keymap_config.no_gui);
        }
        if (QK_MODS_GET_MODS(code) & 0x2) {
            if (QK_MODS_GET_MODS(code) & 0x10)
                bts_process_keys(KC_RSFT, 0, dev_info.devs, keymap_config.no_gui);
            else
                bts_process_keys(KC_LSFT, 0, dev_info.devs, keymap_config.no_gui);
        }
        if (QK_MODS_GET_MODS(code) & 0x4) {
            if (QK_MODS_GET_MODS(code) & 0x10)
                bts_process_keys(KC_RALT, 0, dev_info.devs, keymap_config.no_gui);
            else
                bts_process_keys(KC_LALT, 0, dev_info.devs, keymap_config.no_gui);
        }
        if (QK_MODS_GET_MODS(code) & 0x8) {
            if (QK_MODS_GET_MODS(code) & 0x10)
                bts_process_keys(KC_RGUI, 0, dev_info.devs, keymap_config.no_gui);
            else
                bts_process_keys(KC_LGUI, 0, dev_info.devs, keymap_config.no_gui);
        }
        bts_process_keys(QK_MODS_GET_BASIC_KEYCODE(code), 0, dev_info.devs, keymap_config.no_gui);
    } else {
        unregister_code(code);
        if (IS_MODIFIER_KEYCODE(code) || code == KC_NO) {
            do_code16(code, unregister_mods);
        } else {
            do_code16(code, unregister_weak_mods);
        }
    }
}

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
    // bt_used_pin_init();

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
    if ((bt_init_time != 0) && (timer_elapsed32(bt_init_time) >= BT_INIT_WAIT_MS)) {
        bt_init_time = 0;

        // bts_send_vendor(v_en_sleep_bt);
        // bts_send_vendor(v_en_sleep_wl);

        bts_send_name(DEVS_HOST1);
        switch (dev_info.devs) {
            case DEVS_HOST1: {
                bts_send_vendor(v_host1);
            } break;
            case DEVS_HOST2: {
                bts_send_vendor(v_host2);
            } break;
            case DEVS_HOST3: {
                bts_send_vendor(v_host3);
            } break;
            case DEVS_2_4G: {
                bts_send_vendor(v_2_4g);
            } break;
            default: {
                bts_send_vendor(v_usb);
                dev_info.devs = DEVS_USB;
                eeconfig_update_user(dev_info.raw);
            } break;
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
            close_rgb();
#endif
        }
    }

    bt_long_pressed_keys_hook();
    if (!bt_init_time) bt_scan_mode();
}

// ===========================================
// 按键处理函数
// ===========================================
bool bt_process_record(uint16_t keycode, keyrecord_t *record) {
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

    retval = bt_process_record_other(keycode, record);

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
    if ((dev_info.devs != DEVS_USB) && (dev_info.devs != DEVS_2_4G)) {
        dev_info.last_devs = dev_info.devs;
    } else if (dev_info.devs == DEVS_USB) {
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
    bts_info.bt_info.pairing        = false;
    bts_info.bt_info.paired         = false;
    bts_info.bt_info.come_back      = false;
    bts_info.bt_info.come_back_err  = false;
    bts_info.bt_info.mode_switched  = false;
    bts_info.bt_info.indictor_rgb_s = 0;
    eeconfig_update_user(dev_info.raw);

    // Handle indicator status and send vendor commands
    switch (dev_info.devs) {
        case DEVS_HOST1: {
            if (reset != false) {
                indicator_status          = 1;
                indicator_reset_last_time = true;
                bts_send_name(DEVS_HOST1);
                bts_send_vendor(v_host1);
                bts_send_vendor(v_pair);
            } else {
                indicator_status          = 2;
                indicator_reset_last_time = true;
                bts_send_vendor(v_host1);
            }
        } break;
        case DEVS_HOST2: {
            if (reset != false) {
                indicator_status          = 1;
                indicator_reset_last_time = true;
                bts_send_name(DEVS_HOST2);
                bts_send_vendor(v_host2);
                bts_send_vendor(v_pair);
            } else {
                indicator_status          = 2;
                indicator_reset_last_time = true;
                bts_send_vendor(v_host2);
            }
        } break;
        case DEVS_HOST3: {
            if (reset != false) {
                indicator_status          = 1;
                indicator_reset_last_time = true;
                bts_send_name(DEVS_HOST3);
                bts_send_vendor(v_host3);
                bts_send_vendor(v_pair);
            } else {
                indicator_status          = 2;
                indicator_reset_last_time = true;
                bts_send_vendor(v_host3);
            }
        } break;
        case DEVS_2_4G: {
            if (reset != false) {
                indicator_status          = 1;
                indicator_reset_last_time = true;
                // bts_send_vendor(v_2_4g);
                bts_send_vendor(v_pair);
            } else {
                indicator_status          = 2;
                indicator_reset_last_time = true;
                bts_send_vendor(v_2_4g);
            }
        } break;
        case DEVS_USB: {
            indicator_status          = 2;
            indicator_reset_last_time = true;
            bts_send_vendor(v_usb);
        } break;
        default:
            break;
    }
}

// ===========================================
// 其他按键处理
// ===========================================
static bool bt_process_record_other(uint16_t keycode, keyrecord_t *record) {
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

    switch (keycode) {
        case BT_HOST1: {
            if (record->event.pressed) {
                if (dev_info.devs != DEVS_HOST1) {
                    bt_switch_mode(dev_info.devs, DEVS_HOST1, false);
                }
            }
        } break;
        case BT_HOST2: {
            if (record->event.pressed) {
                if (dev_info.devs != DEVS_HOST2) {
                    bt_switch_mode(dev_info.devs, DEVS_HOST2, false);
                }
            }
        } break;
        case BT_HOST3: {
            if (record->event.pressed) {
                if (dev_info.devs != DEVS_HOST3) {
                    bt_switch_mode(dev_info.devs, DEVS_HOST3, false);
                }
            }
        } break;
        case BT_2_4G: {
            if (record->event.pressed) {
                if (dev_info.devs != DEVS_2_4G) {
                    bt_switch_mode(dev_info.devs, DEVS_2_4G, false);
                }
            }
        } break;
        case BT_USB: {
            if (record->event.pressed) {
                bt_switch_mode(dev_info.devs, DEVS_USB, false);
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

        case EE_CLR: {
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
static void bt_long_pressed_keys_cb(uint16_t keycode) {
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

        case EE_CLR: {
            if (!EE_CLR_flag) {
                EE_CLR_flag       = true;
                EE_CLR_press_time = timer_read32();
                EE_CLR_press_cnt  = 1;
                rgb_matrix_enable_noeeprom();
            }
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

static void bt_long_pressed_keys_hook(void) {
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
void bt_used_pin_init(void) {
#if defined(MM_BT_MODE_PIN) && defined(MM_2G4_MODE_PIN)
    setPinInputHigh(MM_BT_MODE_PIN);
    setPinInputHigh(MM_2G4_MODE_PIN);
#endif
#if defined(MM_CABLE_PIN) && defined(MM_CHARGE_PIN)
    setPinInputHigh(MM_CABLE_PIN);
    setPinInput(MM_CHARGE_PIN);
#endif
}

static void bt_scan_mode(void) {
#if defined(MM_BT_MODE_PIN) && defined(MM_2G4_MODE_PIN)
    if (readPin(MM_2G4_MODE_PIN) && !readPin(MM_BT_MODE_PIN)) {
        if ((dev_info.devs == DEVS_USB) || (dev_info.devs == DEVS_2_4G)) bt_switch_mode(dev_info.devs, dev_info.last_devs, false); // BT mode
    }
    if (readPin(MM_BT_MODE_PIN) && !readPin(MM_2G4_MODE_PIN)) {
        if (dev_info.devs != DEVS_2_4G) bt_switch_mode(dev_info.devs, DEVS_2_4G, false); // 2_4G mode
    }
    if (readPin(MM_BT_MODE_PIN) && readPin(MM_2G4_MODE_PIN)) {
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
void led_config_all(void) {
    if (!led_inited) {
#ifdef RGB_MATRIX_SHUTDOWN_PIN
        setPinOutputPushPull(RGB_MATRIX_SHUTDOWN_PIN);
        writePinHigh(RGB_MATRIX_SHUTDOWN_PIN);
#endif
        led_inited = true;
    }
}

void led_deconfig_all(void) {
    if (led_inited) {
#ifdef RGB_MATRIX_SHUTDOWN_PIN
        setPinOutputOpenDrain(RGB_MATRIX_SHUTDOWN_PIN);
#endif
        led_inited = false;
    }
}

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

#ifdef RGB_MATRIX_SHUTDOWN_PIN
            setPinOutputOpenDrain(RGB_MATRIX_SHUTDOWN_PIN);
#endif
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
        if (bak_rgb_toggle) {
            kb_sleep_flag       = false;
            low_vol_offed_sleep = false;
            rgb_matrix_enable_noeeprom();

#ifdef RGB_MATRIX_SHUTDOWN_PIN
            setPinOutputPushPull(RGB_MATRIX_SHUTDOWN_PIN);
            writePinHigh(RGB_MATRIX_SHUTDOWN_PIN);
#endif
        }
        if (!led_inited) {
            led_config_all();
        }
        sober = true;
    }
}

// ===========================================
// 指示灯函数
// ===========================================
static void bt_indicate_led(void) {
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

static void usb_indicate_led(void) {
    static uint32_t USB_blink_time;
    static bool     USB_blink;

    if ((USB_DRIVER.state != USB_ACTIVE)) {
        if (USB_blink_cnt <= USB_CONN_BLINK_COUNT) {
            if (timer_elapsed32(USB_blink_time) >= USB_CONN_INTVL_MS) {
                USB_blink_cnt++;
                USB_blink      = !USB_blink;
                USB_blink_time = timer_read32();
            }
            if (USB_blink) {
                rgb_matrix_set_color(rgb_index_table[DEVS_USB], rgb_index_color_table[DEVS_USB][0], rgb_index_color_table[DEVS_USB][1], rgb_index_color_table[DEVS_USB][2]);
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
static void bt_blink_effects(void) {
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

static void bt_bat_low_level_warning(void) {
    static bool     Low_power_bink = false;
    static uint32_t Low_power_time = 0;

    if (bts_info.bt_info.low_vol) {
        rgb_matrix_set_color_all(RGB_OFF);
        if (timer_elapsed32(Low_power_time) >= 500) {
            Low_power_bink = !Low_power_bink;
            Low_power_time = timer_read32();
        }
        if (Low_power_bink) {
            rgb_matrix_set_color(CHRGE_LOW_LEVEL_INDICATOR_INDEX, CHRGE_LOW_LEVEL_INDICATOR_COLOR); // 红色
        } else {
            rgb_matrix_set_color(CHRGE_LOW_LEVEL_INDICATOR_INDEX, RGB_OFF);
        }
    } else {
        Low_power_bink = 0;
    }
}

static void bt_charging_indication(void) {
    static uint32_t charging_time = 0;

    if (get_battery_charge_state() == BATTERY_STATE_CHARGING) {
        // 正在充电
        if (timer_elapsed32(charging_time) >= 2000) {
            rgb_matrix_set_color(CHRGE_LOW_LEVEL_INDICATOR_INDEX, CHRGE_LOW_LEVEL_INDICATOR_COLOR); // 红灯
        }
    } else {
        charging_time = timer_read32();
    }
}

static void bt_bat_low_level_shutdown(void) {
    if (bts_info.bt_info.low_vol_offed) {
        kb_sleep_flag       = true;
        low_vol_offed_sleep = true;
    }
}

// ===========================================
// 电量查询显示函数
// ===========================================
static battery_charge_state_t get_battery_charge_state(void) {
#if defined(MM_CABLE_PIN) && defined(MM_CHARGE_PIN)
    static battery_charge_state_t stable_state = BATTERY_STATE_UNPLUGGED;

    if (readPin(MM_CABLE_PIN)) {
        stable_state = BATTERY_STATE_UNPLUGGED;
    } else {
        if (!readPin(MM_CHARGE_PIN)) {
            stable_state = BATTERY_STATE_CHARGING;
        } else {
            stable_state = BATTERY_STATE_CHARGED_FULL;
        }
    }

    return stable_state;
#else
    return BATTERY_STATE_UNPLUGGED;
#endif
}

static void bt_bat_query_period(void) {
    static uint32_t query_vol_time = 0;
    if (!bt_init_time && !kb_sleep_flag && (bts_info.bt_info.paired) && timer_elapsed32(query_vol_time) >= 4000) {
        query_vol_time = timer_read32();
        bts_send_vendor(v_query_vol);
    }
}

static void bt_bat_level_display(void) {
    if (query_vol_flag) {
        for (uint8_t i = 0; i < 10; i++) {
            uint8_t pvol      = bts_info.bt_info.pvol;
            uint8_t led_count = (pvol < 20) ? 2 : ((pvol / 10) > 10 ? 10 : (pvol / 10));

            RGB color;
            if (pvol < 30) {
                color = (RGB){RGB_RED}; // 红色
            } else if (pvol < 60) {
                color = (RGB){RGB_YELLOW}; // 黄色
            } else {
                color = (RGB){RGB_GREEN}; // 绿色
            }

            for (uint8_t i = 0; i < led_count; i++) {
                rgb_matrix_set_color(query_index[i], color.r, color.g, color.b);
            }
        }
    }
}

static void show_device_state(void) {
    // FN 按下时显示当前无线设备状态
    if ((get_highest_layer(default_layer_state | layer_state) == 1) || (get_highest_layer(default_layer_state | layer_state) == 3)) {
        switch (dev_info.devs) {
            case DEVS_HOST1: {
                rgb_matrix_set_color(MM_BLINK_HOST1_INDEX, MM_BLINK_HOST1_COLOR);
            } break;
            case DEVS_HOST2: {
                rgb_matrix_set_color(MM_BLINK_HOST2_INDEX, MM_BLINK_HOST2_COLOR);
            } break;
            case DEVS_HOST3: {
                rgb_matrix_set_color(MM_BLINK_HOST3_INDEX, MM_BLINK_HOST3_COLOR);
            } break;
            case DEVS_2_4G: {
                rgb_matrix_set_color(MM_BLINK_2G4_INDEX, MM_BLINK_2G4_COLOR);
            } break;
            case DEVS_USB: {
                rgb_matrix_set_color(MM_BLINK_USB_INDEX, MM_BLINK_USB_COLOR);
            } break;
            default:
                break;
        }
    }
}

static void factory_reset_indicate_led(void) {
    if (EE_CLR_flag) {
        if (timer_elapsed32(EE_CLR_press_time) >= EE_CLR_press_cnt * 300) {
            EE_CLR_press_cnt++;
        }
        if (EE_CLR_press_cnt >= 7) {
            EE_CLR_press_time = 0;
            EE_CLR_press_cnt  = 0;
            EE_CLR_flag       = false;
            eeconfig_init();
            eeconfig_update_rgb_matrix_default();
            if (get_highest_layer(default_layer_state | layer_state) != 0) {
                set_single_persistent_default_layer(0);
            }
            if (dev_info.devs != DEVS_USB && dev_info.devs != DEVS_2_4G) {
                bts_send_vendor(v_clear);
                wait_ms(1000);
            }
            keymap_config.no_gui = false;
        }
        if (EE_CLR_press_cnt & 0x1) {
            rgb_matrix_set_color_all(120, 120, 120);
        } else {
            rgb_matrix_set_color_all(0, 0, 0);
        }
    }
}

// ===========================================
// 主RGB指示器函数
// ===========================================
bool bt_indicator_rgb(void) {
    // Low battery warning
    if ((dev_info.devs != DEVS_USB) && (get_battery_charge_state() == BATTERY_STATE_UNPLUGGED)) {
        bt_bat_low_level_warning();
    }

    // Show the current device state
    show_device_state();

    if (dev_info.devs == DEVS_USB) {
        usb_indicate_led();
    } else {
        bt_indicate_led();
        bt_bat_level_display();
    }

#if defined(MM_CABLE_PIN) && defined(MM_CHARGE_PIN)
    static bool is_cable_plugin = false;
    // 充电状态指示
    bt_charging_indication();
    is_cable_plugin = (get_battery_charge_state() == BATTERY_STATE_UNPLUGGED) ? false : true;
#endif
    if (!is_cable_plugin) {
        // 非充电状态下的其他指示
        if (dev_info.devs != DEVS_USB) {
            bt_bat_query_period();
            bt_bat_low_level_warning();
            bt_bat_low_level_shutdown();
        }
    }

    // 闪烁效果处理
    bt_blink_effects();

    // Factory reset
    factory_reset_indicate_led();

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

void bt_housekeeping_task_user(void) {
    bt_task();
#ifdef USB_SUSPEND_CHECK_ENABLE
    static uint32_t usb_suspend_timer = 0;
    static uint32_t usb_suspend       = false;

    if (dev_info.devs == DEVS_USB) {
        if (USB_DRIVER.state != USB_ACTIVE) {
            // USB挂起状态
            if (!usb_suspend_timer) {
                // 开始计时
                usb_suspend_timer = timer_read32();
            } else if (timer_elapsed32(usb_suspend_timer) > 10000) {
                // 挂起超过10秒，关闭背光
                if (!usb_suspend) {
                    // 如果之前没有进入挂起状态，执行挂起操作
                    usb_suspend = true;
                    led_deconfig_all();
                }
                usb_suspend_timer = 0;
            }
        } else {
            // USB活跃状态，重置计时器
            if (usb_suspend_timer) {
                usb_suspend_timer = 0;
                if (usb_suspend) {
                    // 如果之前处于挂起状态，恢复背光
                    usb_suspend = false;
                    led_config_all();
                }
            }
        }
    } else {
        if (usb_suspend) {
            usb_suspend_timer = 0;
            usb_suspend       = false;
            led_config_all();
        }
    }
#endif
}

void bt_keyboard_post_init_user(void) {
    // 初始化蓝牙
    bt_init();
}
