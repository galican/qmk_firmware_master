// Copyright 2023 JoyLee (@itarze)
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H
#include "common/bt_task.h"
#include "usb_main.h"

// clang-format off

#ifdef RGB_MATRIX_ENABLE
const is31fl3733_led_t PROGMEM g_is31fl3733_leds[RGB_MATRIX_LED_COUNT] = {
/* Refer to IS31 manual for these locations
 *   driver
 *   |   R location
 *   |   |     G location
 *   |   |     |     B location
 *   |   |     |     | */

    {0, SW1_CS1,   SW2_CS1,   SW3_CS1},
    {0, SW1_CS2,   SW2_CS2,   SW3_CS2},
    {0, SW1_CS3,   SW2_CS3,   SW3_CS3},
    {0, SW1_CS4,   SW2_CS4,   SW3_CS4},

    {0, SW1_CS5,   SW2_CS5,   SW3_CS5},
    {0, SW1_CS6,   SW2_CS6,   SW3_CS6},
    {0, SW1_CS7,   SW2_CS7,   SW3_CS7},
    {0, SW1_CS8,   SW2_CS8,   SW3_CS8},

    {0, SW4_CS1,   SW5_CS1,   SW6_CS1},
    {0, SW4_CS2,   SW5_CS2,   SW6_CS2},
    {0, SW4_CS3,   SW5_CS3,   SW6_CS3},
    {0, SW4_CS4,   SW5_CS4,   SW6_CS4},

    {0, SW4_CS5,   SW5_CS5,   SW6_CS5},
    {0, SW4_CS6,   SW5_CS6,   SW6_CS6},
    {0, SW4_CS7,   SW5_CS7,   SW6_CS7},
    {0, SW4_CS8,   SW5_CS8,   SW6_CS8},

    {0, SW7_CS1,   SW8_CS1,   SW9_CS1},
    {0, SW7_CS2,   SW8_CS2,   SW9_CS2},
    {0, SW7_CS3,   SW8_CS3,   SW9_CS3},
    {0, SW7_CS4,   SW8_CS4,   SW9_CS4},

    {0, SW7_CS5,   SW8_CS5,   SW9_CS5},
    {0, SW7_CS6,   SW8_CS6,   SW9_CS6},

    {0, SW1_CS9,   SW2_CS9,   SW3_CS9},
};
#endif
// clang-format on
bool led_inited = false;

void led_config_all(void) {
    if (!led_inited) {
        setPinOutputPushPull(RGB_DRIVER_SDB_PIN);
        writePinHigh(RGB_DRIVER_SDB_PIN);
        led_inited = true;
    }
}

void led_deconfig_all(void) {
    if (led_inited) {
        setPinOutputPushPull(RGB_DRIVER_SDB_PIN);
        writePinLow(RGB_DRIVER_SDB_PIN);
        led_inited = false;
    }
}

void suspend_power_down_user(void) {
    // code will run multiple times while keyboard is suspended
    led_deconfig_all();
}

void suspend_wakeup_init_user(void) {
    // code will run on keyboard wakeup
    led_config_all();
}

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
    if (process_record_user(keycode, record) != true) {
        return false;
    }
    switch (keycode) {
        case RGB_TOG:
            if (record->event.pressed) {
                if (bts_info.bt_info.pvol <= 5) {
                    return false;
                }

                if (per_info.backlight_off) {
                    // 开启背光
                    per_info.backlight_off = false;

                    uint8_t target_mode = per_info.saved_rgb_mode;
                    if (target_mode > 21 || target_mode == RGB_MATRIX_CUSTOM_EFFECT_OFF) {
                        target_mode = rgb_matrix_config.mode;
                        if (target_mode == RGB_MATRIX_CUSTOM_EFFECT_OFF || target_mode > 21) {
                            target_mode = RGB_MATRIX_DEFAULT_MODE;
                        }
                    }
                    rgb_matrix_mode(target_mode);

                } else {
                    // 关闭背光
                    per_info.backlight_off = true;

                    uint8_t current_mode = rgb_matrix_get_mode();
                    if (current_mode <= 21 && current_mode != RGB_MATRIX_CUSTOM_EFFECT_OFF) {
                        per_info.saved_rgb_mode = current_mode;
                    } else {
                        per_info.saved_rgb_mode = RGB_MATRIX_DEFAULT_MODE;
                    }
                    rgb_matrix_mode(RGB_MATRIX_CUSTOM_EFFECT_OFF);
                }

                // validate_per_info_ranges();
                eeconfig_update_kb(per_info.raw);
            }
            return false;

        case RGB_MOD:
            if (record->event.pressed) {
                if (bts_info.bt_info.pvol <= 5) {
                    return false;
                }

                // 如果背光关闭，先开启
                if (per_info.backlight_off) {
                    per_info.backlight_off = false;
                    eeconfig_update_kb(per_info.raw);
                }

                rgb_matrix_step();
                // 跳过关闭模式
                if (rgb_matrix_get_mode() == RGB_MATRIX_CUSTOM_EFFECT_OFF) {
                    rgb_matrix_step();
                }

                // 保存新模式
                uint8_t new_mode = rgb_matrix_get_mode();
                if (new_mode <= 21) {
                    per_info.saved_rgb_mode = new_mode;
                    eeconfig_update_kb(per_info.raw);
                }
            }
            return false;

        case RGB_RMOD:
            if (record->event.pressed) {
                if (bts_info.bt_info.pvol <= 5) {
                    return false;
                }

                // 如果背光关闭，先开启
                if (per_info.backlight_off) {
                    per_info.backlight_off = false;
                    eeconfig_update_kb(per_info.raw);
                }

                rgb_matrix_step_reverse();
                // 跳过关闭模式
                if (rgb_matrix_get_mode() == RGB_MATRIX_CUSTOM_EFFECT_OFF) {
                    rgb_matrix_step_reverse();
                }

                // 保存新模式
                uint8_t new_mode = rgb_matrix_get_mode();
                if (new_mode <= 21) {
                    per_info.saved_rgb_mode = new_mode;
                    eeconfig_update_kb(per_info.raw);
                }
            }
            return false;
        default:
            break;
    }
