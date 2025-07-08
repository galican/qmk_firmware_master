#include "bled.h"

#include "rgb_matrix.h"
#include "lib/lib8tion/lib8tion.h" //声明函数SIN8

bool is_first_light_on    = true;
bool color_change_enabled = true;

// clang-format off
const HSV color_table[COLOR_COUNT] = {
    [COLOR_RED]    = {0,   255, 255},
    [COLOR_ORANGE] = {32,  255, 255},
    [COLOR_YELLOW] = {64,  255, 255},
    [COLOR_GREEN]  = {96,  255, 255},
    [COLOR_CYAN]   = {128, 255, 255},
    [COLOR_BLUE]   = {160, 255, 255},
    [COLOR_PURPLE] = {192, 255, 255},
};
// clang-format on

// 灯效任务
static uint32_t last_flow_timer = 0;
static uint8_t  flow_offset     = 0;
static uint32_t last_neon_timer = 0;
static uint8_t  neon_hue        = 0;

void bled_task(void) {
    switch (dev_info.bled_mode) {
        case BLED_MODE_CYCLE_LEFT_TO_RIGHT: {
            // 流光：慢速流动
            if (timer_elapsed32(last_flow_timer) > 5) { // 30ms 一步，越大越慢
                last_flow_timer = timer_read32();
                flow_offset++;
            }
            for (uint8_t i = 82; i <= 84; i++) {
                uint8_t led_hue = flow_offset + (i - 82) * 85;
                HSV     hsv     = {led_hue, 255, 255};
                RGB     rgb     = hsv_to_rgb(hsv);
                rgb_matrix_set_color(i, rgb.r, rgb.g, rgb.b);
            }
            break;
        }
        case BLED_MODE_NEON: {
            // 霓虹：慢速整体变色
            if (timer_elapsed32(last_neon_timer) > 5) { // 50ms 一步
                last_neon_timer = timer_read32();
                neon_hue++;
            }
            for (uint8_t i = 82; i <= 84; i++) {
                uint8_t led_hue = neon_hue + (i - 82) * 20;
                HSV     hsv     = {led_hue, 255, 255};
                RGB     rgb     = hsv_to_rgb(hsv);
                rgb_matrix_set_color(i, rgb.r, rgb.g, rgb.b);
            }
            break;
        }

        case BLED_MODE_BREATHING: {
            static uint8_t  breath_step       = 0;
            static uint32_t last_breath_timer = 0;

            // 控制呼吸速度 - 更平滑的步进 (周期约4秒)
            if (timer_elapsed32(last_breath_timer) > 15) { // 15ms 一步，更平滑
                last_breath_timer = timer_read32();
                breath_step++;
            }

            // 优化亮度曲线 - 使用更自然的呼吸效果
            // uint8_t raw_brightness = sin8(breath_step);

            // 方案1：使用三角波函数，更平滑的过渡
            uint8_t brightness = scale8(triwave8(breath_step), 200) + 1; // 55-255范围

            // 方案2：或者使用双重缓动，更自然
            // uint8_t brightness = scale8(ease8InOutCubic(sin8(breath_step)), 180) + 75;

            // 方案3：或者自定义分段函数，完全模拟真实呼吸
            // uint8_t brightness;
            // if (raw_brightness < 128) {
            //     // 吸气阶段：慢启动
            //     brightness = scale8(ease8InQuad(raw_brightness * 2), 200) + 55;
            // } else {
            //     // 呼气阶段：快结束
            //     brightness = scale8(ease8OutQuad((255 - raw_brightness) * 2), 200) + 55;
            // }
            // uint16_t time       = scale16by8(g_rgb_timer, breath_step / 8);
            // uint8_t  brightness = scale8(abs8(sin8(time) - 128) * 2, 200);

            // 彩虹呼吸 or 固定色呼吸
            uint8_t base_hue = 0;
            if (dev_info.bled_color == COLOR_RAINBOW) {
                static uint8_t rainbow_hue = 0;
                // 彩虹变色更慢，避免与呼吸抢夺注意力
                if (breath_step % 16 == 0) rainbow_hue++; // 更慢的变色
                base_hue = rainbow_hue;
            } else {
                base_hue = color_table[dev_info.bled_color].h;
            }

            for (uint8_t i = 82; i <= 84; i++) {
                HSV hsv = {base_hue, 255, brightness};
                RGB rgb = hsv_to_rgb(hsv);
                rgb_matrix_set_color(i, rgb.r, rgb.g, rgb.b);
            }
            break;
        }

        case BLED_MODE_ON_YELLOW: {
            for (uint8_t i = 82; i <= 84; i++) {
                rgb_matrix_set_color(i, RGB_YELLOW);
            }
            break;
        }

        case BLED_MODE_ON_BLUE: {
            for (uint8_t i = 82; i <= 84; i++) {
                rgb_matrix_set_color(i, RGB_BLUE);
            }
            break;
        }

        case BLED_MODE_ON_PINK: {
            for (uint8_t i = 82; i <= 84; i++) {
                rgb_matrix_set_color(i, RGB_MAGENTA);
            }
            break;
        }

        case BLED_MODE_ON_WHITE: {
            for (uint8_t i = 82; i <= 84; i++) {
                rgb_matrix_set_color(i, RGB_WHITE);
            }
            break;
        }

        case BLED_MODE_OFF:
        default: {
            for (uint8_t i = 82; i <= 84; i++) {
                rgb_matrix_set_color(i, RGB_OFF);
            }
            break;
        }
    }
}

// 模式切换函数
void bled_mode_next(void) {
    dev_info.bled_mode = (dev_info.bled_mode + 1) % 8; // 8个模式循环
    eeconfig_update_user(dev_info.raw);
}

// 逆序切换函数
void bled_mode_prev(void) {
    dev_info.bled_mode = (dev_info.bled_mode - 1 + 8) % 8; // 修复：8个模式
    eeconfig_update_user(dev_info.raw);
}

// 速度控制函数（预留）
void bled_increase_speed(void) {
    // 可以通过调整静态变量的增量来控制速度
    // 这里预留接口，具体实现可以加入全局速度参数
}

void bled_decrease_speed(void) {
    // 降低速度的接口
}

// 亮度控制函数（预留）
void bled_increase_brightness(void) {
    // 预留亮度控制接口
}

void bled_decrease_brightness(void) {
    // 预留亮度控制接口
}

void bled_color_next(void) {
    // 切换到下一个颜色
}

void bled_color_prev(void) {
    // 切换到上一个颜色
}
