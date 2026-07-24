#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <hidapi/hidapi.h>

#include "config.h"
#include "protocol.h"
#include "device.h"

static uint8_t to_bcd(uint8_t num) {
    if (num >= 100) {
        fprintf(stderr, "Error: to_bcd expects 0-99, got %d\n", num);
        return 0;
    }
    uint8_t low = num % 10;
    uint8_t high = num / 10;
    return (high << 4) | low;
}

bool config_read(hid_device* dev, uint8_t* config, bool verbose) {
    uint8_t chunk[4];
    int i;

    if (verbose) printf("Reading configuration...\n");

    /* Drain stale data */
    uint8_t drain[64];
    for (i = 0; i < 10; i++) {
        if (hid_read_timeout(dev, drain, 64, 10) <= 0) break;
    }

    if (!protocol_send_command(dev, CMD_INIT, NULL, 0, 0, NULL, 0)) {
        if (verbose) fprintf(stderr, "Error: INIT failed\n");
        return false;
    }

    for (i = 0; i < 9; i++) {
        uint8_t dummy[4] = {0};
        if (!protocol_send_command(dev, CMD_PREP_READ, dummy, 4, (uint32_t)i * 4, NULL, 0)) {
            if (verbose) fprintf(stderr, "Error: PREP_READ failed at chunk %d\n", i);
            return false;
        }
    }
    uint8_t dummy[1] = {0};
    if (!protocol_send_command(dev, CMD_PREP_READ, dummy, 1, 36, NULL, 0)) {
        if (verbose) fprintf(stderr, "Error: PREP_READ failed at byte 36\n");
        return false;
    }

    if (!protocol_send_command(dev, CMD_COMMIT, NULL, 0, 0, NULL, 0)) {
        if (verbose) fprintf(stderr, "Error: COMMIT failed\n");
        return false;
    }

    for (i = 0; i < 12; i++) {
        uint8_t req[4] = {0};
        if (!protocol_send_command(dev, CMD_READ_DATA, req, 4, (uint32_t)i * 4, chunk, 4)) {
            if (verbose) fprintf(stderr, "Error: READ_DATA failed at chunk %d\n", i);
            return false;
        }
        memcpy(&config[i * 4], chunk, 4);
    }

    if (verbose) printf("Configuration read (%d bytes)\n", CONFIG_BUFFER_SIZE);

    if (!config_validate(config)) {
        if (verbose) {
            fprintf(stderr, "Error: Config validation failed\n");
            config_print_hex(config);
        }
        return false;
    }

    return true;
}

bool config_write(hid_device* dev, const uint8_t* config, bool verbose) {
    if (verbose) printf("Writing configuration...\n");

    if (!protocol_send_command(dev, CMD_INIT, NULL, 0, 0, NULL, 0)) {
        if (verbose) fprintf(stderr, "Error: INIT failed\n");
        return false;
    }

    if (!protocol_send_command(dev, CMD_WRITE_CONFIG, config, CONFIG_BUFFER_SIZE, 0, NULL, 0)) {
        if (verbose) fprintf(stderr, "Error: WRITE_CONFIG failed\n");
        return false;
    }

    if (!protocol_send_command(dev, CMD_COMMIT, NULL, 0, 0, NULL, 0)) {
        if (verbose) fprintf(stderr, "Error: COMMIT failed\n");
        return false;
    }

    if (verbose) printf("Configuration written successfully\n");
    return true;
}

