#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hidapi/hidapi.h>

#include "device.h"

char* device_find_path(void) {
    struct hid_device_info* devs;
    struct hid_device_info* cur;
    char* device_path = NULL;

    devs = hid_enumerate(GMK_VENDOR_ID, GMK_PRODUCT_ID);
    if (!devs) return NULL;

    cur = devs;
    while (cur) {
        if (cur->interface_number == GMK_INTERFACE) {
            device_path = strdup(cur->path);
            break;
        }
        cur = cur->next;
    }

    if (!device_path && devs) {
        device_path = strdup(devs->path);
    }

    hid_free_enumeration(devs);
    return device_path;
}

hid_device* device_open(const char* path) {
    hid_device* dev = hid_open_path(path);
    if (!dev) {
        fprintf(stderr, "Error: Failed to open device at %s\n", path);
        fprintf(stderr, "Hint: Run with sudo or add udev rule\n");
        return NULL;
    }
    return dev;
}

void device_close(hid_device* dev) {
    if (dev) hid_close(dev);
}

int device_init_hidapi(void) {
    if (hid_init() != 0) {
        fprintf(stderr, "Error: Failed to initialize hidapi\n");
        return -1;
    }
    return 0;
}

void device_exit_hidapi(void) {
    hid_exit();
}