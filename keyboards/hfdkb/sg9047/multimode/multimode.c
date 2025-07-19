/**
 * @file multimode.c
 * @brief
 * @author Joy chang.li@westberrytech.com
 * @version 2.0.0
 * @date 2023-08-21
 *
 * @copyright Copyright (c) 2023 Westberry Technology (ChangZhou) Corp., Ltd
 */

#include QMK_KEYBOARD_H

#include "quantum.h"
#include "uart.h"
#include "report.h"
#include "usb_main.h"
#include "eeprom.h"
#include "multimode.h"
#include "bt_task.h"

#ifndef MM_EXE_TIME
#    define MM_EXE_TIME 50 // us
#endif

#ifndef MM_READY_TIME
#    define MM_READY_TIME 2000 // ms
#endif

#ifndef MM_BT_SLEEP_EN
#    define MM_BT_SLEEP_EN true
#endif

#ifndef MM_2G4_SLEEP_EN
#    define MM_2G4_SLEEP_EN true
#endif

#ifndef MM_BT_HOST1_NAME
#    define MM_BT_HOST1_NAME "WB KB 1"
#endif

#ifndef MM_BT_HOST2_NAME
#    define MM_BT_HOST2_NAME "WB KB 2"
#endif

#ifndef MM_BT_HOST3_NAME
#    define MM_BT_HOST3_NAME "WB KB 3"
#endif

#ifndef MM_BT_HOST4_NAME
#    define MM_BT_HOST4_NAME "WB KB 4"
#endif

#ifndef MM_BT_HOST5_NAME
#    define MM_BT_HOST5_NAME "WB KB 5"
#endif

#ifndef MM_USB_EN_STATE
#    define MM_USB_EN_STATE 1
#endif

#ifdef MM_DEBUG_ENABLE
#    define MM_DEBUG_INFO(fmt, ...) dprintf(fmt, ##__VA_ARGS__)
#else
#    define MM_DEBUG_INFO(fmt, ...)
#endif

// globals
mm_eeconfig_t mm_eeconfig = {0};
bts_info_t    bts_info    = {
          .bt_name        = {MM_BT_HOST1_NAME, MM_BT_HOST2_NAME, MM_BT_HOST3_NAME, MM_BT_HOST4_NAME, MM_BT_HOST5_NAME},
          .uart_init      = uart_init,
          .uart_read      = uart_read,
          .uart_transmit  = uart_transmit,
          .uart_receive   = uart_receive,
          .uart_available = uart_available,
          .timer_read32   = timer_read32,
};

extern host_driver_t chibios_driver;

mm_mode_t now_mode = MM_MODE_USB;

// local
static uint32_t bt_init_time = 0;

static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION(Thread1, arg) {
    (void)arg;
    chRegSetThreadName("multimode");
    while (true) {
        bts_task(mm_eeconfig.devs);
        mm_bts_task_kb(mm_eeconfig.devs);
        chThdSleepMicroseconds(MM_EXE_TIME);
    }
}

// user-defined overridable functions
__attribute__((weak)) void mm_init_user(void) {}

__attribute__((weak)) void mm_init_kb(void) {
    mm_init_user();
}

__attribute__((weak)) void mm_bts_task_user(uint8_t devs) {}

__attribute__((weak)) void mm_bts_task_kb(uint8_t devs) {
    mm_bts_task_user(devs);
}

static void mm_used_pin_init(void);

void eeconfig_update_multimode_default(void) {
    MM_DEBUG_INFO("eeconfig_update_multimode_default\n");
    mm_eeconfig.devs      = DEVS_USB;
    mm_eeconfig.last_devs = DEVS_HOST1;
    bt_init_time          = timer_read32();
    eeconfig_update_kb(mm_eeconfig.raw);
}

void eeconfig_debug_multimode(void) {
    MM_DEBUG_INFO("eeconfig_debug_multimode EEPROM\n");
    MM_DEBUG_INFO("mm_eeconfig.devs = %d\n", mm_eeconfig.devs);
    MM_DEBUG_INFO("mm_eeconfig.last_devs = %d\n", mm_eeconfig.last_devs);
}

