/**
 * @file bt_task.c
 * @brief
 * @author JoyLee
 * @version 2.0.0
 * @date 2023-04-06
 *
 * @copyright Copyright (c) 2023 Westberry Technology Corp., Ltd
 */

#include "bt_task.h"
#include "multimode.h"

#ifdef BT_DEBUG_ENABLE
#    define BT_DEBUG_INFO(fmt, ...) dprintf(fmt, ##__VA_ARGS__)
#else
#    define BT_DEBUG_INFO(fmt, ...)
#endif

#ifdef MULTIMODE_ENABLE
static bool    query_vol_flag = false;
static uint8_t query_index[]  = {BATTERY_VOL_DISPLAY_INDEX};
#endif

#ifdef CLOSE_RGB_ENABLE
static uint32_t key_press_time;
static uint32_t close_rgb_time;
static bool     bak_rgb_toggle;
static bool     sober         = true;
static bool     kb_sleep_flag = false;

extern void led_config_all(void);
extern void led_deconfig_all(void);
extern bool led_inited;

uint8_t device_table[] = {
#    ifdef RGB_MATRIX_BLINK_INDEX_USB
    RGB_MATRIX_BLINK_INDEX_USB,
#    endif
    RGB_MATRIX_BLINK_INDEX_HOST1, RGB_MATRIX_BLINK_INDEX_HOST2, RGB_MATRIX_BLINK_INDEX_HOST3, RGB_MATRIX_BLINK_INDEX_2G4,
};

void open_rgb(void) {
    key_press_time = timer_read32();
    if (!sober) {
        if (bak_rgb_toggle) {
            kb_sleep_flag = false;
            rgb_matrix_enable_noeeprom();
        }
        if (!led_inited) {
            led_config_all();
        }
        sober = true;
    }
}

void close_rgb(void) {
    if (!key_press_time) {
        key_press_time = timer_read32();
        return;
    }

    if (sober) {
        if (kb_sleep_flag || (timer_elapsed32(key_press_time) >= (WL_SLEEP_TIMEOUT))) { // WL_SLEEP_TIMEOUT minutes
            bak_rgb_toggle = rgb_matrix_config.enable;
            sober          = false;
            close_rgb_time = timer_read32();
            rgb_matrix_disable_noeeprom();
        }
    } else {
        if (!rgb_matrix_config.enable) {
            if (timer_elapsed32(close_rgb_time) >= 100) {
                /* Turn off all indicators led */
                if (led_inited) {
                    led_deconfig_all();
                }

                lp_system_sleep();
                // mm_switch_mode(!mm_eeconfig.devs, mm_eeconfig.devs, false);
                if (mm_eeconfig.devs != DEVS_USB && mm_eeconfig.devs != DEVS_2G4) {
                    mm_switch_mode(DEVS_USB, mm_eeconfig.last_devs, false);
                } else if (mm_eeconfig.devs == DEVS_2G4) {
                    mm_switch_mode(DEVS_USB, DEVS_2G4, false);
                }
                open_rgb();
            }
        }
    }
}
#endif

#ifdef RGB_MATRIX_BLINK_ENABLE
#    include "rgb_matrix_blink.h"
#    ifdef RGB_MATRIX_BLINK_INDEX_USB
#        include "usb_main.h"
#    endif

uint8_t  wl_rgb_indicator_s = 0;
uint32_t wl_rgb_timer       = 0;

static void rgb_blink_cb(uint8_t index);
static void wl_indicators_hook(uint8_t index);

