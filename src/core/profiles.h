#ifndef GMK67STS_PROFILES_H
#define GMK67STS_PROFILES_H

#include <stdint.h>
#include <stdbool.h>
#include <hidapi/hidapi.h>

#include "config.h"

#define EFFECT_OFF                       0x00
#define EFFECT_HORIZONTAL_DIMMING_WAVE   0x01
#define EFFECT_HORIZONTAL_PULSE_WAVE     0x02
#define EFFECT_WATERFALL                 0x03
#define EFFECT_FULL_CYCLING_COLORS       0x04
#define EFFECT_BREATHING                 0x05
#define EFFECT_FULL_ONE_COLOR            0x06
#define EFFECT_GLOW_PRESSED_KEY          0x07
#define EFFECT_GLOW_SPREADING            0x08
#define EFFECT_GLOW_ROW                  0x09
#define EFFECT_RANDOM_PATTERN            0x0a
#define EFFECT_RAINBOW_CYCLE             0x0b
#define EFFECT_RAINBOW_WATERFALL         0x0c
#define EFFECT_WAVE_FROM_CENTER          0x0d
#define EFFECT_CIRCLING_JK               0x0e
#define EFFECT_RAINING                   0x0f
#define EFFECT_WAVE_LEFT_RIGHT           0x10
#define EFFECT_SLOW_SATURATION_CYCLE     0x11
#define EFFECT_SLOW_RAINBOW_FROM_CENTER  0x12

#define LED_MODE_BLINKING_ONE_COLOR      0x00
#define LED_MODE_PULSE_RAINBOW           0x01
#define LED_MODE_BLINKING_ONE_COLOR_ALT  0x02
#define LED_MODE_FIXED_COLOR             0x03
#define LED_MODE_FIXED_COLOR_ALT         0x04

#define LED_COLOR_RED    0x00
#define LED_COLOR_ORANGE 0x01
#define LED_COLOR_YELLOW 0x02
#define LED_COLOR_GREEN  0x03
#define LED_COLOR_TEAL   0x04
#define LED_COLOR_BLUE   0x05
#define LED_COLOR_PURPLE 0x06
#define LED_COLOR_WHITE  0x07
#define LED_COLOR_OFF    0x08

/* Default frame duration in ms */
#define DEFAULT_FRAME_DURATION_MS 100

typedef struct {
    const char* name;
    const char* description;
    uint8_t underglow_effect;
    uint8_t underglow_brightness;
    uint8_t underglow_speed;
    uint8_t underglow_orientation;
    uint8_t underglow_rainbow;
    uint8_t underglow_red;
    uint8_t underglow_green;
    uint8_t underglow_blue;
    uint8_t led_mode;
    uint8_t led_saturation;
    uint8_t led_rainbow;
    uint8_t led_color;
} lighting_profile_t;

/*
 * Factory default config - dumped from the user's keyboard (2026-07-23).
 * This is the known-good 48-byte config buffer that --restore-defaults writes.
 * Time bytes (35-41) are overwritten with current time on restore.
 *
 * Layout:
 *   [0]      : 0x00
 *   [1-8]    : underglow (effect=0x06 FULL_ONE_COLOR, brightness=9, speed=4,
 *              orient=1, rainbow=0, RGB=0xff,0x01,0x00)
 *   [9-19]   : zeros
 *   [20]     : 0xff (reserved)
 *   [21]     : winlock = 0x00
 *   [22-27]  : zeros
 *   [28-32]  : LED (mode=0x03 FIXED_COLOR, sat=7, byte30=0x02, rainbow=0, color=0x03 GREEN)
 *   [33]     : showImage = 0x02 (show slot 1)
 *   [34]     : image1Frames = 0x01
 *   [35-41]  : time (overwritten on restore)
 *   [42]     : 0x00
 *   [43-44]  : frameDuration = 100ms (little-endian)
 *   [45]     : 0x00
 *   [46]     : image2Frames = 0x01
 *   [47]     : 0x00
 */
static const uint8_t FACTORY_CONFIG[CONFIG_BUFFER_SIZE] = {
    0x00, 0x06, 0x09, 0x04, 0x01, 0x00, 0xff, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x03, 0x07, 0x02, 0x00, 0x03, 0x02, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x01, 0x00
};

extern const int g_profile_count;

const lighting_profile_t* profile_get_by_name(const char* name);
const lighting_profile_t* profile_get_by_index(int index);
void profile_apply_to_config(uint8_t* config, const lighting_profile_t* profile);
void profile_list_all(void);
int profiles_restore_defaults(void* dev, bool verbose);
int profiles_load_by_name(void* dev, const char* profile_name, bool verbose);

#endif /* GMK67STS_PROFILES_H */