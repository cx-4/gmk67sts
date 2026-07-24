#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <hidapi/hidapi.h>

#include "profiles.h"
#include "config.h"
#include "protocol.h"
#include "device.h"

/*
 * Attempts to roll back the keyboard config to `original` after a failed write.
 * If the rollback itself also fails, prints a warning — the keyboard may be
 * in a bad state and the user should replug and run --restore-defaults.
 */
static void rollback_write(hid_device* dev, const uint8_t* original, bool verbose) {
    if (verbose) printf("Write failed — attempting rollback to previous config...\n");
    if (config_write(dev, original, verbose)) {
        if (verbose) printf("✓ Rollback succeeded (keyboard restored to prior state)\n");
    } else {
        fprintf(stderr, "WARNING: Rollback also failed. Keyboard may be in a bad state.\n");
        fprintf(stderr, "         Replug the keyboard and run --restore-defaults to recover.\n");
    }
}

static const lighting_profile_t g_profiles[] = {
    {
        .name = "gaming",
        .description = "Aggressive red theme with fast effects",
        .underglow_effect = EFFECT_RAINBOW_CYCLE,
        .underglow_brightness = 9,
        .underglow_speed = 1,
        .underglow_orientation = 1,
        .underglow_rainbow = 0,
        .underglow_red = 255,
        .underglow_green = 0,
        .underglow_blue = 0,
        .led_mode = LED_MODE_FIXED_COLOR,
        .led_saturation = 9,
        .led_rainbow = 0,
        .led_color = LED_COLOR_RED,
    },
    {
        .name = "relaxed",
        .description = "Calm cyan breathing effect for typing",
        .underglow_effect = EFFECT_BREATHING,
        .underglow_brightness = 3,
        .underglow_speed = 7,
        .underglow_orientation = 1,
        .underglow_rainbow = 0,
        .underglow_red = 0,
        .underglow_green = 255,
        .underglow_blue = 255,
        .led_mode = LED_MODE_FIXED_COLOR,
        .led_saturation = 3,
        .led_rainbow = 0,
        .led_color = LED_COLOR_TEAL,
    },
    {
        .name = "party",
        .description = "Fast rainbow effects for maximum visual impact",
        .underglow_effect = EFFECT_RAINBOW_CYCLE,
        .underglow_brightness = 9,
        .underglow_speed = 0,
        .underglow_orientation = 1,
        .underglow_rainbow = 1,
        .underglow_red = 255,
        .underglow_green = 255,
        .underglow_blue = 255,
        .led_mode = LED_MODE_PULSE_RAINBOW,
        .led_saturation = 9,
        .led_rainbow = 1,
        .led_color = LED_COLOR_RED,
    },
    {
        .name = "minimal",
        .description = "All lighting off for minimal distraction",
        .underglow_effect = EFFECT_OFF,
        .underglow_brightness = 0,
        .underglow_speed = 5,
        .underglow_orientation = 1,
        .underglow_rainbow = 0,
        .underglow_red = 0,
        .underglow_green = 0,
        .underglow_blue = 0,
        .led_mode = LED_MODE_FIXED_COLOR,
        .led_saturation = 0,
        .led_rainbow = 0,
        .led_color = LED_COLOR_OFF,
    },
    {
        .name = "productivity",
        .description = "Subtle white backlight for focused work",
        .underglow_effect = EFFECT_FULL_ONE_COLOR,
        .underglow_brightness = 4,
        .underglow_speed = 5,
        .underglow_orientation = 1,
        .underglow_rainbow = 0,
        .underglow_red = 255,
        .underglow_green = 255,
        .underglow_blue = 255,
        .led_mode = LED_MODE_FIXED_COLOR,
        .led_saturation = 5,
        .led_rainbow = 0,
        .led_color = LED_COLOR_WHITE,
    },
    {
        .name = "purple-wave",
        .description = "Purple wave effect with moderate brightness",
        .underglow_effect = EFFECT_WAVE_LEFT_RIGHT,
        .underglow_brightness = 6,
        .underglow_speed = 4,
        .underglow_orientation = 1,
        .underglow_rainbow = 0,
        .underglow_red = 128,
        .underglow_green = 0,
        .underglow_blue = 255,
        .led_mode = LED_MODE_FIXED_COLOR,
        .led_saturation = 6,
        .led_rainbow = 0,
        .led_color = LED_COLOR_PURPLE,
    },
    {
        .name = "matrix",
        .description = "Green rain effect for Matrix-style aesthetics",
        .underglow_effect = EFFECT_RAINING,
        .underglow_brightness = 7,
        .underglow_speed = 2,
        .underglow_orientation = 1,
        .underglow_rainbow = 0,
        .underglow_red = 0,
        .underglow_green = 255,
        .underglow_blue = 0,
        .led_mode = LED_MODE_FIXED_COLOR,
        .led_saturation = 7,
        .led_rainbow = 0,
        .led_color = LED_COLOR_GREEN,
    },
    {
        .name = "sunset",
        .description = "Warm orange/yellow sunset colors",
        .underglow_effect = EFFECT_BREATHING,
        .underglow_brightness = 6,
        .underglow_speed = 6,
        .underglow_orientation = 1,
        .underglow_rainbow = 0,
        .underglow_red = 255,
        .underglow_green = 128,
        .underglow_blue = 0,
        .led_mode = LED_MODE_FIXED_COLOR,
        .led_saturation = 6,
        .led_rainbow = 0,
        .led_color = LED_COLOR_ORANGE,
    },
};

const int g_profile_count = sizeof(g_profiles) / sizeof(g_profiles[0]);