static void wl_indicators_hook(uint8_t index) {
    extern void rgb_blink_cb(uint8_t index);

    switch (wl_rgb_indicator_s) {
        case wls_none: {
            wl_rgb_timer = 0;
        } break;
        case wls_lback: {
            if (wl_rgb_timer == 0x00) {
#    ifdef RGB_MATRIX_BLINK_INDEX_USB
                if (index == RGB_MATRIX_BLINK_INDEX_USB) {
                    rgb_matrix_blink_set_interval_times(index, USB_CONN_BLINK_INTERVAL, 1);
                    rgb_matrix_blink_set(index);
                    wl_rgb_timer = timer_read32();
                    break;
                }
#    endif
                rgb_matrix_blink_set_interval_times(index, WL_LBACK_BLINK_INTERVAL, 1); // 2Hz
                rgb_matrix_blink_set(index);
                wl_rgb_timer = timer_read32();
            } else {
                if (timer_elapsed32(wl_rgb_timer) >= WL_LBACK_TIMEOUT) {
                    wl_rgb_indicator_s = wls_lback_timeout;
                    rgb_blink_cb(index);
                } else {
#    ifdef RGB_MATRIX_BLINK_INDEX_USB
                    if (index == RGB_MATRIX_BLINK_INDEX_USB) {
                        if (USB_DRIVER.state == USB_ACTIVE) {
                            wl_rgb_indicator_s = wls_lback_succeed;
                            rgb_blink_cb(index);
                        } else {
                            rgb_matrix_blink_set(index);
                        }
                        break;
                    }
#    endif
                    if (bts_info.bt_info.paired) {
                        wl_rgb_indicator_s = wls_lback_succeed;
                        rgb_blink_cb(index);
                    } else {
                        if (mm_eeconfig.devs != DEVS_USB) {
                            rgb_matrix_blink_set(index);
                        } else {
                            wl_rgb_indicator_s = wls_none;
                        }
                    }
                }
            }
        } break;
        case wls_pair: {
            if (wl_rgb_timer == 0x00) {
                rgb_matrix_blink_set_interval_times(index, WL_PAIR_BLINK_INTERVAL, 1); // 5Hz
                rgb_matrix_blink_set(index);
                wl_rgb_timer = timer_read32();
            } else {
                if (timer_elapsed32(wl_rgb_timer) >= WL_PAIR_TIMEOUT) {
                    wl_rgb_indicator_s = wls_pair_timeout;
                    rgb_blink_cb(index);
                } else {
                    if (bts_info.bt_info.paired) {
                        wl_rgb_indicator_s = wls_pair_succeed;
                        rgb_blink_cb(index);
                    } else {
                        if (mm_eeconfig.devs != DEVS_USB) {
                            rgb_matrix_blink_set(index);
                        } else {
                            wl_rgb_indicator_s = wls_none;
                        }
                    }
                }
            }
        } break;
        case wls_lback_succeed:
        case wls_pair_succeed: {
            rgb_matrix_blink_set_interval_times(index, WL_SUCCEED_TIME, 0xFF);
            rgb_matrix_blink_set(index);
            wl_rgb_indicator_s = wls_none;
        } break;
        case wls_lback_timeout:
        case wls_pair_timeout: {
            wl_rgb_indicator_s = wls_none;
            rgb_matrix_blink_set(index);
#    ifdef CLOSE_RGB_ENABLE
            kb_sleep_flag = true;
#    endif
        } break;
    }
}

static void rgb_blink_cb(uint8_t index) {
    switch (index) {
        case RGB_MATRIX_BLINK_INDEX_HOST1: {
            wl_indicators_hook(RGB_MATRIX_BLINK_INDEX_HOST1);
        } break;
        case RGB_MATRIX_BLINK_INDEX_HOST2: {
            wl_indicators_hook(RGB_MATRIX_BLINK_INDEX_HOST2);
        } break;
        case RGB_MATRIX_BLINK_INDEX_HOST3: {
            wl_indicators_hook(RGB_MATRIX_BLINK_INDEX_HOST3);
        } break;
#    ifdef RGB_MATRIX_BLINK_INDEX_HOST4
        case RGB_MATRIX_BLINK_INDEX_HOST4: {
            wl_indicators_hook(RGB_MATRIX_BLINK_INDEX_HOST4);
        } break;
#    endif
#    ifdef RGB_MATRIX_BLINK_INDEX_HOST5
        case RGB_MATRIX_BLINK_INDEX_HOST5: {
            wl_indicators_hook(RGB_MATRIX_BLINK_INDEX_HOST5);
        } break;
#    endif
        case RGB_MATRIX_BLINK_INDEX_2G4: {
            wl_indicators_hook(RGB_MATRIX_BLINK_INDEX_2G4);
        } break;
#    ifdef RGB_MATRIX_BLINK_INDEX_USB
        case RGB_MATRIX_BLINK_INDEX_USB: {
            wl_indicators_hook(RGB_MATRIX_BLINK_INDEX_USB);
        } break;
#    endif
#    ifdef RGB_MATRIX_BLINK_INDEX_ALL
        case RGB_MATRIX_BLINK_INDEX_ALL: {
            // reserved
        } break;
#    endif
        default: {
            wl_rgb_indicator_s = wls_none;
        } break;
    }
}

