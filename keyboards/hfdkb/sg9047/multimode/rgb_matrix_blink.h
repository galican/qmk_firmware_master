// Copyright 2023 JoyLee (@itarze)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "quantum.h"

typedef struct {
    uint16_t interval;
    uint16_t remain_time;
    uint8_t  times;
    uint32_t time;
    uint8_t  index;
    uint8_t  flip;
    void (*blink_cb)(uint8_t);
    RGB color;
} blink_rgb_t;

bool rgb_matrix_blink_set(uint8_t index);
bool rgb_matrix_blink_set_color(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
bool rgb_matrix_blink_set_cb(uint8_t index, void *blink_cb);
bool rgb_matrix_blink_set_interval(uint8_t index, uint8_t interval);
bool rgb_matrix_blink_set_times(uint8_t index, uint8_t times);
bool rgb_matrix_blink_set_remain_time(uint8_t index, uint8_t time);
bool rgb_matrix_blink_set_interval_times(uint8_t index, uint16_t interval, uint8_t times);
void rgb_matrix_blink_task(void);
