#include "bled.h"
#include "multimode.h"
#include "bt_task.h"
#include "lib/lib8tion/lib8tion.h"

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

        default:
            break;
    }

    return true;
}

bool bled_rgb_matrix_indicators_user(void) {
    bled_task();

    return true;
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

void bled_keyboard_post_init_user(void) {
    bled_info.raw = eeconfig_read_user();
}

void bled_eeconfig_init_user(void) {
    bled_info.bled_mode       = BLED_MODE_CYCLE;
    bled_info.bled_Brightness = RGB_MATRIX_DEFAULT_VAL;
    bled_info.bled_speed      = RGB_MATRIX_DEFAULT_SPD;
    bled_info.bled_color      = COLOR_RAINBOW;
    eeconfig_update_user(bled_info.raw);
}
