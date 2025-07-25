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
#include "bled.h"
#ifdef MULTIMODE_ENABLE
#    include "bt_task.h"
#endif

enum _layers {
    WIN_BASE,
    WIN_FN,
    MAC_BASE,
    MAC_FN,
};

#define WIN_DET G(KC_D)
#define KC_TASK G(KC_TAB)
#define WIN_EMO G(KC_DOT)
#define MAC_DET C(KC_UP)
#define MAC_EMO C(G(KC_SPACE))
#define KC_SNAP G(S(KC_3))

#define BL_NEXT BLED_Mode
#define BL_VALU BLED_Brightness
#define BL_SPDU BLED_Speed
#define BL_HUEU BLED_Color

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [WIN_BASE] = LAYOUT_79_ansi(
        KC_ESC,  KC_F1,    KC_F2,    KC_F3,    KC_F4,    KC_F5,   KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,   KC_F12,             KC_MUTE,
        KC_GRV,  KC_1,     KC_2,     KC_3,     KC_4,     KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINS,  KC_EQL,   KC_BSPC,  KC_DEL,
        KC_TAB,  KC_Q,     KC_W,     KC_E,     KC_R,     KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_LBRC,  KC_RBRC,  KC_BSLS,  KC_PGUP,
        KC_CAPS, KC_A,     KC_S,     KC_D,     KC_F,     KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT,            KC_ENT,   KC_PGDN,
        KC_LSFT,           KC_Z,     KC_X,     KC_C,     KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH,  KC_RSFT,  KC_UP,
        KC_LCTL, KC_LWIN,  KC_LALT,                               KC_SPC,                             MO(1),   KC_RCTL,  KC_LEFT,  KC_DOWN,  KC_RGHT),

    [WIN_FN] = LAYOUT_79_ansi(
        EE_CLR,  KC_BRID,  KC_BRIU,  WIN_DET,  KC_TASK,  WIN_EMO, KC_PSCR, KC_MPRV, KC_MPLY, KC_MNXT, KC_MUTE, KC_VOLD,  KC_VOLU,            _______,
        _______, BT_HOST1, BT_HOST2, BT_HOST3, BT_2_4G,  BT_USB,  _______, _______, _______, _______, _______, _______,  _______,  BT_VOL,   _______,
        _______, _______,  _______,  _______,  _______,  _______, _______, _______, _______, _______, _______, _______,  RM_TOGG,  RM_NEXT,  _______,
        _______, PDF(2),   _______,  _______,  _______,  _______, _______, _______, _______, _______, _______, _______,            RM_HUEU,  _______,
        _______,           BL_VALU,  BL_SPDU,  BL_NEXT,  BL_HUEU, _______, _______, _______, _______, _______, _______,  _______,  RM_VALU,
        _______, GU_TOGG,  _______,                               _______,                            _______, _______,  RM_SPDD,  RM_VALD,  RM_SPDU),

    [MAC_BASE] = LAYOUT_79_ansi(
        KC_ESC,  KC_BRID,  KC_BRIU,  MAC_DET,  KC_TASK,  MAC_EMO, KC_SNAP, KC_MPRV, KC_MPLY, KC_MNXT, KC_MUTE, KC_VOLD,  KC_VOLU,            KC_MUTE,
        KC_GRV,  KC_1,     KC_2,     KC_3,     KC_4,     KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINS,  KC_EQL,   KC_BSPC,  KC_DEL,
        KC_TAB,  KC_Q,     KC_W,     KC_E,     KC_R,     KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_LBRC,  KC_RBRC,  KC_BSLS,  KC_PGUP,
        KC_CAPS, KC_A,     KC_S,     KC_D,     KC_F,     KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT,            KC_ENT,   KC_PGDN,
        KC_LSFT,           KC_Z,     KC_X,     KC_C,     KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH,  KC_RSFT,  KC_UP,
        KC_LCTL, KC_LOPT,  KC_LCMD,                               KC_SPC,                             MO(3),   KC_RCTL,  KC_LEFT,  KC_DOWN,  KC_RGHT),

    [MAC_FN] = LAYOUT_79_ansi(
        EE_CLR,  KC_F1,    KC_F2,    KC_F3,    KC_F4,    KC_F5,   KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,   KC_F12,             _______,
        _______, BT_HOST1, BT_HOST2, BT_HOST3, BT_2_4G,  BT_USB,  _______, _______, _______, _______, _______, _______,  _______,  BT_VOL,   _______,
        _______, _______,  _______,  _______,  _______,  _______, _______, _______, _______, _______, _______, _______,  RM_TOGG,  RM_NEXT,  _______,
        _______, _______,  PDF(0),   _______,  _______,  _______, _______, _______, _______, _______, _______, _______,            RM_HUEU,  _______,
        _______,           BL_VALU,  BL_SPDU,  BL_NEXT,  BL_HUEU, _______, _______, _______, _______, _______, _______,  _______,  RM_VALU,
        _______, _______,  _______,                               _______,                            _______, _______,  RM_SPDD,  RM_VALD,  RM_SPDU),
};