const lighting_profile_t* profile_get_by_name(const char* name) {
    if (!name) return NULL;
    for (int i = 0; i < g_profile_count; i++) {
        if (strcasecmp(g_profiles[i].name, name) == 0) return &g_profiles[i];
    }
    return NULL;
}

const lighting_profile_t* profile_get_by_index(int index) {
    if (index < 0 || index >= g_profile_count) return NULL;
    return &g_profiles[index];
}

void profile_apply_to_config(uint8_t* config, const lighting_profile_t* profile) {
    if (!config || !profile) return;
    
    /* ONLY modify lighting-related bytes.
     * Do NOT touch reserved/unknown bytes - they may have different
     * meanings on different keyboard models.
     */
    
    /* Underglow settings (bytes 1-8) */
    config[1] = profile->underglow_effect;
    config[2] = profile->underglow_brightness;
    config[3] = profile->underglow_speed;
    config[4] = profile->underglow_orientation;
    config[5] = profile->underglow_rainbow;
    config[6] = profile->underglow_red;
    config[7] = profile->underglow_green;
    config[8] = profile->underglow_blue;
    
    /* LED settings (bytes 28-32) */
    config[28] = profile->led_mode;
    config[29] = profile->led_saturation;
    config[31] = profile->led_rainbow;
    config[32] = profile->led_color;
    
    /* NOTE: Do NOT modify bytes 20, 21, 27 or any other reserved bytes.
     * These may control key layout or other model-specific settings.
     * Only read-modify-write should be used, preserving all unknown bytes.
     */
}

void profile_list_all(void) {
    printf("Available lighting profiles:\n");
    printf("============================\n");
    for (int i = 0; i < g_profile_count; i++) {
        printf("  %-16s - %s\n", g_profiles[i].name, g_profiles[i].description);
    }
    printf("\nUsage:\n");
    printf("  gmk67sts -r, --restore-defaults    Restore saved factory config\n");
    printf("  gmk67sts -l, --load-profile NAME   Load profile (e.g., gaming, matrix)\n");
}

int profiles_restore_defaults(void* dev_param, bool verbose) {
    hid_device* dev = NULL;
    char* device_path = NULL;
    uint8_t config[CONFIG_BUFFER_SIZE];
    int ret = 1;

    if (verbose) printf("Restoring factory default config (from saved dump)...\n");

    if (dev_param) {
        dev = (hid_device*)dev_param;
    } else {
        if (device_init_hidapi() != 0) return 1;
        device_path = device_find_path();
        if (!device_path) {
            if (verbose) fprintf(stderr, "Error: Device not found\n");
            device_exit_hidapi();
            return 1;
        }
        dev = device_open(device_path);
        if (!dev) {
            free(device_path);
            device_exit_hidapi();
            return 1;
        }
    }

    /* Copy the saved factory config, then update time to current */
    memcpy(config, FACTORY_CONFIG, CONFIG_BUFFER_SIZE);
    config_update_time(config, verbose);

    if (!config_write(dev, config, verbose)) goto cleanup;

    if (verbose) {
        printf("✓ Factory config restored\n");
        printf("  Underglow: effect=0x%02x brightness=%d RGB(%d,%d,%d)\n",
               config[1], config[2], config[6], config[7], config[8]);
        printf("  LED: mode=0x%02x sat=%d color=%d\n",
               config[28], config[29], config[32]);
        printf("  showImage=%d image1Frames=%d image2Frames=%d frameDuration=%dms\n",
               config[33], config[34], config[46],
               config[43] | (config[44] << 8));
    }
    ret = 0;

cleanup:
    if (!dev_param) {
        if (dev) device_close(dev);
        if (device_path) free(device_path);
        device_exit_hidapi();
    }
    return ret;
}

int profiles_load_by_name(void* dev_param, const char* profile_name, bool verbose) {
    hid_device* dev = NULL;
    char* device_path = NULL;
    const lighting_profile_t* profile;
    uint8_t original[CONFIG_BUFFER_SIZE];
    uint8_t modified[CONFIG_BUFFER_SIZE];
    int ret = 1;

    if (!profile_name) {
        fprintf(stderr, "Error: No profile name specified\n");
        return 1;
    }

    if (dev_param) {
        dev = (hid_device*)dev_param;
    } else {
        if (device_init_hidapi() != 0) return 1;
        device_path = device_find_path();
        if (!device_path) {
            if (verbose) fprintf(stderr, "Error: Device not found\n");
            device_exit_hidapi();
            return 1;
        }
        dev = device_open(device_path);
        if (!dev) {
            free(device_path);
            device_exit_hidapi();
            return 1;
        }
    }

    /* Step 1: Read current config into `original` (preserved for rollback) */
    if (!config_read(dev, original, verbose)) goto cleanup;

    profile = profile_get_by_name(profile_name);
    if (!profile) {
        fprintf(stderr, "Error: Profile '%s' not found\n", profile_name);
        profile_list_all();
        ret = 1;
        goto cleanup;
    }

    /* Step 2: Copy to `modified` and apply profile lighting bytes only to `modified` */
    memcpy(modified, original, CONFIG_BUFFER_SIZE);
    if (verbose) printf("Loading profile: %s - %s\n", profile->name, profile->description);
    profile_apply_to_config(modified, profile);

    /* Step 3: Attempt write. On failure, roll back to `original`. */
    if (!config_write(dev, modified, verbose)) {
        rollback_write(dev, original, verbose);
        goto cleanup;
    }

    if (verbose) printf("✓ Profile '%s' applied successfully\n", profile_name);
    ret = 0;

cleanup:
    if (!dev_param) {
        if (dev) device_close(dev);
        if (device_path) free(device_path);
        device_exit_hidapi();
    }
    return ret;
}