void config_update_time(uint8_t* config, bool verbose) {
    time_t now;
    struct tm* tm_info;

    time(&now);
    tm_info = localtime(&now);

    config[IDX_TIME_SECOND]     = to_bcd(tm_info->tm_sec);
    config[IDX_TIME_MINUTE]     = to_bcd(tm_info->tm_min);
    config[IDX_TIME_HOUR]       = to_bcd(tm_info->tm_hour);
    config[IDX_TIME_DAYOFWEEK]  = (uint8_t)tm_info->tm_wday;
    config[IDX_TIME_DATE]       = to_bcd(tm_info->tm_mday);
    config[IDX_TIME_MONTH]      = to_bcd(tm_info->tm_mon + 1);
    config[IDX_TIME_YEAR]       = to_bcd((uint8_t)(tm_info->tm_year % 100));

    if (verbose) {
        printf("Time updated: %04d-%02d-%02d %02d:%02d:%02d\n",
               tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
               tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
    }
}

void config_print_hex(const uint8_t* config) {
    printf("Config (hex): ");
    for (int i = 0; i < CONFIG_BUFFER_SIZE; i++) {
        printf("%02x ", config[i]);
        if ((i + 1) % 16 == 0) printf("\n              ");
    }
    printf("\n");
}

bool config_validate(const uint8_t* config) {
    int all_zeros = 1;
    for (int i = 0; i < CONFIG_BUFFER_SIZE; i++) {
        if (config[i] != 0x00) { all_zeros = 0; break; }
    }
    if (all_zeros) {
        fprintf(stderr, "Warning: Config is all zeros - read failed\n");
        return false;
    }

    int time_bytes_valid = 0;
    for (int i = 35; i <= 41; i++) {
        uint8_t bcd = config[i];
        if ((bcd & 0x0F) <= 9 && (bcd >> 4) <= 9) time_bytes_valid++;
    }
    if (time_bytes_valid < 3) {
        fprintf(stderr, "Warning: Time fields appear invalid (%d/7 valid BCD)\n", time_bytes_valid);
        return false;
    }

    int non_zero_count = 0;
    for (int i = 0; i < CONFIG_BUFFER_SIZE; i++) {
        if (config[i] != 0x00) non_zero_count++;
    }
    if (non_zero_count < 5) {
        fprintf(stderr, "Warning: Config has very few non-zero bytes (%d)\n", non_zero_count);
        return false;
    }

    return true;
}

int config_dump(void) {
    hid_device* dev = NULL;
    char* device_path = NULL;
    uint8_t config[CONFIG_BUFFER_SIZE] = {0};
    int ret = 1;

    printf("=== READ-ONLY CONFIG DUMP (no writes to keyboard) ===\n\n");

    if (device_init_hidapi() != 0) return 1;

    device_path = device_find_path();
    if (!device_path) {
        fprintf(stderr, "Error: Device not found\n");
        device_exit_hidapi();
        return 1;
    }

    dev = device_open(device_path);
    if (!dev) {
        free(device_path);
        device_exit_hidapi();
        return 1;
    }

    printf("Device opened: %s\n", device_path);

    if (!config_read(dev, config, true)) {
        fprintf(stderr, "Error: Failed to read config\n");
        goto cleanup;
    }

    printf("\n--- Raw hex (48 bytes) ---\n");
    config_print_hex(config);

    printf("\n--- C array (paste into profiles.c as FACTORY_CONFIG) ---\n");
    printf("static const uint8_t FACTORY_CONFIG[CONFIG_BUFFER_SIZE] = {\n");
    printf("    ");
    for (int i = 0; i < CONFIG_BUFFER_SIZE; i++) {
        printf("0x%02x", config[i]);
        if (i < CONFIG_BUFFER_SIZE - 1) printf(", ");
        if ((i + 1) % 12 == 0 && i < CONFIG_BUFFER_SIZE - 1) printf("\n    ");
    }
    printf("\n");
    printf("};\n");

    printf("\n--- Parsed fields ---\n");
    printf("Byte  0: 0x%02x\n", config[0]);
    printf("Underglow: effect=0x%02x brightness=0x%02x speed=0x%02x orient=0x%02x rainbow=0x%02x RGB(0x%02x,0x%02x,0x%02x)\n",
           config[1], config[2], config[3], config[4], config[5], config[6], config[7], config[8]);
    printf("Bytes 9-20: ");
    for (int i = 9; i <= 20; i++) printf("%02x ", config[i]);
    printf("\n");
    printf("Byte 21 (winlock): 0x%02x\n", config[21]);
    printf("Bytes 22-27: ");
    for (int i = 22; i <= 27; i++) printf("%02x ", config[i]);
    printf("\n");
    printf("LED: mode=0x%02x sat=0x%02x byte30=0x%02x rainbow=0x%02x color=0x%02x\n",
           config[28], config[29], config[30], config[31], config[32]);
    printf("Byte 33 (showImage): 0x%02x\n", config[33]);
    printf("Byte 34 (image1Frames): 0x%02x\n", config[34]);
    printf("Time: sec=0x%02x min=0x%02x hour=0x%02x dow=0x%02x date=0x%02x month=0x%02x year=0x%02x\n",
           config[35], config[36], config[37], config[38], config[39], config[40], config[41]);
    printf("Byte 42: 0x%02x\n", config[42]);
    printf("Frame duration: 0x%02x 0x%02x (%d ms)\n", config[43], config[44],
           config[43] | (config[44] << 8));
    printf("Byte 45: 0x%02x\n", config[45]);
    printf("Byte 46 (image2Frames): 0x%02x\n", config[46]);
    printf("Byte 47: 0x%02x\n", config[47]);

    printf("\n=== DUMP COMPLETE (no writes were performed) ===\n");
    ret = 0;

cleanup:
    if (dev) device_close(dev);
    if (device_path) free(device_path);
    device_exit_hidapi();
    return ret;
}