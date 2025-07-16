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

#include "quantum.h"

bool bt_process_record_user(uint16_t keycode, keyrecord_t *record);
void bt_proc_long_press_keys(void);
void open_rgb(void);
void close_rgb(void);
bool bt_rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max);
