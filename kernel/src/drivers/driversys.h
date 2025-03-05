/*
 * FreeCore - A free operating system kernel
 * Copyright (C) 2025 FreeCore Development Team
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * Device Driver Management System
 */

#ifndef _KERNEL_DRIVERS_DRIVERSYS_H
#define _KERNEL_DRIVERS_DRIVERSYS_H

#include <stdint.h>
#include <stddef.h>

/* Maximum number of supported device classes */
#define MAX_DEVICE_CLASSES 16
#define MAX_DRIVERS_PER_CLASS 32

/* Device driver states */
typedef enum {
    DRIVER_STATE_UNLOADED,
    DRIVER_STATE_INITIALIZING,
    DRIVER_STATE_READY,
    DRIVER_STATE_ERROR
} driver_state_t;

/* Device classes */
typedef enum {
    DEVICE_CLASS_UNKNOWN,
    DEVICE_CLASS_STORAGE,
    DEVICE_CLASS_NETWORK,
    DEVICE_CLASS_DISPLAY,
    DEVICE_CLASS_INPUT,
    DEVICE_CLASS_AUDIO,
    DEVICE_CLASS_USB,
    DEVICE_CLASS_PCI,
    /* Add more device classes as needed */
    DEVICE_CLASS_MAX
} device_class_t;

/* Forward declaration of driver structure */
struct device_driver;

/* Driver operations structure */
typedef struct {
    int (*probe)(struct device_driver *driver);
    int (*remove)(struct device_driver *driver);
    int (*suspend)(struct device_driver *driver);
    int (*resume)(struct device_driver *driver);
} driver_ops_t;

/* Device driver structure */
typedef struct device_driver {
    const char *name;           /* Driver name */
    device_class_t device_class; /* Device class */
    driver_state_t state;       /* Current driver state */
    driver_ops_t *ops;          /* Driver operations */
    void *private_data;         /* Driver-specific data */
} device_driver_t;

/* Device driver management functions */
int device_driver_register(device_driver_t *driver);
int device_driver_unregister(device_driver_t *driver);
device_driver_t* device_driver_find(const char *name, device_class_t device_class);
int device_driver_init(void);
int device_driver_enumerate(
    device_class_t device_class, 
    int (*callback)(device_driver_t *driver, void *context),
    void *context
);

/* Driver initialization function */
void drivers_early_init(void);

/* Helper macros for driver registration */
#define DEFINE_DRIVER(name, class, probe_func, remove_func) \
    static driver_ops_t name##_ops = { \
        .probe = probe_func, \
        .remove = remove_func, \
        .suspend = NULL, \
        .resume = NULL \
    }; \
    static device_driver_t name##_driver = { \
        .name = #name, \
        .device_class = class, \
        .state = DRIVER_STATE_UNLOADED, \
        .ops = &name##_ops, \
        .private_data = NULL \
    }

#endif /* _KERNEL_DRIVERS_DRIVERSYS_H */