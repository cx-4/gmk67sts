#ifndef GMK67STS_PROTOCOL_H
#define GMK67STS_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <hidapi/hidapi.h>

#define GMK_REPORT_ID    0x04
#define HID_REPORT_SIZE  64
#define HID_DATA_SIZE    60
#define MAX_PAYLOAD_SIZE 56

#define CMD_INIT        0x01
#define CMD_COMMIT      0x02
#define CMD_PREP_READ   0x03
#define CMD_READ_DATA   0x05
#define CMD_WRITE_CONFIG 0x06
#define CMD_READY       0x23

uint16_t protocol_calc_checksum(const uint8_t* buf);

bool protocol_send_command(
    hid_device* dev,
    uint8_t cmd,
    const uint8_t* data,
    size_t data_len,
    uint32_t pos,
    uint8_t* response,
    size_t response_len
);

#endif /* GMK67STS_PROTOCOL_H */