#ifdef BT_MODE_ENABLE
    if (process_record_bt(keycode, record) != true) {
        return false;
    }
#endif
    return true;
}

void matrix_init_kb(void) {
#ifdef WS2812_EN_PIN
    setPinOutput(WS2812_EN_PIN);
    writePinLow(WS2812_EN_PIN);
#endif

#ifdef BT_MODE_ENABLE
    bt_init(); // 使用新的初始化函数
    led_config_all();
#endif

    matrix_init_user();
}

void keyboard_post_init_kb(void) {
    per_info.raw = eeconfig_read_kb();

    if (per_info.backlight_off) {
        // 用户关闭了背光，设置为关闭模式
        rgb_matrix_mode_noeeprom(RGB_MATRIX_CUSTOM_EFFECT_OFF);
    } else {
        // 用户开启了背光，恢复保存的模式
        uint8_t target_mode = per_info.saved_rgb_mode;
        if (target_mode > 21 || target_mode == RGB_MATRIX_CUSTOM_EFFECT_OFF) {
            target_mode = RGB_MATRIX_DEFAULT_MODE;
        }
        rgb_matrix_mode_noeeprom(target_mode);
    }

    keyboard_post_init_user();
}

void eeconfig_init_kb(void) {
    // 设置默认值
    per_info.sleep_mode      = 1;                       // 默认睡眠模式
    per_info.ind_brightness  = 200;                     // 默认亮度
    per_info.smd_color_index = 0;                       // 默认颜色
    per_info.ind_color_index = 0;                       // 默认颜色
    per_info.backlight_off   = true;                    // 默认关闭背光
    per_info.eco_tog_flag    = false;                   // 默认关闭省电
    per_info.saved_rgb_mode  = RGB_MATRIX_DEFAULT_MODE; // 默认RGB模式
    per_info.manual_usb_mode = false;                   //
    per_info.reserved        = 0;                       // 预留位清零

    // RGB配置
    rgb_matrix_config.hsv.h = 170;
    rgb_matrix_config.mode  = RGB_MATRIX_CUSTOM_EFFECT_OFF;

    // validate_per_info_ranges(); // 验证范围

    eeconfig_update_kb(per_info.raw);
    eeconfig_update_rgb_matrix(&rgb_matrix_config);

    dev_info.custom_numlock = true; // 默认启用自定义 NumLock 状态
    eeconfig_update_user(dev_info.raw);

    eeconfig_init_user();
}

void matrix_scan_kb(void) {
#ifdef BT_MODE_ENABLE
    bt_task();
#endif

#ifdef USB_SUSPEND_CHECK_ENABLE
    static uint32_t usb_suspend_timer = 0;
    static uint32_t usb_suspend       = false;

    if (dev_info.devs == DEVS_USB) {
        if (USB_DRIVER.state != USB_ACTIVE || USB_DRIVER.state == USB_SUSPENDED) {
            // USB挂起状态
            if (!usb_suspend_timer) {
                // 开始计时
                usb_suspend_timer = timer_read32();
                dprintf("USB suspended, starting 10s timer\n");
            } else if (timer_elapsed32(usb_suspend_timer) > 10000) {
                // 挂起超过10秒，关闭背光
                dprintf("USB suspended for 10s, turning off lights\n");
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
                dprintf("USB resumed, canceling suspend timer\n");
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
    matrix_scan_user();
}

void housekeeping_task_kb(void) {
#ifdef BT_MODE_ENABLE
    extern void housekeeping_task_bt(void);
    housekeeping_task_bt();
#endif

#ifdef NKRO_ENABLE
    if (dev_info.devs) {
        static uint8_t nkro_mode = true;
        do {
            nkro_mode = keymap_config.nkro;
            if (nkro_mode) {
                bts_set_nkro(true);
            } else {
                bts_set_nkro(false);
            }
        } while (nkro_mode != keymap_config.nkro);
    }
#endif // NKRO_ENABLE

#ifdef CONSOLE_ENABLE
    debug_enable = true;
#endif
}

bool rgb_matrix_indicators_advanced_kb(uint8_t led_min, uint8_t led_max) {
    if (rgb_matrix_indicators_advanced_user(led_min, led_max) != true) {
        return false;
    }

#ifdef BT_MODE_ENABLE
    if (bt_indicator_rgb(led_min, led_max) != true) {
        return false;
    }
#endif

    return true;
}

#ifdef DIP_SWITCH_ENABLE
// 拨动开关选择系统模式
bool dip_switch_update_kb(uint8_t index, bool active) {
    if (!dip_switch_update_user(index, active)) {
        return false;
    }
    if (index == 0) {
        default_layer_set(1UL << ((!active) ? 0 : 1));
        if (!active) {
            layer_state_set(0);
        } else {
            layer_state_set(1);
        }
    }
    return true;
}

#endif // DIP_SWITCH_ENABLE