void wl_rgb_indicator_set(uint8_t index, uint8_t status) {
    // note: The states can only be wls_lback and wls_pair
    wl_rgb_indicator_s = wls_none;
    wl_rgb_timer       = 0;
    rgb_matrix_blink_set_remain_time(RGB_MATRIX_BLINK_INDEX_HOST1, 0x00);
    rgb_matrix_blink_set_remain_time(RGB_MATRIX_BLINK_INDEX_HOST2, 0x00);
    rgb_matrix_blink_set_remain_time(RGB_MATRIX_BLINK_INDEX_HOST3, 0x00);
#    ifdef RGB_MATRIX_BLINK_INDEX_HOST4
    rgb_matrix_blink_set_remain_time(RGB_MATRIX_BLINK_INDEX_HOST4, 0x00);
#    endif
#    ifdef RGB_MATRIX_BLINK_INDEX_HOST5
    rgb_matrix_blink_set_remain_time(RGB_MATRIX_BLINK_INDEX_HOST5, 0x00);
#    endif
    rgb_matrix_blink_set_remain_time(RGB_MATRIX_BLINK_INDEX_2G4, 0x00);
#    ifdef RGB_MATRIX_BLINK_INDEX_USB
    rgb_matrix_blink_set_remain_time(RGB_MATRIX_BLINK_INDEX_USB, 0x00);
#    endif
#    ifdef RGB_MATRIX_BLINK_INDEX_ALL
    rgb_matrix_blink_set_remain_time(RGB_MATRIX_BLINK_INDEX_ALL, 0x00);
#    endif
    wl_rgb_indicator_s = status;
    rgb_blink_cb(index);
}

// clang-format off
blink_rgb_t blink_rgbs[RGB_MATRIX_BLINK_COUNT] = {
    {.index = RGB_MATRIX_BLINK_INDEX_HOST1, .interval = 250, .times = 1, .time = 0, .flip = false, .color = {RGB_MATRIX_BLINK_HOST1_COLOR}, .blink_cb = rgb_blink_cb, .remain_time = 0x00},
    {.index = RGB_MATRIX_BLINK_INDEX_HOST2, .interval = 250, .times = 1, .time = 0, .flip = false, .color = {RGB_MATRIX_BLINK_HOST2_COLOR}, .blink_cb = rgb_blink_cb, .remain_time = 0x00},
    {.index = RGB_MATRIX_BLINK_INDEX_HOST3, .interval = 250, .times = 1, .time = 0, .flip = false, .color = {RGB_MATRIX_BLINK_HOST3_COLOR}, .blink_cb = rgb_blink_cb, .remain_time = 0x00},
    #    ifdef RGB_MATRIX_BLINK_INDEX_HOST4
    {.index = RGB_MATRIX_BLINK_INDEX_HOST4, .interval = 250, .times = 1, .time = 0, .flip = false, .color = {RGB_MATRIX_BLINK_HOST4_COLOR}, .blink_cb = rgb_blink_cb, .remain_time = 0x00},
    #endif
    #    ifdef RGB_MATRIX_BLINK_INDEX_HOST5
    {.index = RGB_MATRIX_BLINK_INDEX_HOST5, .interval = 250, .times = 1, .time = 0, .flip = false, .color = {RGB_MATRIX_BLINK_HOST5_COLOR}, .blink_cb = rgb_blink_cb, .remain_time = 0x00},
    #endif
    {.index = RGB_MATRIX_BLINK_INDEX_2G4, .interval = 250, .times = 1, .time = 0, .flip = false, .color = {RGB_MATRIX_BLINK_2G4_COLOR}, .blink_cb = rgb_blink_cb, .remain_time = 0x00},
#    ifdef RGB_MATRIX_BLINK_INDEX_USB
    {.index = RGB_MATRIX_BLINK_INDEX_USB, .interval = 250, .times = 1, .time = 0, .flip = false, .color = {RGB_MATRIX_BLINK_USB_COLOR}, .blink_cb = rgb_blink_cb, .remain_time = 0x00},
#    endif
#ifdef RGB_MATRIX_BLINK_INDEX_ALL
    {.index = RGB_MATRIX_BLINK_INDEX_ALL, .interval = 250, .times = 1, .time = 0, .flip = false, .color = {RGB_MATRIX_BLINK_ALL_COLOR}, .blink_cb = rgb_blink_cb, .remain_time = 0x00},
#endif
};
// clang-format on
#endif

#ifdef MATRIX_LONG_PRESS
static void bt_long_press_keys_cb(uint16_t keycode);

typedef struct {
    uint32_t press_time;
    uint32_t long_time;
    uint16_t keycode;
    uint8_t  upinvalid;
    void (*event_cb)(uint16_t);
} bt_long_key_t;

#    define NUM_LONG_PRESS_KEYS (sizeof(bt_long_press_keys) / sizeof(bt_long_key_t))