static void mm_used_pin_init(void) {
#if defined(MM_BT_MODE_PIN) && defined(MM_2G4_MODE_PIN)
    setPinInputHigh(MM_BT_MODE_PIN);
    setPinInputHigh(MM_2G4_MODE_PIN);
#endif
#if defined(MM_CABLE_PIN) && defined(MM_CHARGE_PIN)
    setPinInputHigh(MM_CABLE_PIN);
    setPinInput(MM_CHARGE_PIN);
#endif
#ifdef MM_USB_EN_PIN
    setPinOutput(MM_USB_EN_PIN);
    writePin(MM_USB_EN_PIN, MM_USB_EN_STATE);
#endif
}

void mm_init(void) {
    bts_init(&bts_info);

    mm_used_pin_init();

    mm_eeconfig.raw = eeconfig_read_kb();
    if (!mm_eeconfig.raw) {
        eeconfig_update_multimode_default();
    }

    eeconfig_debug_multimode(); // display current eeprom values

    bt_init_time = timer_read32();

    chThdCreateStatic(waThread1, sizeof(waThread1), HIGHPRIO, Thread1, NULL);

    mm_init_kb();

    mm_mode_scan();
}

void mm_task(void) {
    static uint32_t query_vol_interval_time = 0;
    static uint32_t led_interval_time       = 0;

    if ((bt_init_time != 0) && (timer_elapsed32(bt_init_time) >= MM_READY_TIME)) {
        bt_init_time = 0;

        if (MM_BT_SLEEP_EN) {
            bts_send_vendor(v_en_sleep_bt);
        } else {
            bts_send_vendor(v_dis_sleep_bt);
        }

        if (MM_2G4_SLEEP_EN) {
            bts_send_vendor(v_en_sleep_wl);
        } else {
            bts_send_vendor(v_dis_sleep_wl);
        }
    }

    static bool is_inited = false;
    if (!is_inited) {
        is_inited = true;
        mm_switch_mode(!mm_eeconfig.devs, mm_eeconfig.devs, false);
    }

    // Set led indicators
    if (timer_elapsed32(led_interval_time) >= 1) {
        led_interval_time = timer_read32();
        if (mm_eeconfig.devs != DEVS_USB) {
            uint8_t keyboard_led_state = 0;
            led_t  *kb_leds            = (led_t *)&keyboard_led_state;
            kb_leds->raw               = bts_info.bt_info.indictor_rgb_s;
            usb_device_state_set_leds(keyboard_led_state);
        }
    }

    if (!bt_init_time && (timer_elapsed32(query_vol_interval_time) >= 4000)) {
        query_vol_interval_time = timer_read32();
        if (!get_kb_sleep_flag()) {
            switch (get_battery_charge_state()) {
                case BATTERY_STATE_CHARGING:
                    bts_send_vendor(v_query_vol_chrg);
                    break;
                case BATTERY_STATE_CHARGED_FULL:
                    bts_send_vendor(v_query_vol_full);
                    break;
                case BATTERY_STATE_UNPLUGGED:
                    bts_send_vendor(v_query_vol);
                    break;
                default:
                    break;
            }
        }
    }

    // mode scan
    if (!bt_init_time) mm_mode_scan();
}

