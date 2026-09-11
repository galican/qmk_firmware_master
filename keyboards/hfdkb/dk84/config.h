/* Copyright (C) 2025 @ HFD (https://www.hfdic.com/)
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

#pragma once

/*
 * Feature disable options
 *  These options are also useful to firmware size reduction.
 */

#ifdef BT_MODE_ENABLE
#    define BT_CABLE_PIN B9
#    define BT_CHARGE_PIN B8
#    define BT_MODE_SW_PIN C0
#    define RF_MODE_SW_PIN B11
#    define RGB_DRIVER_SDB_PIN A15
#    define RGB_DRIVER_RESET_PIN C11

#    define USB_SUSPEND_STATE_CHECK

#    define BT1_LED_INDEX 17
#    define BT2_LED_INDEX 18
#    define BT3_LED_INDEX 19
#    define BT4_LED_INDEX 16
#    define USB_LED_INDEX 20

#    define BT1_LED_COLOR {0xFF / 3, 0xFF, 0xFF}
#    define BT2_LED_COLOR {0xFF / 3, 0xFF, 0xFF}
#    define BT3_LED_COLOR {0xFF / 3, 0xFF, 0xFF}
#    define BT4_LED_COLOR {0xFF / 3, 0xFF, 0xFF}
#    define USB_LED_COLOR {0xFF / 3, 0xFF, 0xFF}
#endif

/* SPI Config for spi flash*/
#define SPI_DRIVER SPIDQ
#define SPI_SCK_PIN B3
#define SPI_MOSI_PIN B5
#define SPI_MISO_PIN B4
#define SPI_MOSI_PAL_MODE 5
#define EXTERNAL_FLASH_SPI_SLAVE_SELECT_PIN C12

#define KEY_NUM 6

/* Enable CapsLock LED */
#define CAPS_LOCK_LED_INDEX 46
/* Enable GUI Lock LED */
#define GUI_LOCK_LED_INDEX 77

/* I2C Config for LED Driver */
// #define SNLED27351_DRIVER_COUNT 2
// #define SNLED27351_I2C_ADDRESS_1 SNLED27351_I2C_ADDRESS_GND
// #define SNLED27351_I2C_ADDRESS_2 SNLED27351_I2C_ADDRESS_VDDIO
#define IS31FL3733_I2C_ADDRESS_1 0b1110100 // IS31FL3733_I2C_ADDRESS_GND_GND
#define IS31FL3733_I2C_ADDRESS_2 0b1110111 // IS31FL3733_I2C_ADDRESS_GND_VCC

#define I2C1_OPMODE OPMODE_I2C
#define I2C1_CLOCK_SPEED 400000
// #define SNLED27351_PHASE_CHANNEL SNLED27351_SCAN_PHASE_9_CHANNEL

#define RGB_MATRIX_DEFAULT_MODE RGB_MATRIX_CUSTOM_EFFECT_CONST

/* Set LED driver current */
// #define SNLED27351_CURRENT_TUNE {0x45, 0x80, 0x80, 0x45, 0x80, 0x80, 0x45, 0x80, 0x80, 0x45, 0x80, 0x80}

#define BAT_VOL_LED_INDEX {17, 18, 19, 20, 21, 22, 23, 24, 25, 26}

/* Battery/charging indicators */
#define FLASK_LED_INDEX 15
#define CHARGE_LED_INDEX 65
// #define LOGO_LED_INDEX 81

#define LOW_BATTERY_COLOR 0xFF, 0x00, 0x00
#define CHARGE_INDICATOR_COLOR 0xFF / 3, 0xFF, 0xFF

#define LOW_BATTERY_BLINK_INTERVAL 500
#define LOW_BATTERY_NOTICE_TIME 10000
#define CHARGE_BLINK_INTERVAL 400
#define CHARGE_NOTICE_TIME 2400
