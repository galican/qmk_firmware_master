/**
 * @file bt_task.h
 * @brief
 * @author Joy chang.li@westberrytech.com
 * @version 1.1.0
 * @date 2022-10-03
 *
 * @copyright Copyright (c) 2022 Westberry Technology (ChangZhou) Corp., Ltd
 */

#pragma once

#include "bled/bled.h"

enum multimode_keycodes {
    BT_HOST1 = BLED_KEYCODE_END,
    BT_HOST2,
    BT_HOST3,
    BT_2G4,
    BT_USB,
    BT_VOL,
    BT_KEYCODE_END,
};

#if defined(MM_BT_MODE_PIN) && defined(MM_2G4_MODE_PIN)
typedef enum {
    MM_MODE_USB = 0,
    MM_MODE_BT,
    MM_MODE_2G4,
} mm_mode_t;
#endif

extern mm_mode_t now_mode;

#ifdef RGB_MATRIX_BLINK_ENABLE
enum wl_indicator_status {
    wls_none = 0,
    wls_lback,
    wls_pair,
    wls_lback_succeed,
    wls_pair_succeed,
    wls_lback_timeout,
    wls_pair_timeout,
};
#endif

#ifdef MULTIMODE_ENABLE
typedef enum {
    BATTERY_STATE_UNPLUGGED = 0, // No cable connected
    BATTERY_STATE_CHARGING,      // Cable connected, charging
    BATTERY_STATE_CHARGED_FULL   // Cable connected, fully charged
} battery_charge_state_t;

battery_charge_state_t get_battery_charge_state(void);
#endif

bool    bt_process_record_user(uint16_t keycode, keyrecord_t *record);
void    bt_proc_long_press_keys(void);
void    open_rgb(void);
void    close_rgb(void);
bool    get_kb_sleep_flag(void);
bool    bt_rgb_matrix_indicators_user(void);
void    wl_rgb_indicator_set(uint8_t index, uint8_t status);
void    bt_housekeeping_task_user(void);
void    bt_keyboard_post_init_user(void);
uint8_t get_wl_rgb_indicator_s(void);
