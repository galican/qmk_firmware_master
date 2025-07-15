#include "bled.h"

uint8_t  all_blink_cnt;
uint32_t all_blink_time;
RGB      all_blink_color;
uint8_t  single_blink_cnt;
uint8_t  single_blink_index;
RGB      single_blink_color;
uint32_t single_blink_time;
uint32_t long_pressed_time;
uint16_t long_pressed_keycode;

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case BLED_Mode:
            if (record->event.pressed) {
            }

            return false;

        case BLED_Brightness:
            if (record->event.pressed) {
            }
            return false;

        case BLED_Speed:
            if (record->event.pressed) {
            }
            return false;

        case BLED_Color:
            if (record->event.pressed) {
            }
            return false;

        case PDF(0):
        case PDF(2):
            if (record->event.pressed) {
                all_blink_cnt   = 6;
                all_blink_time  = timer_read32();
                all_blink_color = (RGB){RGB_WHITE}; // White color
            }
            return true;

        case RM_VALU:
            if (record->event.pressed) {
                rgb_matrix_increase_val();
                if (rgb_matrix_get_val() >= RGB_MATRIX_MAXIMUM_BRIGHTNESS) {
                    all_blink_cnt   = 6;
                    all_blink_time  = timer_read32();
                    all_blink_color = (RGB){RGB_WHITE}; // White color
                }
            }
            return false;
        case RM_VALD:
            if (record->event.pressed) {
                rgb_matrix_decrease_val();
                if (rgb_matrix_get_val() == 0) {
                    all_blink_cnt   = 6;
                    all_blink_time  = timer_read32();
                    all_blink_color = (RGB){RGB_WHITE}; // White color
                }
            }
            return false;
        case RM_SPDU:
            if (record->event.pressed) {
                rgb_matrix_increase_speed();
                if (rgb_matrix_get_speed() >= UINT8_MAX) {
                    all_blink_cnt   = 6;
                    all_blink_time  = timer_read32();
                    all_blink_color = (RGB){RGB_WHITE}; // White color
                }
            }
            return false;
        case RM_SPDD:
            if (record->event.pressed) {
                rgb_matrix_decrease_speed();
                if (rgb_matrix_get_speed() == 0) {
                    all_blink_cnt   = 6;
                    all_blink_time  = timer_read32();
                    all_blink_color = (RGB){RGB_WHITE}; // White color
                }
            }
            return false;

        case EE_CLR:
            if (record->event.pressed) {
                long_pressed_time    = timer_read32();
                long_pressed_keycode = EE_CLR;
            } else {
                long_pressed_time = 0;
            }
            return false;

        default:
            break;
    }

    return true;
}

bool rgb_matrix_indicators_user(void) {
    // 全键闪烁
    if (all_blink_cnt) {
        // Turn off all LEDs before blinking
        rgb_matrix_set_color_all(RGB_OFF);
        if (timer_elapsed32(all_blink_time) > 300) {
            all_blink_time = timer_read32();
            all_blink_cnt--;
        }
        if (all_blink_cnt & 0x1) {
            rgb_matrix_set_color_all(all_blink_color.r, all_blink_color.g, all_blink_color.b);
        }
    }

    // 单键闪烁
    if (single_blink_cnt) {
        if (timer_elapsed32(single_blink_time) > 300) {
            single_blink_time = timer_read32();
            single_blink_cnt--;
        }
        if (single_blink_cnt % 2) {
            rgb_matrix_set_color(single_blink_index, single_blink_color.r, single_blink_color.g, single_blink_color.b);
        } else {
            rgb_matrix_set_color(single_blink_index, RGB_OFF);
        }
    }

    return true;
}

void housekeeping_task_user(void) {
    if ((timer_elapsed32(long_pressed_time) > 3000) && (long_pressed_time)) {
        long_pressed_time = 0;
        switch (long_pressed_keycode) {
            case EE_CLR:
                eeconfig_init();
                eeconfig_update_rgb_matrix_default();
                keymap_config.nkro   = 1;
                keymap_config.no_gui = 0;

                all_blink_cnt   = 6;
                all_blink_time  = timer_read32();
                all_blink_color = (RGB){RGB_WHITE}; // White color
                break;
            default:
                break;
        }
    }
}
