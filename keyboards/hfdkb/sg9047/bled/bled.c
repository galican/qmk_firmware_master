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

#ifdef RGB_MATRIX_ENABLE
#    include "rgb_matrix.h"
#    define __ NO_LED
// clang-format off
// __attribute__((weak)) led_config_t g_led_config = {
//     {
//         { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, __, __},
//         {27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13},
//         {28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42},
//         {56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, __, 44, 43},
//         {57, __, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, __},
//         {82, 81, 80, __, __, __, 77, __, __, __, 74, 73, 72, 71, 70},
//     },
//     {
//         {0, 0},    {19, 0},   {34, 0},   {49, 0},   {63, 0},   {82, 0},   {97, 0},   {112, 0},  {127, 0},  {146, 0},  {160, 0},  {175, 0},  {190, 0},
//         {224, 15}, {201, 15}, {179, 15}, {164, 15}, {149, 15}, {134, 15}, {119, 15}, {105, 15}, {90, 15},  {75, 15},  {60, 15},  {45, 15},  {30, 15},  {15, 15},  {0, 15},
//         {4, 27},   {22, 27},  {37, 27},  {52, 27},  {67, 27},  {82, 27},  {97, 27},  {112, 27}, {127, 27}, {142, 27}, {157, 27}, {172, 27}, {187, 27}, {205, 27}, {224, 27},
//         {224, 40}, {200, 40}, {175, 40}, {161, 40}, {146, 40}, {131, 40}, {116, 40}, {101, 40}, {86, 40},  {71, 40},  {56, 40},  {41, 40},  {26, 40},  {6, 40},
//         {9, 52},   {34, 52},  {49, 52},  {63, 52},  {78, 52},  {93, 52},  {108, 52}, {123, 52}, {138, 52}, {153, 52}, {168, 52}, {188, 52}, {209, 52},
//         {224, 64}, {209, 64}, {194, 64}, {170, 64}, {151, 64}, {115, 64}, {105, 64}, {95, 64},  {85, 64},  {75, 64},  {39, 64},  {21, 64},  {2, 64},
//         {7, 0},    {14, 0},   {21, 0},   {28, 0},   {35, 0},   {42, 0},   {49, 0},   {56, 0},   {63, 0},   {70, 0},   {77, 0},   {84, 0},   {91, 0},   {98, 0},   {105, 0},  {112, 0},
//         {119, 0},  {126, 0},  {133, 0},  {140, 0},  {147, 0},  {154, 0},  {161, 0},  {168, 0},  {175, 0},  {182, 0},  {189, 0},  {196, 0},  {203, 0},  {210, 0},  {217, 0},  {224, 0}
//     },
//     {
//         4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
//         4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
//         4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
//         4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,    4, 4,
//         4,    8, 1, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
//         1, 4, 4, 4, 4, 1, 1, 2, 2, 4, 2, 2, 1, 8, 1,
//         2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
//         2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
//     },
// };
#endif
// clang-format on
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
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