__attribute__((weak)) void mm_switch_mode(uint8_t last_mode, uint8_t now_mode, uint8_t reset) {
    bool usb_sws = !!last_mode ? !now_mode : !!now_mode;

    if (usb_sws) {
        if (!!now_mode) {
            usbDisconnectBus(&USB_DRIVER);
            usbStop(&USB_DRIVER);
        } else {
            init_usb_driver(&USB_DRIVER);
        }
    }

    mm_eeconfig.devs = now_mode;
    if (mm_eeconfig.devs != DEVS_USB && mm_eeconfig.devs != DEVS_2G4) {
        mm_eeconfig.last_devs = mm_eeconfig.devs;
    }

#ifdef MM_USB_EN_PIN
    if (mm_eeconfig.devs == DEVS_USB) {
        writePin(MM_USB_EN_PIN, MM_USB_EN_STATE);
    } else {
        writePin(MM_USB_EN_PIN, !MM_USB_EN_STATE);
    }
#endif

    bts_info.bt_info.pairing       = false;
    bts_info.bt_info.paired        = false;
    bts_info.bt_info.come_back     = false;
    bts_info.bt_info.come_back_err = false;
    bts_info.bt_info.mode_switched = false;
    eeconfig_update_kb(mm_eeconfig.raw);

    switch (mm_eeconfig.devs) {
        case DEVS_HOST1: {
            if (reset != false) {
                bts_send_vendor(v_host1);
                bts_send_name(DEVS_HOST1);
                bts_send_vendor(v_pair);
                wl_rgb_indicator_set(RGB_MATRIX_BLINK_INDEX_HOST1, wls_pair);
            } else if (last_mode != DEVS_HOST1) {
                bts_send_vendor(v_host1);
                wl_rgb_indicator_set(RGB_MATRIX_BLINK_INDEX_HOST1, wls_lback);
            }
        } break;
        case DEVS_HOST2: {
            if (reset != false) {
                bts_send_vendor(v_host2);
                bts_send_name(DEVS_HOST2);
                bts_send_vendor(v_pair);
                wl_rgb_indicator_set(RGB_MATRIX_BLINK_INDEX_HOST2, wls_pair);
            } else if (last_mode != DEVS_HOST2) {
                bts_send_vendor(v_host2);
                wl_rgb_indicator_set(RGB_MATRIX_BLINK_INDEX_HOST2, wls_lback);
            }
        } break;
        case DEVS_HOST3: {
            if (reset != false) {
                bts_send_vendor(v_host3);
                bts_send_name(DEVS_HOST3);
                bts_send_vendor(v_pair);
                wl_rgb_indicator_set(RGB_MATRIX_BLINK_INDEX_HOST3, wls_pair);
            } else if (last_mode != DEVS_HOST3) {
                bts_send_vendor(v_host3);
                wl_rgb_indicator_set(RGB_MATRIX_BLINK_INDEX_HOST3, wls_lback);
            }
        } break;
        case DEVS_HOST4: {
            if (reset != false) {
                bts_send_vendor(v_host4);
                bts_send_name(DEVS_HOST4);
                bts_send_vendor(v_pair);
#ifdef RGB_MATRIX_BLINK_INDEX_HOST4
                wl_rgb_indicator_set(RGB_MATRIX_BLINK_INDEX_HOST4, wls_pair);
#endif
            } else if (last_mode != DEVS_HOST4) {
                bts_send_vendor(v_host4);
#ifdef RGB_MATRIX_BLINK_INDEX_HOST4
                wl_rgb_indicator_set(RGB_MATRIX_BLINK_INDEX_HOST4, wls_lback);
#endif
            }
        } break;
        case DEVS_HOST5: {
            if (reset != false) {
                bts_send_vendor(v_host5);
                bts_send_name(DEVS_HOST5);
                bts_send_vendor(v_pair);
#ifdef RGB_MATRIX_BLINK_INDEX_HOST5
                wl_rgb_indicator_set(RGB_MATRIX_BLINK_INDEX_HOST5, wls_pair);
#endif
            } else if (last_mode != DEVS_HOST5) {
                bts_send_vendor(v_host5);
#ifdef RGB_MATRIX_BLINK_INDEX_HOST5
                wl_rgb_indicator_set(RGB_MATRIX_BLINK_INDEX_HOST5, wls_lback);
#endif
            }
        } break;
        case DEVS_2G4: {
            if (reset != false) {
                bts_send_vendor(v_2g4);
                bts_send_vendor(v_pair);
                wl_rgb_indicator_set(RGB_MATRIX_BLINK_INDEX_2G4, wls_pair);
            } else if (last_mode != DEVS_2G4) {
                bts_send_vendor(v_2g4);
                wl_rgb_indicator_set(RGB_MATRIX_BLINK_INDEX_2G4, wls_lback);
            }
        } break;
        case DEVS_USB: {
            bts_send_vendor(v_usb);
#ifdef RGB_MATRIX_BLINK_INDEX_USB
            wl_rgb_indicator_set(RGB_MATRIX_BLINK_INDEX_USB, wls_lback);
#endif
        } break;
        default: {
            bts_send_vendor(v_usb);
            eeconfig_update_multimode_default();
#ifdef RGB_MATRIX_BLINK_INDEX_USB
            wl_rgb_indicator_set(RGB_MATRIX_BLINK_INDEX_USB, wls_lback);
#endif
        } break;
    }
}

