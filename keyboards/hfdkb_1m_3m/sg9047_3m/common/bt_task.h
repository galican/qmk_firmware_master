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

#include "bled.h"
#include "bts_lib.h"

// #define BT_DEBUG_MODE
#define ENTRY_STOP_TIMEOUT 100 // ms
// #define ENTRY_STOP_TIMEOUT (30 * 60000) // ms

enum multimode_keycodes {
    BT_HOST1 = BLED_KEYCODE_END,
    BT_HOST2,
    BT_HOST3,
    BT_2_4G,
    BT_USB,
    BT_VOL,
    RGB_TEST,
    BT_KEYCODE_END,
};

typedef union {
    uint32_t raw;
    struct {
        uint8_t devs;
        uint8_t last_devs;
    };
} dev_info_t;

extern dev_info_t dev_info;
extern bts_info_t bts_info;

#if defined(MM_BT_MODE_PIN) && defined(MM_2G4_MODE_PIN)
typedef enum {
    MM_MODE_USB = 0,
    MM_MODE_BT,
    MM_MODE_2G4,
} mm_mode_t;
#endif

/**
 * @brief bluetooth 初始化函数
 * @param None
 * @return None
 */
void bt_init(void);

/**
 * @brief bluetooth交互任务
 * @param None
 * @return None
 */
void bt_task(void);
void bt_keyboard_post_init_user(void);
void bt_used_pin_init(void);

/**
 * @brief 处理和BT相关的按键
 * @param keycode: 键值
 * @param record: 记录值
 * @return None
 */
bool bt_process_record(uint16_t keycode, keyrecord_t *record);

/**
 * @brief rgb指示灯任务
 * @param None
 * @return None
 */
bool bt_indicator_rgb(void);

/**
 * @brief 切换工作模式
 * @param None
 * @return None
 */
void bt_switch_mode(uint8_t last_mode, uint8_t now_mode, uint8_t reset);

/**
 * @brief usb suspend task
 * @param none
 * @param none
 * @return None
 */
void bt_housekeeping_task_user(void);
