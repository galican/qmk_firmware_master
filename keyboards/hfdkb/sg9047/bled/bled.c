#include "bled.h"
#include "lib/lib8tion/lib8tion.h"

#define n_rgb_matrix_set_color_all(r, g, b)   \
    do {                                      \
        for (uint8_t i = 0; i <= 82; i++) {   \
            rgb_matrix_set_color(i, r, g, b); \
        }                                     \
    } while (0)
#define n_rgb_matrix_off_all()                \
    do {                                      \
        for (uint8_t i = 0; i <= 82; i++) {   \
            rgb_matrix_set_color(i, 0, 0, 0); \
        }                                     \
    } while (0)

// Color table for fixed color modes
// clang-format off
const uint8_t color_table[COLOR_COUNT][3] = {
    [COLOR_RED]    = {0,   255, 160},
    [COLOR_ORANGE] = {21,  255, 160},
    [COLOR_YELLOW] = {43,  255, 160},
    [COLOR_GREEN]  = {85,  255, 160},
    [COLOR_CYAN]   = {128, 255, 160},
    [COLOR_BLUE]   = {170, 255, 160},
    [COLOR_PURPLE] = {191, 255, 160},
    [COLOR_WHITE]  = {0, 0, 160},
};
// clang-format on

uint8_t  all_blink_cnt;
uint32_t all_blink_time;
RGB      all_blink_color;
uint8_t  single_blink_cnt;
uint8_t  single_blink_index;
RGB      single_blink_color;
uint32_t single_blink_time;
uint32_t long_pressed_time;
uint16_t long_pressed_keycode;

bled_info_t bled_info = {0};