__attribute__((weak)) void mm_mode_scan(void) {
#if defined(MM_BT_MODE_PIN) && defined(MM_2G4_MODE_PIN)
    if (!readPin(MM_BT_MODE_PIN) && readPin(MM_2G4_MODE_PIN)) {
        now_mode = MM_MODE_BT; // 蓝牙模式
    } else if (!readPin(MM_2G4_MODE_PIN) && readPin(MM_BT_MODE_PIN)) {
        now_mode = MM_MODE_2G4; // 2.4G模式
    } else {
        now_mode = MM_MODE_USB; // USB模式
    }

    switch (now_mode) {
        case MM_MODE_USB:
            if (mm_eeconfig.devs != DEVS_USB) {
                mm_switch_mode(mm_eeconfig.devs, DEVS_USB, false); // usb mode
            }
            break;
        case MM_MODE_2G4:
            if (mm_eeconfig.devs != DEVS_2G4) {
                mm_switch_mode(mm_eeconfig.devs, DEVS_2G4, false); // 2.4G mode
            }
            break;
        case MM_MODE_BT:
            if ((mm_eeconfig.devs == DEVS_USB) || (mm_eeconfig.devs == DEVS_2G4)) {
                mm_switch_mode(mm_eeconfig.devs, mm_eeconfig.last_devs, false); // ble mode
            }
            break;
        default:
            break;
    }

#endif
#ifdef MM_MODE_SW_PIN // 用来检测无线和有线模式的PIN

#    ifndef MM_USB_MODE_STATE
#        define MM_USB_MODE_STATE 0 // 默认低电平为USB状态(有线状态)
#    endif

    uint8_t now_mode = false;
    uint8_t usb_sws  = false;

    now_mode = !!MM_USB_MODE_STATE ? !readPin(MM_MODE_SW_PIN) : readPin(MM_MODE_SW_PIN);
    usb_sws  = !!mm_eeconfig.devs ? !now_mode : now_mode;

    if (usb_sws) {
        if (now_mode) {
            mm_switch_mode(mm_eeconfig.devs, mm_eeconfig.last_devs, false); // ble mode
        } else {
            mm_switch_mode(mm_eeconfig.devs, DEVS_USB, false); // usb mode
        }
    }
#endif
}

// user-defined overridable functions
__attribute__((weak)) bool process_record_multimode_user(uint16_t keycode, keyrecord_t *record) {
    return true;
}

__attribute__((weak)) bool process_record_multimode_kb(uint16_t keycode, keyrecord_t *record) {
    return process_record_multimode_user(keycode, record);
}

