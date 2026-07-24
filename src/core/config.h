#ifndef GMK67STS_CONFIG_H
#define GMK67STS_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include <hidapi/hidapi.h>

#define CONFIG_BUFFER_SIZE 48

#define IDX_TIME_SECOND     35
#define IDX_TIME_MINUTE     36
#define IDX_TIME_HOUR       37
#define IDX_TIME_DAYOFWEEK  38
#define IDX_TIME_DATE       39
#define IDX_TIME_MONTH      40
#define IDX_TIME_YEAR       41

bool config_read(hid_device* dev, uint8_t* config, bool verbose);
bool config_write(hid_device* dev, const uint8_t* config, bool verbose);
void config_update_time(uint8_t* config, bool verbose);
void config_print_hex(const uint8_t* config);
bool config_validate(const uint8_t* config);

/**
 * Read-only dump: reads current config from keyboard and prints it
 * as a C array. Does NOT write anything to the keyboard.
 * @return 0 on success, non-zero on failure
 */
int config_dump(void);

#endif /* GMK67STS_CONFIG_H */