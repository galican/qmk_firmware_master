/**
 * @file bt_task.c
 * @brief
 * @author JoyLee
 * @version 2.0.0
 * @date 2023-04-06
 *
 * @copyright Copyright (c) 2023 Westberry Technology Corp., Ltd
 */

#ifdef MULTIMODE_ENABLE
#    include "multimode.h"
#endif
// #include "multimode.h"
// #include "rgb_matrix_blink.h"

#ifdef CLOSE_RGB_ENABLE
// Global variables for RGB control
static uint32_t key_press_time;
static uint32_t close_rgb_time;
static bool     bak_rgb_toggle;
static bool     sober         = true;
static bool     kb_sleep_flag = false;

extern void led_config_all(void);
extern void led_deconfig_all(void);

bool led_inited = false;

void led_config_all(void) {
    if (!led_inited) {
        // Set our LED pins as output

        /* user code*/

        // led_set(host_keyboard_leds());
        led_inited = true;
    }
}

void led_deconfig_all(void) {
    if (led_inited) {
        // Set our LED pins as input

        /* user code*/

        led_inited = false;
    }
}

void open_rgb(void) {
    key_press_time = timer_read32();
    if (!sober) {
        if (bak_rgb_toggle) {
#    ifdef RGB_DRIVER_SDB_PIN
            writePinHigh(RGB_DRIVER_SDB_PIN);
#    endif
            kb_sleep_flag = false;
            rgb_matrix_enable_noeeprom();
            bts_info.bt_info.active = true;
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
            bts_info.bt_info.active = false;
#    ifdef RGB_DRIVER_SDB_PIN
            writePinLow(RGB_DRIVER_SDB_PIN);
#    endif
        }
    } else {
        if (!rgb_matrix_config.enable) {
            if (timer_elapsed32(close_rgb_time) >= 100) {
                /* Turn off all indicators led */
                if (led_inited) {
                    led_deconfig_all();
                }

                lp_system_sleep();
                mm_switch_mode(DEVS_USB, mm_eeconfig.last_devs, false);
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

enum wl_indicator_status {
    wls_none = 0,
    wls_lback,
    wls_pair,
    wls_lback_succeed,
    wls_pair_succeed,
    wls_lback_timeout,
    wls_pair_timeout,
};

uint8_t  wl_rgb_indicator_s = 0;
uint32_t wl_rgb_timer       = 0;

void wl_indicators_hook(uint8_t index) {
    extern void rgb_blink_cb(uint8_t index);

    switch (wl_rgb_indicator_s) {
        case wls_none: {
            wl_rgb_timer = 0;
        } break;
        case wls_lback: {
            if (wl_rgb_timer == 0x00) {
                switch (index) {
                    case RGB_MATRIX_BLINK_INDEX_HOST1: {
                        rgb_matrix_blink_set_color(index, RGB_MATRIX_BLINK_HOST1_COLOR);
                    } break;
                    case RGB_MATRIX_BLINK_INDEX_HOST2: {
                        rgb_matrix_blink_set_color(index, RGB_MATRIX_BLINK_HOST2_COLOR);
                    } break;
                    case RGB_MATRIX_BLINK_INDEX_HOST3: {
                        rgb_matrix_blink_set_color(index, RGB_MATRIX_BLINK_HOST3_COLOR);
                    } break;
                    case RGB_MATRIX_BLINK_INDEX_2G4: {
                        rgb_matrix_blink_set_color(index, RGB_MATRIX_BLINK_2G4_COLOR);
                    } break;
#    ifdef RGB_MATRIX_BLINK_INDEX_USB
                    case RGB_MATRIX_BLINK_INDEX_USB: {
                        rgb_matrix_blink_set_color(index, RGB_OFF);
                        rgb_matrix_blink_set_interval_times(index, USB_CONN_BLINK_INTERVAL, 0xFF); // 长亮
                        rgb_matrix_blink_set(index);
                        wl_rgb_timer = timer_read32();
                        return;
                    } break;
#    endif
                    default:
                        break;
                }
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
                            rgb_matrix_blink_set_color(index, RGB_MATRIX_BLINK_USB_COLOR);
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
                switch (index) {
                    case RGB_MATRIX_BLINK_INDEX_HOST1: {
                        rgb_matrix_blink_set_color(index, RGB_MATRIX_BLINK_HOST1_COLOR);
                    } break;
                    case RGB_MATRIX_BLINK_INDEX_HOST2: {
                        rgb_matrix_blink_set_color(index, RGB_MATRIX_BLINK_HOST2_COLOR);
                    } break;
                    case RGB_MATRIX_BLINK_INDEX_HOST3: {
                        rgb_matrix_blink_set_color(index, RGB_MATRIX_BLINK_HOST3_COLOR);
                    } break;
                    case RGB_MATRIX_BLINK_INDEX_2G4: {
                        rgb_matrix_blink_set_color(index, RGB_MATRIX_BLINK_2G4_COLOR);
                    } break;
                    default:
                        return;
                }
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
        } break;
    }
}

void rgb_blink_cb(uint8_t index) {
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
        case RGB_MATRIX_BLINK_INDEX_ALL: {
            // reserved
        } break;
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
    rgb_matrix_blink_set_remain_time(RGB_MATRIX_BLINK_INDEX_2G4, 0x00);
#    ifdef RGB_MATRIX_BLINK_INDEX_USB
    rgb_matrix_blink_set_remain_time(RGB_MATRIX_BLINK_INDEX_USB, 0x00);
#    endif
    wl_rgb_indicator_s = status;
    rgb_blink_cb(index);
}

// clang-format off
blink_rgb_t blink_rgbs[RGB_MATRIX_BLINK_COUNT] = {
    {.index = RGB_MATRIX_BLINK_INDEX_HOST1, .interval = 250, .times = 1, .color = {.r = 0x00, .g = 0xFF, .b = 0x00}, .blink_cb = rgb_blink_cb},
    {.index = RGB_MATRIX_BLINK_INDEX_HOST2, .interval = 250, .times = 1, .color = {.r = 0x00, .g = 0xFF, .b = 0x00}, .blink_cb = rgb_blink_cb},
    {.index = RGB_MATRIX_BLINK_INDEX_HOST3, .interval = 250, .times = 1, .color = {.r = 0x00, .g = 0xFF, .b = 0x00}, .blink_cb = rgb_blink_cb},
    {.index = RGB_MATRIX_BLINK_INDEX_2G4, .interval = 250, .times = 1, .color = {.r = 0x00, .g = 0xFF, .b = 0x00}, .blink_cb = rgb_blink_cb},
#    ifdef RGB_MATRIX_BLINK_INDEX_USB
    {.index = RGB_MATRIX_BLINK_INDEX_USB, .interval = 250, .times = 1, .color = {.r = 0x00, .g = 0xFF, .b = 0x00}, .blink_cb = rgb_blink_cb},
#    endif
    {.index = RGB_MATRIX_BLINK_INDEX_ALL, .interval = 250, .times = 3, .color = {.r = 0xFF, .g = 0x00, .b = 0x00}, .blink_cb = rgb_blink_cb},
};
// clang-format on
#endif

#ifdef MATRIX_LONG_PRESS
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
    {.keycode = EE_CLR, .press_time = 0, .long_time = 3000, .upinvalid = true, .event_cb = &bt_long_press_keys_cb},
    {.keycode = BT_HOST1, .press_time = 0, .long_time = 3000, .upinvalid = true, .event_cb = &bt_long_press_keys_cb},
    {.keycode = BT_HOST2, .press_time = 0, .long_time = 3000, .upinvalid = true, .event_cb = &bt_long_press_keys_cb},
    {.keycode = BT_HOST3, .press_time = 0, .long_time = 3000, .upinvalid = true, .event_cb = &bt_long_press_keys_cb},
    // {.keycode = BT_HOST4, .press_time = 0, .long_time = 3000, .upinvalid = true, .event_cb = &bt_long_press_keys_cb},
    // {.keycode = BT_HOST5, .press_time = 0, .long_time = 3000, .upinvalid = true, .event_cb = &bt_long_press_keys_cb},
    {.keycode = BT_2G4, .press_time = 0, .long_time = 3000, .upinvalid = true, .event_cb = &bt_long_press_keys_cb},
};

static void bt_long_press_keys_cb(uint16_t keycode) {
    switch (keycode) {
        case EE_CLR: {
            eeconfig_init();
            eeconfig_update_rgb_matrix_default();
            eeconfig_update_multimode_default();
            matrix_init_kb();
#    ifdef RGB_MATRIX_BLINK_ENABLE
            extern bool rgb_matrix_blink_set(uint8_t index);
            rgb_matrix_blink_set(RGB_MATRIX_BLINK_INDEX_ALL);
#    endif
        } break;
        case BT_HOST1: {
            dprintf("reset to host1\n");
            mm_switch_mode(mm_eeconfig.devs, DEVS_HOST1, true);
            wl_rgb_indicator_set(RGB_MATRIX_BLINK_INDEX_HOST1, wls_pair);
        } break;
        case BT_HOST2: {
            dprintf("reset to host2\n");
            mm_switch_mode(mm_eeconfig.devs, DEVS_HOST2, true);
            wl_rgb_indicator_set(RGB_MATRIX_BLINK_INDEX_HOST2, wls_pair);
        } break;
        case BT_HOST3: {
            dprintf("reset to host3\n");
            mm_switch_mode(mm_eeconfig.devs, DEVS_HOST3, true);
            wl_rgb_indicator_set(RGB_MATRIX_BLINK_INDEX_HOST3, wls_pair);
        } break;
        case BT_2G4: {
            dprintf("reset to 2g4\n");
            mm_switch_mode(mm_eeconfig.devs, DEVS_2G4, true);
            wl_rgb_indicator_set(RGB_MATRIX_BLINK_INDEX_2G4, wls_pair);
        }
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

bool bt_process_record_user(uint16_t keycode, keyrecord_t *record) {
#ifdef MATRIX_LONG_PRESS
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
#endif

#ifdef CLOSE_RGB_ENABLE
    open_rgb();
#endif

#ifdef MULTIMODE_ENABLE
#    if defined(MM_BT_MODE_PIN) && defined(MM_2G4_MODE_PIN)
    switch (mm_mode) {
        case MM_MODE_USB:
            if (keycode >= BT_HOST1 && keycode <= BT_2G4) {
                break;
            }
            break;
        case MM_MODE_BT:
            if (keycode >= BT_2G4 && keycode <= BT_USB) {
                break;
            }
            break;
        case MM_MODE_2G4:
            if (keycode >= BT_HOST1 && keycode <= BT_USB) {
                break;
            }
        default:
            break;
    }
#    endif

    switch (keycode) {
        case EE_CLR: {
            return false;
        } break;
        case BT_HOST1: {
            if (record->event.pressed && (mm_eeconfig.devs != DEVS_HOST1)) {
                dprintf("switch to host1\n");
                mm_switch_mode(mm_eeconfig.devs, DEVS_HOST1, false);
                wl_rgb_indicator_set(RGB_MATRIX_BLINK_INDEX_HOST1, wls_lback);
            }
            return false;
        } break;
        case BT_HOST2: {
            if (record->event.pressed && (mm_eeconfig.devs != DEVS_HOST2)) {
                dprintf("switch to host2\n");
                mm_switch_mode(mm_eeconfig.devs, DEVS_HOST2, false);
                wl_rgb_indicator_set(RGB_MATRIX_BLINK_INDEX_HOST2, wls_lback);
            }
            return false;
        } break;
        case BT_HOST3: {
            if (record->event.pressed && (mm_eeconfig.devs != DEVS_HOST3)) {
                dprintf("switch to host3\n");
                mm_switch_mode(mm_eeconfig.devs, DEVS_HOST3, false);
                wl_rgb_indicator_set(RGB_MATRIX_BLINK_INDEX_HOST3, wls_lback);
            }
            return false;
        } break;
        case BT_2G4: {
            if (record->event.pressed && (mm_eeconfig.devs != DEVS_2G4)) {
                dprintf("switch to 2g4\n");
                mm_switch_mode(mm_eeconfig.devs, DEVS_2G4, false);
                wl_rgb_indicator_set(RGB_MATRIX_BLINK_INDEX_2G4, wls_lback);
            }
            return false;
        } break;
        case BT_USB: {
            if (record->event.pressed) {
                dprintf("switch to usb\n");
                mm_switch_mode(mm_eeconfig.devs, DEVS_USB, false);
#    ifdef RGB_MATRIX_BLINK_INDEX_USB
                wl_rgb_indicator_set(RGB_MATRIX_BLINK_INDEX_USB, wls_lback);
#    endif
            }
        } break;

            /* user code*/

        default: {
            return true;
        }
    }

    return false;
}

bool bt_rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    // FN按下显示电量信息
    // if (get_highest_layer(default_layer_state | layer_state) == 1) {
    //     for (uint8_t i = 0; i < 10; i++) {
    //         if (i < (bts_info.bt_info.pvol / 10)) {
    //             rgb_matrix_set_color(1 + i, RGB_WHITE);
    //         } else {
    //             rgb_matrix_set_color(1 + i, 0x00, 0x00, 0x00);
    //         }
    //     }
    // }

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
                rgb_matrix_set_color(RGB_MATRIX_BLINK_INDEX_2G4, RGB_MATRIX_BLINK_2G4_COLOR);
            } break;
            default:
                break;
        }
    }

    return true;
}
#endif