bool process_record_multimode(uint16_t keycode, keyrecord_t *record) {
    if (!process_record_multimode_kb(keycode, record)) {
        return false;
    }

    if (mm_eeconfig.devs != DEVS_USB) {
        if ((keycode > QK_MODS) && (keycode <= QK_MODS_MAX)) {
            if (QK_MODS_GET_MODS(keycode) & 0x1) {
                if (QK_MODS_GET_MODS(keycode) & 0x10)
                    bts_process_keys(KC_RCTL, record->event.pressed, mm_eeconfig.devs, keymap_config.no_gui, 8);
                else
                    bts_process_keys(KC_LCTL, record->event.pressed, mm_eeconfig.devs, keymap_config.no_gui, 8);
            }
            if (QK_MODS_GET_MODS(keycode) & 0x2) {
                if (QK_MODS_GET_MODS(keycode) & 0x10)
                    bts_process_keys(KC_RSFT, record->event.pressed, mm_eeconfig.devs, keymap_config.no_gui, 8);
                else
                    bts_process_keys(KC_LSFT, record->event.pressed, mm_eeconfig.devs, keymap_config.no_gui, 8);
            }
            if (QK_MODS_GET_MODS(keycode) & 0x4) {
                if (QK_MODS_GET_MODS(keycode) & 0x10)
                    bts_process_keys(KC_RALT, record->event.pressed, mm_eeconfig.devs, keymap_config.no_gui, 8);
                else
                    bts_process_keys(KC_LALT, record->event.pressed, mm_eeconfig.devs, keymap_config.no_gui, 8);
            }
            if (QK_MODS_GET_MODS(keycode) & 0x8) {
                if (QK_MODS_GET_MODS(keycode) & 0x10)
                    bts_process_keys(KC_RGUI, record->event.pressed, mm_eeconfig.devs, keymap_config.no_gui, 8);
                else
                    bts_process_keys(KC_LGUI, record->event.pressed, mm_eeconfig.devs, keymap_config.no_gui, 8);
            }
            return bts_process_keys(QK_MODS_GET_BASIC_KEYCODE(keycode), record->event.pressed, mm_eeconfig.devs, keymap_config.no_gui, 8);
        } else {
            return bts_process_keys(keycode, record->event.pressed, mm_eeconfig.devs, keymap_config.no_gui, 8);
        }
    }

    return true;
}

#include "command.h"
#include "action.h"

extern void register_mouse(uint8_t mouse_keycode, bool pressed);

__attribute__((weak)) void register_code(uint8_t code) {
    if (mm_eeconfig.devs) {
        bts_process_keys(code, true, mm_eeconfig.devs, keymap_config.no_gui, 8);
    } else {
        if (code == KC_NO) {
            return;

#ifdef LOCKING_SUPPORT_ENABLE
        } else if (KC_LOCKING_CAPS_LOCK == code) {
#    ifdef LOCKING_RESYNC_ENABLE
            // Resync: ignore if caps lock already is on
            if (host_keyboard_leds() & (1 << USB_LED_CAPS_LOCK)) return;
#    endif
            add_key(KC_CAPS_LOCK);
            send_keyboard_report();
            wait_ms(TAP_HOLD_CAPS_DELAY);
            del_key(KC_CAPS_LOCK);
            send_keyboard_report();

        } else if (KC_LOCKING_NUM_LOCK == code) {
#    ifdef LOCKING_RESYNC_ENABLE
            if (host_keyboard_leds() & (1 << USB_LED_NUM_LOCK)) return;
#    endif
            add_key(KC_NUM_LOCK);
            send_keyboard_report();
            wait_ms(100);
            del_key(KC_NUM_LOCK);
            send_keyboard_report();

        } else if (KC_LOCKING_SCROLL_LOCK == code) {
#    ifdef LOCKING_RESYNC_ENABLE
            if (host_keyboard_leds() & (1 << USB_LED_SCROLL_LOCK)) return;
#    endif
            add_key(KC_SCROLL_LOCK);
            send_keyboard_report();
            wait_ms(100);
            del_key(KC_SCROLL_LOCK);
            send_keyboard_report();
#endif

        } else if (IS_BASIC_KEYCODE(code)) {
            // TODO: should push command_proc out of this block?
            if (command_proc(code)) return;

            // Force a new key press if the key is already pressed
            // without this, keys with the same keycode, but different
            // modifiers will be reported incorrectly, see issue #1708
            if (is_key_pressed(code)) {
                del_key(code);
                send_keyboard_report();
            }
            add_key(code);
            send_keyboard_report();
        } else if (IS_MODIFIER_KEYCODE(code)) {
            add_mods(MOD_BIT(code));
            send_keyboard_report();

#ifdef EXTRAKEY_ENABLE
        } else if (IS_SYSTEM_KEYCODE(code)) {
            host_system_send(KEYCODE2SYSTEM(code));
        } else if (IS_CONSUMER_KEYCODE(code)) {
            host_consumer_send(KEYCODE2CONSUMER(code));
#endif

        } else if (IS_MOUSE_KEYCODE(code)) {
            register_mouse(code, true);
        }
    }
}

