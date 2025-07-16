// Copyright 2023 JoyLee (@itarze)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

/*
 * Feature disable options
 *  These options are also useful to firmware size reduction.
 */

#ifdef MULTIMODE_ENABLE
#    define CLOSE_RGB_ENABLE

#    define MM_BT_HOST1_NAME "R85Ultra-1"
#    define MM_BT_HOST2_NAME "R85Ultra-2"
#    define MM_BT_HOST3_NAME "R85Ultra-3"

#    define WL_LBACK_TIMEOUT (10 * 1000)
#    define WL_LBACK_BLINK_INTERVAL (500)
#    define WL_PAIR_TIMEOUT (30 * 1000)
#    define WL_PAIR_BLINK_INTERVAL (200)
#    define WL_SUCCEED_TIME (2 * 1000)
#    define WL_SLEEP_TIMEOUT (5 * 60000)

#    define USB_CONN_BLINK_INTERVAL (500)

#    define MATRIX_LONG_PRESS
#    define RGB_MATRIX_BLINK_COUNT 6

// Multi mode used pins
#    define MM_CABLE_PIN B9
#    define MM_CHARGE_PIN B8

#    define MM_BT_MODE_PIN C0
#    define MM_2G4_MODE_PIN B11

#    define MM_USB_EN_PIN A14
#    define MM_USB_EN_STATE 0

// Indicate index of mm device
#    define RGB_MATRIX_BLINK_INDEX_HOST1 26
#    define RGB_MATRIX_BLINK_INDEX_HOST2 25
#    define RGB_MATRIX_BLINK_INDEX_HOST3 24
#    define RGB_MATRIX_BLINK_INDEX_2G4 23
#    define RGB_MATRIX_BLINK_INDEX_USB 22
#    define RGB_MATRIX_BLINK_INDEX_ALL 0xFF

// Indicate color of mm device
#    define RGB_MATRIX_BLINK_HOST1_COLOR RGB_BLUE   // 蓝牙1指示灯颜色
#    define RGB_MATRIX_BLINK_HOST2_COLOR RGB_CYAN   // 蓝牙2指示灯颜色
#    define RGB_MATRIX_BLINK_HOST3_COLOR RGB_PURPLE // 蓝牙3指示灯颜色
#    define RGB_MATRIX_BLINK_2G4_COLOR RGB_GREEN    // 2.4G指示灯颜色
#    define RGB_MATRIX_BLINK_USB_COLOR RGB_WHITE    // USB指示灯颜色
#endif

/* SPI Config for spi flash*/
#define SPI_DRIVER SPIDQ
#define SPI_SCK_PIN B3
#define SPI_MOSI_PIN B5
#define SPI_MISO_PIN B4
#define SPI_MOSI_PAL_MODE 5

#define EXTERNAL_FLASH_SPI_SLAVE_SELECT_PIN C12

// USB Config
#define USB_SUSPEND_CHECK_ENABLE

// LED indicator config
#define CAPS_LOCK_LED_INDEX 56
#define GUI_LOCK_LED_INDEX 81

// RGB Matrix Config
#define RGB_MATRIX_SHUTDOWN_PIN D2

// Encoder Config
#define ENCODER_DEFAULT_POS 0x0
