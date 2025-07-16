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
#    include "multimode.h"
#endif

void matrix_init_kb(void) {
    static bool is_inited = false;

    if (!is_inited) {
        is_inited = true;
#ifdef MULTIMODE_ENABLE
        mm_init();
#endif
    }

    matrix_init_user();
}

void matrix_scan_kb(void) {
#ifdef MULTIMODE_ENABLE
    mm_task();
#endif

    matrix_scan_user();
}

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
    if (!process_record_user(keycode, record)) {
        return false;
    }

#ifdef MULTIMODE_ENABLE
    if (process_record_multimode(keycode, record) != true) {
        return false;
    }
#endif
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
    // rgb_matrix_set_color(79, RGB_OFF);
    // rgb_matrix_set_color(78, RGB_OFF);
    // rgb_matrix_set_color(76, RGB_OFF);
    // rgb_matrix_set_color(75, RGB_OFF);
    if (!rgb_matrix_indicators_user()) {
        return false;
    }

    return true;
}

#ifdef RGB_MATRIX_ENABLE
#    ifdef RGB_MATRIX_BLINK_ENABLE
#        include "rgb_matrix_blink.h"
#    endif
bool rgb_matrix_indicators_advanced_kb(uint8_t led_min, uint8_t led_max) {
    if (rgb_matrix_indicators_advanced_user(led_min, led_max) != true) {
        return false;
    }

#    ifdef RGB_MATRIX_BLINK_ENABLE
    rgb_matrix_blink_task(led_min, led_max);
#    endif

    return true;
}
#endif

void keyboard_pre_init_kb(void) {
    setPinOutputPushPull(RGB_MATRIX_SHUTDOWN_PIN);
    writePinHigh(RGB_MATRIX_SHUTDOWN_PIN);
}

void suspend_power_down_kb(void) {
    writePinLow(RGB_MATRIX_SHUTDOWN_PIN);
}

void suspend_wakeup_init_kb(void) {
    writePinHigh(RGB_MATRIX_SHUTDOWN_PIN);
}