/** \brief Utilities for actions. (FIXME: Needs better description)
 *
 * FIXME: Needs documentation.
 */
__attribute__((weak)) void unregister_code(uint8_t code) {
    if (mm_eeconfig.devs) {
        bts_process_keys(code, false, mm_eeconfig.devs, keymap_config.no_gui, 8);
    } else {
        if (code == KC_NO) {
            return;

#ifdef LOCKING_SUPPORT_ENABLE
        } else if (KC_LOCKING_CAPS_LOCK == code) {
#    ifdef LOCKING_RESYNC_ENABLE
            // Resync: ignore if caps lock already is off
            if (!(host_keyboard_leds() & (1 << USB_LED_CAPS_LOCK))) return;
#    endif
            add_key(KC_CAPS_LOCK);
            send_keyboard_report();
            del_key(KC_CAPS_LOCK);
            send_keyboard_report();

        } else if (KC_LOCKING_NUM_LOCK == code) {
#    ifdef LOCKING_RESYNC_ENABLE
            if (!(host_keyboard_leds() & (1 << USB_LED_NUM_LOCK))) return;
#    endif
            add_key(KC_NUM_LOCK);
            send_keyboard_report();
            del_key(KC_NUM_LOCK);
            send_keyboard_report();

        } else if (KC_LOCKING_SCROLL_LOCK == code) {
#    ifdef LOCKING_RESYNC_ENABLE
            if (!(host_keyboard_leds() & (1 << USB_LED_SCROLL_LOCK))) return;
#    endif
            add_key(KC_SCROLL_LOCK);
            send_keyboard_report();
            del_key(KC_SCROLL_LOCK);
            send_keyboard_report();
#endif

        } else if (IS_BASIC_KEYCODE(code)) {
            del_key(code);
            send_keyboard_report();
        } else if (IS_MODIFIER_KEYCODE(code)) {
            del_mods(MOD_BIT(code));
            send_keyboard_report();

#ifdef EXTRAKEY_ENABLE
        } else if (IS_SYSTEM_KEYCODE(code)) {
            host_system_send(0);
        } else if (IS_CONSUMER_KEYCODE(code)) {
            host_consumer_send(0);
#endif

        } else if (IS_MOUSE_KEYCODE(code)) {
            register_mouse(code, false);
        }
    }
}

void register_weak_mods(uint8_t mods) {
    if (mods) {
        if (mm_eeconfig.devs != DEVS_USB) {
            while (mods) {
                uint8_t bitindex = biton(mods);
                bts_process_keys(0xE0 + bitindex, true, mm_eeconfig.devs, keymap_config.no_gui, 8);

                mods &= ~(0x01 << bitindex);
            }
        } else {
            add_weak_mods(mods);
            send_keyboard_report();
        }
    }
}

void unregister_weak_mods(uint8_t mods) {
    if (mods) {
        if (mm_eeconfig.devs != DEVS_USB) {
            while (mods) {
                uint8_t bitindex = biton(mods);
                bts_process_keys(0xE0 + bitindex, false, mm_eeconfig.devs, keymap_config.no_gui, 8);

                mods &= ~(0x01 << bitindex);
            }
        } else {
            del_weak_mods(mods);
            send_keyboard_report();
        }
    }
}

extern void do_code16(uint16_t code, void (*f)(uint8_t));