static void bt_long_press_keys_cb(uint16_t keycode);

bt_long_key_t bt_long_press_keys[] = {
    {.keycode = BT_HOST1, .press_time = 0, .long_time = 3000, .upinvalid = true, .event_cb = &bt_long_press_keys_cb},
    {.keycode = BT_HOST2, .press_time = 0, .long_time = 3000, .upinvalid = true, .event_cb = &bt_long_press_keys_cb},
    {.keycode = BT_HOST3, .press_time = 0, .long_time = 3000, .upinvalid = true, .event_cb = &bt_long_press_keys_cb},
    {.keycode = BT_2G4, .press_time = 0, .long_time = 3000, .upinvalid = true, .event_cb = &bt_long_press_keys_cb},
};

static void bt_long_press_keys_cb(uint16_t keycode) {
    switch (keycode) {
        case BT_HOST1: {
            BT_DEBUG_INFO("reset to host1\n");
            mm_switch_mode(mm_eeconfig.devs, DEVS_HOST1, true);
        } break;
        case BT_HOST2: {
            BT_DEBUG_INFO("reset to host2\n");
            mm_switch_mode(mm_eeconfig.devs, DEVS_HOST2, true);
        } break;
        case BT_HOST3: {
            BT_DEBUG_INFO("reset to host3\n");
            mm_switch_mode(mm_eeconfig.devs, DEVS_HOST3, true);
        } break;
        case BT_2G4: {
            BT_DEBUG_INFO("reset to 2g4\n");
            mm_switch_mode(mm_eeconfig.devs, DEVS_2G4, true);
        } break;
        default:
            break;
    }
}

void bt_proc_long_press_keys(void) {
    for (uint8_t i = 0; i < NUM_LONG_PRESS_KEYS; i++) {
        if ((bt_long_press_keys[i].press_time != 0) && (timer_elapsed32(bt_long_press_keys[i].press_time) >= bt_long_press_keys[i].long_time)) {
            bt_long_press_keys[i].event_cb(bt_long_press_keys[i].keycode);
            bt_long_press_keys[i].press_time = 0;
        }
    }
}
#endif

#ifdef MULTIMODE_ENABLE
static void charging_indication(void);
static void low_battery_warning(void);
static void low_battery_shutdown(void);
static void show_device_state(void);
static void battery_vol_display(void);

bool bt_process_record_user(uint16_t keycode, keyrecord_t *record) {
#    ifdef MATRIX_LONG_PRESS
    for (uint8_t i = 0; i < NUM_LONG_PRESS_KEYS; i++) {
        if (keycode == bt_long_press_keys[i].keycode) {
            if (record->event.pressed) {
                bt_long_press_keys[i].press_time = timer_read32();
            } else {
                bt_long_press_keys[i].press_time = 0;
            }
            break;
        }
    }
#    endif

#    ifdef CLOSE_RGB_ENABLE
    open_rgb();
#    endif

#    if defined(MM_BT_MODE_PIN) && defined(MM_2G4_MODE_PIN)
    switch (now_mode) {
        case MM_MODE_USB:
            if (keycode >= BT_HOST1 && keycode <= BT_2G4) {
                return false;
            }
            break;
        case MM_MODE_BT:
            if (keycode >= BT_2G4 && keycode <= BT_USB) {
                return false;
            }
            break;
        case MM_MODE_2G4:
            if (keycode >= BT_HOST1 && keycode <= BT_USB) {
                return false;
            }
        default:
            break;
    }
#    endif

    switch (keycode) {
        case BT_HOST1: {
            if (record->event.pressed && (mm_eeconfig.devs != DEVS_HOST1)) {
                BT_DEBUG_INFO("switch to host1\n");
                mm_switch_mode(mm_eeconfig.devs, DEVS_HOST1, false);
            }
            return false;
        } break;
        case BT_HOST2: {
            if (record->event.pressed && (mm_eeconfig.devs != DEVS_HOST2)) {
                BT_DEBUG_INFO("switch to host2\n");
                mm_switch_mode(mm_eeconfig.devs, DEVS_HOST2, false);
            }
            return false;
        } break;
        case BT_HOST3: {
            if (record->event.pressed && (mm_eeconfig.devs != DEVS_HOST3)) {
                BT_DEBUG_INFO("switch to host3\n");
                mm_switch_mode(mm_eeconfig.devs, DEVS_HOST3, false);
            }
            return false;
        } break;
        case BT_2G4: {
            if (record->event.pressed && (mm_eeconfig.devs != DEVS_2G4)) {
                BT_DEBUG_INFO("switch to 2g4\n");
                mm_switch_mode(mm_eeconfig.devs, DEVS_2G4, false);
            }
            return false;
        } break;
        case BT_USB: {
            if (record->event.pressed) {
                BT_DEBUG_INFO("switch to usb\n");
                mm_switch_mode(mm_eeconfig.devs, DEVS_USB, false);
            }
        } break;

        /* user code*/
        case BT_VOL: {
            if (record->event.pressed) {
                query_vol_flag = true;
            } else {
                query_vol_flag = false;
            }
        } break;
        default: {
            return true;
        }
    }

    return false;
}

