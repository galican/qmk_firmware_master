/* Copyright (C) 2022 jonylee@hfd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include QMK_KEYBOARD_H
#ifdef MULTIMODE_ENABLE
#    include "bt_task.h"
#endif
#include "usb_main.h"

void matrix_init_kb(void) {
    static bool is_inited = false;

    if (!is_inited) {
        is_inited = true;
#ifdef MULTIMODE_ENABLE
        bt_init();
#endif
    }

    matrix_init_user();
}

bool led_inited = false;

void led_config_all(void) {
    if (!led_inited) {
#ifdef WS2812_EN_PIN
        setPinOutputPushPull(WS2812_EN_PIN);
        writePinHigh(WS2812_EN_PIN);
#endif
        led_inited = true;
    }
}

void led_deconfig_all(void) {
    if (led_inited) {
#ifdef WS2812_EN_PIN
        setPinOutputOpenDrain(WS2812_EN_PIN);
        // writePinLow(WS2812_EN_PIN);
#endif
        led_inited = false;
    }
}

void housekeeping_task_kb(void) {
#ifdef MULTIMODE_ENABLE
    bt_task();
#endif

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

    extern void housekeeping_task_bt(void);
    housekeeping_task_bt();

    housekeeping_task_user();
}

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
    if (!process_record_user(keycode, record)) {
        return false;
    }

    switch (keycode) {
        case QK_RGB_MATRIX_TOGGLE:
            if (record->event.pressed) {
                switch (rgb_matrix_get_flags()) {
                    case LED_FLAG_ALL: {
                        rgb_matrix_set_flags(LED_FLAG_NONE);
                        rgb_matrix_set_color_all(0, 0, 0);
                    } break;
                    default: {
                        rgb_matrix_set_flags(LED_FLAG_ALL);
                    } break;
                }
            }
            if (!rgb_matrix_is_enabled()) {
                rgb_matrix_set_flags(LED_FLAG_ALL);
                rgb_matrix_enable();
            }
            return false;
        default:
            break;
    }
    return true;
}

bool rgb_matrix_indicators_kb(void) {
    if (!rgb_matrix_get_flags()) {
        rgb_matrix_set_color_all(RGB_OFF);
    }

    if (!rgb_matrix_indicators_user()) {
        return false;
    }

    return true;
}

void keyboard_pre_init_kb(void) {
#ifdef WS2812_EN_PIN
    setPinOutputPushPull(WS2812_EN_PIN);
    writePinHigh(WS2812_EN_PIN);
#endif
}

void suspend_power_down_kb(void) {
    led_deconfig_all();
}
