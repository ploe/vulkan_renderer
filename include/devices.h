#ifndef _SODA_DEVICES_H
#define _SODA_DEVICES_H

typedef struct {
  /* Device is a struct that collates the attributes pertaining to a
  VkPhysicalDevice */
  VkPhysicalDevice device;
  VkPhysicalDeviceProperties properties;
  VkPhysicalDeviceFeatures features;
} Device;

typedef struct {
  /* Devices is a container for an array of Device structs */
  uint32_t count;
  Device *devices;
} Devices;

Devices GetDevices(VkInstance instance);
Devices DestroyDevices(Devices devices);

#endif