__attribute__((weak)) void register_code16(uint16_t code) {
    if (mm_eeconfig.devs) {
        if (QK_MODS_GET_MODS(code) & 0x1) {
            if (QK_MODS_GET_MODS(code) & 0x10)
                bts_process_keys(KC_RCTL, true, mm_eeconfig.devs, keymap_config.no_gui, 8);
            else
                bts_process_keys(KC_LCTL, true, mm_eeconfig.devs, keymap_config.no_gui, 8);
        }
        if (QK_MODS_GET_MODS(code) & 0x2) {
            if (QK_MODS_GET_MODS(code) & 0x10)
                bts_process_keys(KC_RSFT, true, mm_eeconfig.devs, keymap_config.no_gui, 8);
            else
                bts_process_keys(KC_LSFT, true, mm_eeconfig.devs, keymap_config.no_gui, 8);
        }
        if (QK_MODS_GET_MODS(code) & 0x4) {
            if (QK_MODS_GET_MODS(code) & 0x10)
                bts_process_keys(KC_RALT, true, mm_eeconfig.devs, keymap_config.no_gui, 8);
            else
                bts_process_keys(KC_LALT, true, mm_eeconfig.devs, keymap_config.no_gui, 8);
        }
        if (QK_MODS_GET_MODS(code) & 0x8) {
            if (QK_MODS_GET_MODS(code) & 0x10)
                bts_process_keys(KC_RGUI, true, mm_eeconfig.devs, keymap_config.no_gui, 8);
            else
                bts_process_keys(KC_LGUI, true, mm_eeconfig.devs, keymap_config.no_gui, 8);
        }
        bts_process_keys(QK_MODS_GET_BASIC_KEYCODE(code), true, mm_eeconfig.devs, keymap_config.no_gui, 8);
    } else {
        if (IS_MODIFIER_KEYCODE(code) || code == KC_NO) {
            do_code16(code, register_mods);
        } else {
            do_code16(code, register_weak_mods);
        }
        register_code(code);
    }
}

__attribute__((weak)) void unregister_code16(uint16_t code) {
    if (mm_eeconfig.devs) {
        if (QK_MODS_GET_MODS(code) & 0x1) {
            if (QK_MODS_GET_MODS(code) & 0x10)
                bts_process_keys(KC_RCTL, false, mm_eeconfig.devs, keymap_config.no_gui, 8);
            else
                bts_process_keys(KC_LCTL, false, mm_eeconfig.devs, keymap_config.no_gui, 8);
        }
        if (QK_MODS_GET_MODS(code) & 0x2) {
            if (QK_MODS_GET_MODS(code) & 0x10)
                bts_process_keys(KC_RSFT, false, mm_eeconfig.devs, keymap_config.no_gui, 8);
            else
                bts_process_keys(KC_LSFT, false, mm_eeconfig.devs, keymap_config.no_gui, 8);
        }
        if (QK_MODS_GET_MODS(code) & 0x4) {
            if (QK_MODS_GET_MODS(code) & 0x10)
                bts_process_keys(KC_RALT, false, mm_eeconfig.devs, keymap_config.no_gui, 8);
            else
                bts_process_keys(KC_LALT, false, mm_eeconfig.devs, keymap_config.no_gui, 8);
        }
        if (QK_MODS_GET_MODS(code) & 0x8) {
            if (QK_MODS_GET_MODS(code) & 0x10)
                bts_process_keys(KC_RGUI, false, mm_eeconfig.devs, keymap_config.no_gui, 8);
            else
                bts_process_keys(KC_LGUI, false, mm_eeconfig.devs, keymap_config.no_gui, 8);
        }
        bts_process_keys(QK_MODS_GET_BASIC_KEYCODE(code), false, mm_eeconfig.devs, keymap_config.no_gui, 8);
    } else {
        unregister_code(code);
        if (IS_MODIFIER_KEYCODE(code) || code == KC_NO) {
            do_code16(code, unregister_mods);
        } else {
            do_code16(code, unregister_weak_mods);
        }
    }
}
