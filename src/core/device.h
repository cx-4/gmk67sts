#ifndef GMK67STS_DEVICE_H
#define GMK67STS_DEVICE_H

#include <stdbool.h>
#include <hidapi/hidapi.h>

#define GMK_VENDOR_ID  0x320f
#define GMK_PRODUCT_ID 0x5055
#define GMK_INTERFACE  3

char* device_find_path(void);
hid_device* device_open(const char* path);
void device_close(hid_device* dev);
int device_init_hidapi(void);
void device_exit_hidapi(void);

#endif /* GMK67STS_DEVICE_H */