static void charging_indication(void) {
    static uint32_t charging_time = 0;

    if (get_battery_charge_state() == BATTERY_STATE_CHARGING) {
        // 正在充电
        if (timer_elapsed32(charging_time) >= 2000) {
            rgb_matrix_set_color(0, RGB_RED); // 红灯
        }
    } else {
        charging_time = timer_read32();
    }
}

static void low_battery_warning(void) {
    static bool     Low_power_bink = false;
    static uint32_t Low_power_time = 0;

    if (bts_info.bt_info.low_vol) {
        rgb_matrix_set_color_all(RGB_OFF);
        if (timer_elapsed32(Low_power_time) >= 500) {
            Low_power_bink = !Low_power_bink;
            Low_power_time = timer_read32();
        }
        if (Low_power_bink) {
            rgb_matrix_set_color(0, RGB_RED); // 红色
        } else {
            rgb_matrix_set_color(0, RGB_OFF);
        }
    } else {
        Low_power_bink = 0;
    }
}

static void low_battery_shutdown(void) {
    extern bool low_vol_offed_sleep;
    if (bts_info.bt_info.low_vol_offed) {
        kb_sleep_flag       = true;
        low_vol_offed_sleep = true;
    }
}

battery_charge_state_t get_battery_charge_state(void) {
#    if defined(MM_CABLE_PIN) && defined(MM_CHARGE_PIN)
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
#    else
    return BATTERY_STATE_UNPLUGGED;
#    endif
}

static void show_device_state(void) {
    // FN 按下时显示当前无线设备状态
    if ((get_highest_layer(default_layer_state | layer_state) == 1) || (get_highest_layer(default_layer_state | layer_state) == 3)) {
        switch (mm_eeconfig.devs) {
            case DEVS_HOST1: {
                rgb_matrix_set_color(RGB_MATRIX_BLINK_INDEX_HOST1, RGB_MATRIX_BLINK_HOST1_COLOR);
            } break;
            case DEVS_HOST2: {
                rgb_matrix_set_color(RGB_MATRIX_BLINK_INDEX_HOST2, RGB_MATRIX_BLINK_HOST2_COLOR);
            } break;
            case DEVS_HOST3: {
                rgb_matrix_set_color(RGB_MATRIX_BLINK_INDEX_HOST3, RGB_MATRIX_BLINK_HOST3_COLOR);
            } break;
            case DEVS_2G4: {
                rgb_matrix_set_color(RGB_MATRIX_BLINK_INDEX_2G4, RGB_MATRIX_BLINK_2G4_COLOR);
            } break;
            case DEVS_USB: {
                rgb_matrix_set_color(RGB_MATRIX_BLINK_INDEX_USB, RGB_MATRIX_BLINK_USB_COLOR);
            } break;
            default:
                break;
        }
    }
}

static void battery_vol_display(void) {
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

bool bt_rgb_matrix_indicators_user(void) {
    // Low battery warning
    if ((mm_eeconfig.devs != DEVS_USB) && (get_battery_charge_state() == BATTERY_STATE_UNPLUGGED)) {
        low_battery_warning();
    }

    // Show the current device state
    show_device_state();

    // Battery voltage display
    battery_vol_display();

    // Device connection indication
#    ifdef RGB_MATRIX_BLINK_ENABLE
    rgb_matrix_blink_task();
#    endif

    // Charging indication
    charging_indication();

    return true;
}

void bt_housekeeping_task_user(void) {
    if ((mm_eeconfig.devs != DEVS_USB) && (get_battery_charge_state() == BATTERY_STATE_UNPLUGGED)) {
        low_battery_shutdown();
    }
}

void bt_keyboard_post_init_user(void) {}

bool get_kb_sleep_flag(void) {
#    ifdef CLOSE_RGB_ENABLE
    return kb_sleep_flag;
#    else
    return false;
#    endif
}
#endif
