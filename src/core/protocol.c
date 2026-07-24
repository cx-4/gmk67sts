#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <hidapi/hidapi.h>

#include "protocol.h"

uint16_t protocol_calc_checksum(const uint8_t* buf) {
    uint16_t sum = 0;
    for (int i = 3; i < HID_REPORT_SIZE; i++) {
        sum = (sum + buf[i]) & 0xffff;
    }
    return sum;
}

bool protocol_send_command(
    hid_device* dev,
    uint8_t cmd,
    const uint8_t* data,
    size_t data_len,
    uint32_t pos,
    uint8_t* response,
    size_t response_len
) {
    uint8_t send_buf[HID_REPORT_SIZE] = {0};
    uint8_t read_buf[HID_REPORT_SIZE];
    int ret;
    int timeout_ms = 5000;
    int max_attempts = 60;

    if (cmd < 1) {
        fprintf(stderr, "Error: Invalid command 0x%02x\n", cmd);
        return false;
    }
    if (data_len > MAX_PAYLOAD_SIZE) {
        fprintf(stderr, "Error: Data too long (%zu bytes, max %d)\n", data_len, MAX_PAYLOAD_SIZE);
        return false;
    }

    send_buf[0] = GMK_REPORT_ID;
    send_buf[3] = cmd;
    send_buf[4] = (uint8_t)data_len;
    send_buf[5] = pos & 0xff;
    send_buf[6] = (pos >> 8) & 0xff;
    send_buf[7] = (pos >> 16) & 0xff;

    if (data && data_len > 0) {
        memcpy(&send_buf[8], data, data_len);
    }

    uint16_t chk = protocol_calc_checksum(send_buf);
    send_buf[1] = chk & 0xff;
    send_buf[2] = (chk >> 8) & 0xff;

    ret = hid_write(dev, send_buf, HID_REPORT_SIZE);
    if (ret < 0) {
        fprintf(stderr, "Error: hid_write failed: %ls\n", hid_error(dev));
        return false;
    }

    for (int attempt = 0; attempt < max_attempts; attempt++) {
        ret = hid_read_timeout(dev, read_buf, HID_REPORT_SIZE, timeout_ms);
        if (ret < 0) {
            continue;
        }
        if (ret == 0) {
            break;
        }
        if (ret >= 4 && read_buf[3] == cmd) {
            if (response && response_len > 0) {
                size_t copy_len = (ret > 4) ? (size_t)(ret - 4) : 0;
                if (copy_len > response_len) copy_len = response_len;
                memcpy(response, &read_buf[4], copy_len);
            }
            return true;
        }
    }

    fprintf(stderr, "Warning: No response for command 0x%02x\n", cmd);
    return false;
}