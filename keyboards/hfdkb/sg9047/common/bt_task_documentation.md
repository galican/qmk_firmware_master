# BT Task Module Documentation

## Overview
This module (`bt_task.c`) handles Bluetooth functionality, device switching, RGB control, battery management, and factory reset operations for the HFDKB keyboard firmware.

**File:** `bt_task.c`  
**Author:** JoyLee  
**Version:** 2.0.0  
**Date:** 2023-04-06  
**Copyright:** Westberry Technology Corp., Ltd

---

## Table of Contents
1. [Includes and Dependencies](#includes-and-dependencies)
2. [Constants and Macros](#constants-and-macros)
3. [Enumerations](#enumerations)
4. [Data Structures](#data-structures)
5. [Global Variables](#global-variables)
6. [Function Reference](#function-reference)
7. [Static Variables](#static-variables)
8. [Thread Definitions](#thread-definitions)

---

## Includes and Dependencies

```c
#include QMK_KEYBOARD_H
#include "common/bts_lib.h"
#include "config.h"
#include "gpio.h"
#include "quantum.h"
#include "rgb_matrix.h"
#include "uart.h"
#include "usb_main.h"
#include "common/bt_task.h"
```

---

## Constants and Macros

| Macro | Value | Description |
|-------|-------|-------------|
| `NUM_LONG_PRESS_KEYS` | `sizeof(long_pressed_keys) / sizeof(long_pressed_keys_t)` | Number of long press keys |
| `LED_OFF_STANDBY_MS` | `10 * 60 * 1000` | LED standby timeout (10 minutes) |
| `KEY_NUM` | `8` | Number of keys |
| `BT_DEBUG_INFO(fmt, ...)` | Debug macro (conditional) | Debug information output |

---

## Enumerations

### `reset_type`
Factory reset types:
- `RESET_NONE = 0` - No reset
- `FACTORY` - Factory reset
- `KEYBOARD` - Keyboard reset
- `BLE` - Bluetooth reset

### `battery_charge_state_t`
Battery charging states:
- `BATTERY_STATE_UNPLUGGED = 0` - No cable connected
- `BATTERY_STATE_CHARGING` - Cable connected, charging
- `BATTERY_STATE_CHARGED_FULL` - Cable connected, fully charged

---

## Data Structures

### `factory_reset_t`
Factory reset state management:
```c
typedef struct {
    uint8_t  type;       // 1=factory, 2=keyboard, 3=ble, 0=none
    uint8_t  progress;   // 0-7 steps
    uint32_t start_time;
} factory_reset_t;
```

### `low_battery_warning_t`
Low battery warning state (20% and below, red blink 6 times):
```c
typedef struct {
    bool     triggered;
    uint8_t  blink_count;
    uint32_t blink_time;
    bool     blink_state;
    bool     completed;
} low_battery_warning_t;
```

### `charge_complete_warning_t`
Charge complete indication state (green blink 5 times):
```c
typedef struct {
    bool     triggered;
    uint8_t  blink_count;
    uint32_t blink_time;
    uint32_t entry_full_time;
    bool     blink_state;
    bool     completed;
} charge_complete_warning_t;
```

### `low_voltage_state_t`
Low voltage backlight control state (5% and below turns off backlight):
```c
typedef struct {
    bool     forced_rgb_off;
    bool     is_low_voltage;
    uint8_t  saved_rgb_mode;
    uint8_t  read_vol_count;
    uint32_t last_normal_vol_time;
    bool     waiting_recovery;
} low_voltage_state_t;
```

### `long_pressed_keys_t`
Long press key configuration:
```c
typedef struct {
    uint32_t press_time;
    uint16_t keycode;
    void (*event_cb)(uint16_t);
} long_pressed_keys_t;
```

---

## Global Variables

### Core Bluetooth Variables
| Variable | Type | Description |
|----------|------|-------------|
| `per_info` | `per_info_t` | Persistent information storage |
| `dev_info` | `dev_info_t` | Device information |
| `bts_info` | `bts_info_t` | Bluetooth service information |
| `bt_init_time` | `uint32_t` | Bluetooth initialization timestamp |

### Status Variables
| Variable | Type | Description |
|----------|------|-------------|
| `indicator_status` | `uint8_t` | LED indicator status |
| `indicator_reset_last_time` | `uint8_t` | Indicator reset flag |
| `layer_save` | `uint8_t` | Saved layer information |
| `kb_sleep_flag` | `bool` | Keyboard sleep flag |
| `low_vol_offed_sleep` | `bool` | Low voltage sleep flag |
| `query_vol_flag` | `bool` | Battery query flag |
| `last_total_time` | `uint32_t` | Last total time |

### USB Related Variables
| Variable | Type | Description |
|----------|------|-------------|
| `USB_switch_time` | `uint32_t` | USB switch timestamp |
| `USB_blink_cnt` | `uint8_t` | USB blink counter |

### Long Press Configuration
| Variable | Type | Description |
|----------|------|-------------|
| `long_pressed_keys[]` | `long_pressed_keys_t` | Long press key configurations |

### Sleep Time Configuration
| Variable | Type | Description |
|----------|------|-------------|
| `sleep_time_table[4]` | `const uint32_t` | Sleep timeout table: {0, 10min, 30min, 60min} |

---

## Function Reference

### Public Functions

#### Core Functions
| Function | Return Type | Parameters | Description |
|----------|-------------|------------|-------------|
| `bt_init()` | `void` | None | Initialize Bluetooth module |
| `bt_task()` | `void` | None | Main Bluetooth task loop |
| `process_record_bt()` | `bool` | `uint16_t keycode, keyrecord_t *record` | Process Bluetooth-related key events |
| `bt_switch_mode()` | `void` | `uint8_t last_mode, uint8_t now_mode, uint8_t reset` | Switch between device modes |
| `bt_indicator_rgb()` | `bool` | `uint8_t led_min, uint8_t led_max` | Handle RGB indicators |
| `register_code()` | `void` | `uint8_t code` | Register key code (weak function) |
| `unregister_code()` | `void` | `uint8_t code` | Unregister key code (weak function) |
| `register_mouse()` | `void` | `uint8_t mouse_keycode, bool pressed` | Register mouse events (external) |

### Static Functions

#### Hardware Management
| Function | Return Type | Parameters | Description |
|----------|-------------|------------|-------------|
| `bt_used_pin_init()` | `static void` | None | Initialize Bluetooth-related pins |
| `bt_scan_mode()` | `static void` | None | Scan and handle hardware switch mode |

#### Key Processing
| Function | Return Type | Parameters | Description |
|----------|-------------|------------|-------------|
| `long_pressed_keys_hook()` | `static void` | None | Handle long press key detection |
| `long_pressed_keys_cb()` | `static void` | `uint16_t keycode` | Long press key callback |
| `process_record_other()` | `static bool` | `uint16_t keycode, keyrecord_t *record` | Process other key events |

#### Helper Functions
| Function | Return Type | Parameters | Description |
|----------|-------------|------------|-------------|
| `is_switch_forcing_wired_mode()` | `static bool` | None | Check if hardware switch forces wired mode |
| `is_wireless_mode_keycode()` | `static bool` | `uint16_t keycode` | Check if keycode is for wireless mode |
| `is_current_mode_wireless()` | `static bool` | None | Check if current mode is wireless |
| `is_current_mode_wireless_by_devs()` | `static bool` | `uint8_t devs` | Check if given device mode is wireless |

#### Power Management
| Function | Return Type | Parameters | Description |
|----------|-------------|------------|-------------|
| `update_low_voltage_state()` | `static void` | None | Update low voltage state |
| `get_battery_charge_state()` | `static battery_charge_state_t` | None | Get current battery charge state |
| `handle_battery_query()` | `static void` | None | Handle periodic battery queries |
| `handle_battery_query_display()` | `static void` | None | Display battery query results |

#### RGB Control
| Function | Return Type | Parameters | Description |
|----------|-------------|------------|-------------|
| `open_rgb()` | `static void` | None | Enable RGB lighting |
| `close_rgb()` | `static void` | None | Disable RGB lighting with power management |

#### LED Indication
| Function | Return Type | Parameters | Description |
|----------|-------------|------------|-------------|
| `handle_bt_indicate_led()` | `static void` | None | Handle Bluetooth indicator LED |
| `handle_usb_indicate_led()` | `static void` | None | Handle USB indicator LED |
| `handle_blink_effects()` | `static void` | None | Handle LED blink effects |
| `handle_layer_indication()` | `static void` | None | Handle layer indication |
| `handle_charging_indication()` | `static void` | None | Handle charging indication |
| `handle_low_battery_warning()` | `static void` | None | Handle low battery warning |
| `handle_low_battery_shutdow()` | `static void` | None | Handle low battery shutdown |

#### Factory Reset
| Function | Return Type | Parameters | Description |
|----------|-------------|------------|-------------|
| `execute_factory_reset()` | `static void` | None | Execute factory reset operation |
| `handle_factory_reset_display()` | `static void` | None | Display factory reset progress |

---

## Static Variables

### State Management
| Variable | Type | Description |
|----------|------|-------------|
| `factory_reset` | `static factory_reset_t` | Factory reset state |
| `low_battery_warning` | `static low_battery_warning_t` | Low battery warning state |
| `charge_complete_warning` | `static charge_complete_warning_t` | Charge complete warning state |
| `low_voltage_state` | `static low_voltage_state_t` | Low voltage state |

### Status Flags
| Variable | Type | Description |
|----------|------|-------------|
| `is_in_low_power_state` | `static bool` | Low power state flag |
| `is_in_full_power_state` | `static bool` | Full power state flag |
| `is_charging` | `static bool` | Charging state flag |

### RGB Control
| Variable | Type | Description |
|----------|------|-------------|
| `key_press_time` | `static uint32_t` | Last key press timestamp |
| `close_rgb_time` | `static uint32_t` | RGB close timestamp |
| `bak_rgb_toggle` | `static bool` | Backup RGB toggle state |
| `sober` | `static bool` | RGB active state |
| `rgb_status_save` | `static bool` | RGB status backup |

### RGB Configuration Tables
| Variable | Type | Description |
|----------|------|-------------|
| `rgb_index_table[]` | `static const uint8_t` | Device RGB index mapping |
| `rgb_index_color_table[][3]` | `static const uint8_t` | Device RGB color mapping |
| `rgb_test_color_table[][3]` | `static const uint8_t` | RGB test color table |
| `rgb_test_index` | `static uint8_t` | RGB test index |
| `rgb_test_en` | `static bool` | RGB test enable flag |
| `rgb_test_time` | `static uint32_t` | RGB test timestamp |

### Blink Effects
| Variable | Type | Description |
|----------|------|-------------|
| `all_blink_cnt` | `static uint8_t` | All keys blink counter |
| `all_blink_time` | `static uint32_t` | All keys blink timestamp |
| `all_blink_color` | `static RGB` | All keys blink color |
| `single_blink_cnt` | `static uint8_t` | Single key blink counter |
| `single_blink_index` | `static uint8_t` | Single key blink index |
| `single_blink_color` | `static RGB` | Single key blink color |
| `single_blink_time` | `static uint32_t` | Single key blink timestamp |

---

## Thread Definitions

### `Thread1`
- **Working Area:** `waThread1` (128 bytes)
- **Priority:** `HIGHPRIO`
- **Function:** Continuous Bluetooth task execution
- **Description:** Background thread that runs `bts_task()` every 1ms

---

## Key Features

### Device Mode Management
- USB mode
- Bluetooth Host 1, 2, 3
- 2.4G wireless mode
- Hardware switch override

### Power Management
- Low battery warning (≤20%)
- Critical battery shutdown (≤5%)
- Charging indication
- RGB power management

### LED Indicators
- Device status indication
- Battery level display
- Charging status
- Layer indication
- Factory reset progress

### Factory Reset Support
- Factory reset (all settings)
- Keyboard reset (keymap only)
- BLE reset (Bluetooth only)
- Visual progress indication

### RGB Control
- Automatic sleep mode
- Power-saving features
- Test mode
- Blink effects

---

## Usage Notes

1. **Initialization:** Call `bt_init()` during keyboard initialization
2. **Task Loop:** Call `bt_task()` regularly in main loop
3. **Key Processing:** Use `process_record_bt()` for key event handling
4. **RGB Indicators:** Call `bt_indicator_rgb()` from RGB matrix callback

This module provides comprehensive Bluetooth and power management functionality for wireless keyboards with multiple connection modes and advanced RGB indication features.
