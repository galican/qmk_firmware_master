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

void housekeeping_task_kb(void) {
#ifdef MULTIMODE_ENABLE
    mm_task();
#endif

    housekeeping_task_user();
}

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
    if (!process_record_user(keycode, record)) {
        return false;
    }

#ifdef MULTIMODE_ENABLE
    if (!process_record_multimode(keycode, record)) {
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
    if (!rgb_matrix_get_flags()) {
        rgb_matrix_set_color_all(RGB_OFF);
    }

    if (!rgb_matrix_indicators_user()) {
        return false;
    }

    return true;
}

void keyboard_pre_init_kb(void) {
    setPinOutputPushPull(RGB_DRIVER_SDB_PIN);
    writePinHigh(RGB_DRIVER_SDB_PIN);
}

void suspend_power_down_kb(void) {
    writePinLow(RGB_DRIVER_SDB_PIN);
}

void suspend_wakeup_init_kb(void) {
    writePinHigh(RGB_DRIVER_SDB_PIN);
}