// clang-format on
bool bled_process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case BLED_Mode:
            if (record->event.pressed) {
                bled_info.bled_mode++;
                if (bled_info.bled_mode >= BLED_MODE_COUNT) {
                    bled_info.bled_mode = BLED_MODE_CYCLE;
                }
                eeconfig_update_user(bled_info.raw);
            }
            return false;

        case BLED_Brightness:
            if (record->event.pressed) {
                if (bled_info.bled_Brightness >= RGB_MATRIX_MAXIMUM_BRIGHTNESS) {
                    bled_info.bled_Brightness = 0;
                } else {
                    bled_info.bled_Brightness += RGB_MATRIX_VAL_STEP;
                }
                eeconfig_update_user(bled_info.raw);
            }
            return false;

        case BLED_Speed:
            if (record->event.pressed) {
                if (bled_info.bled_speed >= UINT8_MAX) { // UINT8_MAX = 255
                    bled_info.bled_speed = 0;
                } else {
                    bled_info.bled_speed = qadd8(bled_info.bled_speed, RGB_MATRIX_SPD_STEP);
                }
                eeconfig_update_user(bled_info.raw);
            }
            return false;

        case BLED_Color:
            if (record->event.pressed) {
                if (bled_info.bled_mode == BLED_MODE_SOLID || bled_info.bled_mode == BLED_MODE_BREATHING) {
                    bled_info.bled_color++;
                    if (bled_info.bled_color >= COLOR_COUNT) {
                        bled_info.bled_color = COLOR_RAINBOW;
                    }
                }
                eeconfig_update_user(bled_info.raw);
            }
            return false;

        case PDF(0):
        case PDF(2):
            if (record->event.pressed) {
                if (get_highest_layer(default_layer_state) == 0) {
                    keymap_config.no_gui = false;
                    eeconfig_update_keymap(&keymap_config);
                }
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
    // caps lock red
    if (host_keyboard_led_state().caps_lock) {
        rgb_matrix_set_color(CAPS_LOCK_LED_INDEX, RGB_WHITE);
    } else {
        if (!rgb_matrix_get_flags()) {
            rgb_matrix_set_color(CAPS_LOCK_LED_INDEX, RGB_OFF);
        }
    }

    // GUI lock white
    if (keymap_config.no_gui) {
        rgb_matrix_set_color(GUI_LOCK_LED_INDEX, RGB_WHITE);
    } else {
        if (!rgb_matrix_get_flags()) {
            rgb_matrix_set_color(GUI_LOCK_LED_INDEX, RGB_OFF);
        }
    }

    // 全键闪烁
    if (all_blink_cnt) {
        // Turn off all LEDs before blinking
        n_rgb_matrix_off_all();
        if (timer_elapsed32(all_blink_time) > 300) {
            all_blink_time = timer_read32();
            all_blink_cnt--;
        }
        if (all_blink_cnt & 0x1) {
            n_rgb_matrix_set_color_all(all_blink_color.r, all_blink_color.g, all_blink_color.b);
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

    bled_task();

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

void bled_task(void) {
    switch (bled_info.bled_mode) {
        case BLED_MODE_CYCLE: {
            uint8_t time = scale16by8(g_rgb_timer, qadd8(bled_info.bled_speed / 4, 1));
            for (uint8_t i = 83; i < 115; i++) {
                HSV hsv = {g_led_config.point[i].x - time, 255, bled_info.bled_Brightness};
                RGB rgb = hsv_to_rgb(hsv);
                rgb_matrix_set_color(i, rgb.r, rgb.g, rgb.b);
            }
            break;
        }
        case BLED_MODE_NEON: {
            // Option 1: Slow rainbow cycling (classic neon)
            uint8_t time = scale16by8(g_rgb_timer, qadd8(bled_info.bled_speed / 8, 1));
            HSV     hsv  = {time, 255, bled_info.bled_Brightness};
            RGB     rgb  = hsv_to_rgb(hsv);
            for (uint8_t i = 83; i < 115; i++) {
                rgb_matrix_set_color(i, rgb.r, rgb.g, rgb.b);
            }
            break;
        }
        case BLED_MODE_SOLID: {
            if (bled_info.bled_color == COLOR_RAINBOW) {
                // Rainbow
                for (uint8_t i = 83; i < 115; i++) {
                    HSV hsv = {(i - 83) * 8, 255, bled_info.bled_Brightness};
                    RGB rgb = hsv_to_rgb(hsv);
                    rgb_matrix_set_color(i, rgb.r, rgb.g, rgb.b);
                }
            } else {
                HSV hsv;
                hsv.h   = color_table[bled_info.bled_color][0];
                hsv.s   = color_table[bled_info.bled_color][1];
                hsv.v   = bled_info.bled_Brightness;
                RGB rgb = hsv_to_rgb(hsv);
                for (uint8_t i = 83; i < 115; i++) {
                    rgb_matrix_set_color(i, rgb.r, rgb.g, rgb.b);
                }
            }
            break;
        }
        case BLED_MODE_BREATHING: {
            if (bled_info.bled_color == COLOR_RAINBOW) {
                // Rainbow breathing effect
                uint8_t time       = scale16by8(g_rgb_timer, qadd8(bled_info.bled_speed / 4, 1));
                uint8_t brightness = scale8(abs8(sin8(time / 2) - 128) * 2, bled_info.bled_Brightness);
                // uint16_t time = scale16by8(g_rgb_timer, bled_info.bled_speed / 8);
                // uint8_t brightness = scale8(abs8(sin8(time) - 128) * 2, bled_info.bled_Brightness);
                for (uint8_t i = 83; i < 115; i++) {
                    HSV hsv = {(i - 83) * 8, 255, brightness};
                    RGB rgb = hsv_to_rgb(hsv);
                    rgb_matrix_set_color(i, rgb.r, rgb.g, rgb.b);
                }
            } else {
                HSV hsv;
                hsv.h              = color_table[bled_info.bled_color][0];
                hsv.s              = color_table[bled_info.bled_color][1];
                uint8_t time       = scale16by8(g_rgb_timer, qadd8(bled_info.bled_speed / 4, 1));
                uint8_t brightness = scale8(abs8(sin8(time / 2) - 128) * 2, bled_info.bled_Brightness);
                // uint16_t time = scale16by8(g_rgb_timer, bled_info.bled_speed / 8);
                // uint8_t brightness = scale8(abs8(sin8(time) - 128) * 2, bled_info.bled_Brightness);
                // Set brightness based on breathing effect
                hsv.v   = brightness;
                RGB rgb = hsv_to_rgb(hsv);
                for (uint8_t i = 83; i < 115; i++) {
                    rgb_matrix_set_color(i, rgb.r, rgb.g, rgb.b);
                }
            }
            break; // Added missing break statement!
        }
        case BLED_MODE_OFF: {
            for (uint8_t i = 83; i < 115; i++) {
                rgb_matrix_set_color(i, RGB_OFF);
            }
            break;
        }
        default:
            break;
    }
}

void keyboard_post_init_user(void) {
    bled_info.raw = eeconfig_read_user();
}

void eeconfig_init_user(void) {
    bled_info.bled_mode       = BLED_MODE_CYCLE;
    bled_info.bled_Brightness = RGB_MATRIX_DEFAULT_VAL;
    bled_info.bled_speed      = RGB_MATRIX_DEFAULT_SPD;
    bled_info.bled_color      = COLOR_RAINBOW;
    eeconfig_update_user(bled_info.raw);
}
