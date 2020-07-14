#ifndef _SODA_DEVICES_H
#define _SODA_DEVICES_H

/* types */

typedef struct {
  /* Device is a struct that collates the attributes pertaining to a
  VkPhysicalDevice */
  VkPhysicalDevice device;
  VkPhysicalDeviceProperties properties;
  VkPhysicalDeviceFeatures features;

  struct {
    /* Namespace for VkQueueFamilyProperties */
    uint32_t count;
    VkQueueFamilyProperties *properties;
  } queue_families;

} Device;

typedef struct {
  /* Devices is a container for an array of Device structs */
  uint32_t count;
  Device *devices;
} Devices;

/* methods */

Devices GetDevices(VkInstance);
Devices DestroyDevices(Devices);

#endif
