/**
 * @file multimode.h
 * @brief
 * @author Joy chang.li@westberrytech.com
 * @version 2.0.0
 * @date 2023-08-21
 *
 * @copyright Copyright (c) 2023 Westberry Technology (ChangZhou) Corp., Ltd
 */

#pragma once

#include "quantum.h"
#include "bts_lib.h"

typedef union {
    uint16_t raw;
    struct {
        uint8_t devs;
        uint8_t last_devs;
    };
} mm_eeconfig_t;

extern mm_eeconfig_t mm_eeconfig;
extern bts_info_t    bts_info;

void mm_init(void);
void mm_task(void);

void mm_init_kb(void);
void mm_init_user(void);

void mm_bts_task_kb(uint8_t devs);
void mm_bts_task_user(uint8_t devs);

void mm_mode_scan(void);
void mm_switch_mode(uint8_t last_mode, uint8_t now_mode, uint8_t reset);

bool process_record_multimode(uint16_t keycode, keyrecord_t *record);
bool process_record_multimode_kb(uint16_t keycode, keyrecord_t *record);
bool process_record_multimode_user(uint16_t keycode, keyrecord_t *record);

void eeconfig_update_multimode_default(void);
