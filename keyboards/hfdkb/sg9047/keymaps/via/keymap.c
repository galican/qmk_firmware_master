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
#include "bled/bled.h"

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
        _______, _______,  _______,  _______,  _______,  _______, _______, _______, _______, _______, _______, _______,  _______,  _______,  _______,
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
        _______, _______,  _______,  _______,  _______,  _______, _______, _______, _______, _______, _______, _______,  _______,  _______,  _______,
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
