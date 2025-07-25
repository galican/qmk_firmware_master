/* Copyright (C) 2025 jonylee@hfd
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
#include "keymap_japanese.h"

enum layers {
    WIN_BASE,
    _FN1,
    MAC_BASE,
    _FN2,
    _FN3,
    _FN4,
};
// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

[WIN_BASE] = LAYOUT_72_jis(
    KC_ESC,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINS, KC_EQL,  JP_YEN,  KC_BSPC, KC_DEL,
    KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_LBRC, KC_RBRC,                   KC_PGUP,
    KC_CAPS, KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT, KC_NUHS, KC_ENT,           KC_PGDN,
    KC_LSFT,          KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH, JP_BSLS, KC_RSFT, KC_UP,   KC_END,
    KC_LCTL, KC_LGUI, KC_LALT, JP_MHEN,                   KC_SPC,                    JP_HENK, JP_KANA, MO(_FN1),KC_RCTL, KC_LEFT, KC_DOWN, KC_RGHT),

[_FN1] = LAYOUT_72_jis(
    KC_GRV,  KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,  _______, _______, _______,
    _______, KC_MPRV, KC_MPLY, KC_MNXT, EE_CLR,  _______, KC_CALC, _______, KC_UP,   _______, KC_PSCR, KC_SCRL, KC_PAUS,                   _______,
    _______, RM_HUEU, KC_VOLU, KC_VOLD, KC_MUTE, _______, KC_HOME, KC_LEFT, KC_DOWN, KC_RGHT, KC_INS,  _______, _______, _______,          _______,
    _______,          KC_APP,  _______, _______, _______, _______, _______, RM_NEXT, PDF(2),  _______, _______, _______, _______, RM_VALU, _______,
    _______, GU_TOGG, _______, _______,                   _______,                   _______, MO(_FN3),_______, _______, RM_SPDD, RM_VALD, RM_SPDU),

[MAC_BASE] = LAYOUT_72_jis(
    KC_ESC,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINS, KC_EQL,  JP_YEN,  KC_BSPC, KC_DEL,
    KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_LBRC, KC_RBRC,                   KC_PGUP,
    KC_CAPS, KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT, KC_NUHS, KC_ENT,           KC_PGDN,
    KC_LSFT,          KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH, JP_BSLS, KC_RSFT, KC_UP,   KC_END,
    KC_LCTL, KC_LOPT, KC_LCMD, KC_LNG2,                   KC_SPC,                    KC_LNG1, KC_RCMD, MO(_FN2),KC_RCTL, KC_LEFT, KC_DOWN, KC_RGHT),

[_FN2] = LAYOUT_72_jis(
    KC_GRV,  KC_BRID, KC_BRIU, KC_MCTL, KC_LPAD, _______, _______, KC_MPRV, KC_MPLY, KC_MNXT, KC_MUTE, KC_VOLD, KC_VOLU, _______, _______, _______,
    _______, KC_MPRV, KC_MPLY, KC_MNXT, EE_CLR,  _______, KC_CALC, _______, KC_UP,   _______, KC_PSCR, KC_SCRL, KC_PAUS,                   _______,
    _______, RM_HUEU, KC_VOLU, KC_VOLD, KC_MUTE, _______, KC_HOME, KC_LEFT, KC_DOWN, KC_RGHT, KC_INS,  _______, _______, _______,          _______,
    _______,          KC_APP,  _______, _______, _______, _______, _______, RM_NEXT, PDF(0),  _______, _______, _______, _______, RM_VALU, _______,
    _______, _______, _______, _______,                   _______,                   MO(_FN3),_______, _______, _______, RM_SPDD, RM_VALD, RM_SPDU),

[_FN3] = LAYOUT_72_jis(
    _______, KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,  _______, _______, _______,
    _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,                   _______,
    _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,          _______,
    _______,          _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
    _______, _______, _______, _______,                   _______,                   _______, _______, _______, _______, _______, _______, _______),

[_FN4] = LAYOUT_72_jis(
    _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
    _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,                   _______,
    _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,          _______,
    _______,          _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
    _______, _______, _______, _______,                   _______,                   _______, _______, _______, _______, _______, _______, _______),
};