#if defined(ENCODER_MAP_ENABLE)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [MAC_BASE] = {ENCODER_CCW_CW(KC_VOLD, KC_VOLU) },
    [MAC_FN]   = {ENCODER_CCW_CW(_______, _______) },
    [WIN_BASE] = {ENCODER_CCW_CW(KC_VOLD, KC_VOLU) },
    [WIN_FN]   = {ENCODER_CCW_CW(_______, _______) }
};
#endif // ENCODER_MAP_ENABLE
// clang-format on

static uint8_t  all_blink_cnt  = 0;
static uint32_t all_blink_time = 0;

#define set_all_led_color_white()                   \
    do {                                            \
        for (uint8_t i = 0; i < 83; i++) {          \
            rgb_matrix_set_color(i, 120, 120, 120); \
        }                                           \
    } while (0)
#define set_all_led_off()                     \
    do {                                      \
        for (uint8_t i = 0; i < 83; i++) {    \
            rgb_matrix_set_color(i, 0, 0, 0); \
        }                                     \
    } while (0)

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (!bled_process_record_user(keycode, record)) {
        return false;
    }
#ifdef MULTIMODE_ENABLE
    if (!bt_process_record(keycode, record)) {
        return false;
    }
#endif

    switch (keycode) {
        case PDF(0):
        case PDF(2):
            if (record->event.pressed) {
                if (get_highest_layer(default_layer_state) == 0 || get_highest_layer(default_layer_state) == 2) {
                    keymap_config.no_gui = false;
                    eeconfig_update_keymap(&keymap_config);
                }
                all_blink_cnt  = 6;
                all_blink_time = timer_read32();
            }
            return true;

        case RM_VALU:
            if (record->event.pressed) {
                rgb_matrix_increase_val();
                if (rgb_matrix_get_val() >= RGB_MATRIX_MAXIMUM_BRIGHTNESS) {
                    all_blink_cnt  = 6;
                    all_blink_time = timer_read32();
                }
            }
            return false;
        case RM_VALD:
            if (record->event.pressed) {
                rgb_matrix_decrease_val();
                if (rgb_matrix_get_val() == 0) {
                    all_blink_cnt  = 6;
                    all_blink_time = timer_read32();
                }
            }
            return false;
        case RM_SPDU:
            if (record->event.pressed) {
                rgb_matrix_increase_speed();
                if (rgb_matrix_get_speed() >= UINT8_MAX) {
                    all_blink_cnt  = 6;
                    all_blink_time = timer_read32();
                }
            }
            return false;
        case RM_SPDD:
            if (record->event.pressed) {
                rgb_matrix_decrease_speed();
                if (rgb_matrix_get_speed() == 0) {
                    all_blink_cnt  = 6;
                    all_blink_time = timer_read32();
                }
            }
            return false;

        default:
            break;
    }

    return true;
}

void keyboard_pre_init_user(void) {
    bt_used_pin_init();
}

void keyboard_post_init_user(void) {
    // Customise these values to desired behaviour
    // debug_enable = true;
    // debug_matrix = true;
    bt_keyboard_post_init_user();
    bled_keyboard_post_init_user();

    if (keymap_config.no_gui) {
        keymap_config.no_gui = false; // Reset no_gui to false
        eeconfig_update_keymap(&keymap_config);
    }
}

void eeconfig_init_user(void) {
    bled_eeconfig_init_user();
}

#ifdef RGB_MATRIX_ENABLE
bool rgb_matrix_indicators_user(void) {
    if (!bts_info.bt_info.low_vol) {
        if (!bled_rgb_matrix_indicators_user()) {
            return false;
        }
    }

    // caps lock red
    if (host_keyboard_led_state().caps_lock
#    ifdef MULTIMODE_ENABLE
        && (dev_info.devs == DEVS_USB || bts_info.bt_info.paired)
#    endif
    ) {
        rgb_matrix_set_color(CAPS_LOCK_LED_INDEX, RGB_WHITE);
    }

    // GUI lock white
    if (keymap_config.no_gui
#    ifdef MULTIMODE_ENABLE
        && (dev_info.devs == DEVS_USB || bts_info.bt_info.paired)
#    endif
    ) {
        rgb_matrix_set_color(GUI_LOCK_LED_INDEX, RGB_WHITE);
    }

    // Bluetooth related indicators
#    ifdef MULTIMODE_ENABLE
    if (!bt_indicator_rgb()) {
        return false;
    }
#    endif

    // All LEDs blink
    if (all_blink_cnt && all_blink_time) {
        // Turn off all LEDs before blinking
        set_all_led_off();
        if (timer_elapsed32(all_blink_time) > 300) {
            all_blink_time = timer_read32();
            all_blink_cnt--;
            if (!all_blink_cnt) {
                all_blink_time = 0;
            }
        }
        if (all_blink_cnt & 0x1) {
            set_all_led_color_white();
        }
    }

    return true;
}
#endif

void housekeeping_task_user(void) {
    bt_housekeeping_task_user();
}
