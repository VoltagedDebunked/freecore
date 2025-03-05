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
 * Device Driver Management System Implementation
 */

#include "driversys.h"
#include "../io.h"
#include "../../minstd.h"

/* Device driver registry */
static struct {
    device_driver_t *drivers[MAX_DEVICE_CLASSES][MAX_DRIVERS_PER_CLASS];
    int driver_count[MAX_DEVICE_CLASSES];
} device_registry = {0};

/**
 * Initialize the device driver subsystem
 * @return 0 on success, negative on error
 */
int device_driver_init(void) {
    kprintf("Initializing Device Driver Subsystem...\n");
    
    /* Clear the device registry */
    memset(&device_registry, 0, sizeof(device_registry));
    
    kprintf("Device Driver Subsystem initialized.\n");
    return 0;
}

/**
 * Register a device driver
 * @param driver Pointer to the device driver to register
 * @return 0 on success, negative on error
 */
int device_driver_register(device_driver_t *driver) {
    /* Validate input */
    if (!driver || !driver->name || driver->device_class >= DEVICE_CLASS_MAX) {
        kerr("Invalid driver registration attempt\n");
        return -1;
    }
    
    /* Check if the device class has room for more drivers */
    int class_idx = driver->device_class;
    if (device_registry.driver_count[class_idx] >= MAX_DRIVERS_PER_CLASS) {
        kerr("Device class %d is full, cannot register more drivers\n", class_idx);
        return -1;
    }
    
    /* Add driver to registry */
    int slot = device_registry.driver_count[class_idx]++;
    device_registry.drivers[class_idx][slot] = driver;
    
    /* Mark driver as initializing */
    driver->state = DRIVER_STATE_INITIALIZING;
    
    /* Call driver's probe function if available */
    if (driver->ops && driver->ops->probe) {
        int probe_result = driver->ops->probe(driver);
        if (probe_result == 0) {
            driver->state = DRIVER_STATE_READY;
            kprintf("Driver %s registered successfully\n", driver->name);
        } else {
            driver->state = DRIVER_STATE_ERROR;
            kerr("Driver %s probe failed\n", driver->name);
            return probe_result;
        }
    } else {
        driver->state = DRIVER_STATE_READY;
        kprintf("Driver %s registered without probe\n", driver->name);
    }
    
    return 0;
}

/**
 * Unregister a device driver
 * @param driver Pointer to the device driver to unregister
 * @return 0 on success, negative on error
 */
int device_driver_unregister(device_driver_t *driver) {
    /* Validate input */
    if (!driver || driver->device_class >= DEVICE_CLASS_MAX) {
        kerr("Invalid driver unregistration attempt\n");
        return -1;
    }
    
    int class_idx = driver->device_class;
    
    /* Find and remove the driver */
    for (int i = 0; i < device_registry.driver_count[class_idx]; i++) {
        if (device_registry.drivers[class_idx][i] == driver) {
            /* Call driver's remove function if available */
            if (driver->ops && driver->ops->remove) {
                driver->ops->remove(driver);
            }
            
            /* Mark driver as unloaded */
            driver->state = DRIVER_STATE_UNLOADED;
            
            /* Shift remaining drivers */
            for (int j = i; j < device_registry.driver_count[class_idx] - 1; j++) {
                device_registry.drivers[class_idx][j] = device_registry.drivers[class_idx][j + 1];
            }
            
            device_registry.driver_count[class_idx]--;
            
            kprintf("Driver %s unregistered successfully\n", driver->name);
            return 0;
        }
    }
    
    kerr("Driver %s not found in registry\n", driver->name);
    return -1;
}

/**
 * Find a driver by name and device class
 * @param name Name of the driver
 * @param device_class Device class to search in
 * @return Pointer to the driver, or NULL if not found
 */
device_driver_t* device_driver_find(const char *name, device_class_t device_class) {
    /* Validate input */
    if (!name || device_class >= DEVICE_CLASS_MAX) {
        return NULL;
    }
    
    /* Search for the driver */
    for (int i = 0; i < device_registry.driver_count[device_class]; i++) {
        device_driver_t *driver = device_registry.drivers[device_class][i];
        if (driver && strcmp(driver->name, name) == 0) {
            return driver;
        }
    }
    
    return NULL;
}

/**
 * Enumerate all drivers in a specific device class
 * @param device_class Device class to enumerate
 * @param callback Function to call for each driver
 * @return Number of drivers processed
 */
int device_driver_enumerate(
    device_class_t device_class, 
    int (*callback)(device_driver_t *driver, void *context),
    void *context
) {
    /* Validate input */
    if (device_class >= DEVICE_CLASS_MAX || !callback) {
        return -1;
    }
    
    int processed = 0;
    
    /* Call callback for each driver in the class */
    for (int i = 0; i < device_registry.driver_count[device_class]; i++) {
        device_driver_t *driver = device_registry.drivers[device_class][i];
        
        if (driver) {
            int result = callback(driver, context);
            if (result != 0) {
                /* Callback can return non-zero to stop enumeration */
                break;
            }
            processed++;
        }
    }
    
    return processed;
}

/* Driver initialization function to be called during kernel startup */
void drivers_early_init(void) {
    /* Initialize driver subsystem */
    device_driver_init();
}