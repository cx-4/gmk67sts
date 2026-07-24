#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "device.h"
#include "config.h"
#include "timesync.h"

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

int timesync_sync(bool verbose) {
    hid_device* dev = NULL;
    char* device_path = NULL;
    uint8_t original[CONFIG_BUFFER_SIZE];
    uint8_t modified[CONFIG_BUFFER_SIZE];
    int ret = 1;

    if (device_init_hidapi() != 0) { ret = 2; goto cleanup; }

    device_path = device_find_path();
    if (!device_path) {
        if (verbose) fprintf(stderr, "Error: GMK67 keyboard not found (VID: 0x%04x, PID: 0x%04x)\n", GMK_VENDOR_ID, GMK_PRODUCT_ID);
        ret = 1;
        goto cleanup;
    }

    dev = device_open(device_path);
    if (!dev) { ret = 2; goto cleanup; }

    if (verbose) printf("Device opened: %s\n", device_path);

    /* Step 1: Read current config into `original` (preserved for rollback) */
    if (!config_read(dev, original, verbose)) { ret = 2; goto cleanup; }

    /* Step 2: Copy to `modified` and apply time update only to `modified` */
    memcpy(modified, original, CONFIG_BUFFER_SIZE);
    config_update_time(modified, verbose);

    /* Step 3: Attempt write. On failure, roll back to `original`. */
    if (!config_write(dev, modified, verbose)) {
        rollback_write(dev, original, verbose);
        ret = 2;
        goto cleanup;
    }

    if (verbose) printf("Time synchronized successfully\n");
    ret = 0;

cleanup:
    if (dev) device_close(dev);
    if (device_path) free(device_path);
    device_exit_hidapi();
    return ret;
}