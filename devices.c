#include <vulkan/vulkan.h>

#include "instance.h"
#include "devices.h"
#include "panic.h"

static void GetQueueFamiliesProperties(Device *device) {
  uint32_t count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device->device, &count, NULL);

  VkQueueFamilyProperties *properties = calloc(count, sizeof(VkQueueFamilyProperties));

  if (!properties)
    Panic("devices/GetQueueFamiliesProperties: Unable to allocate VkPhysicalDevice array");

  vkGetPhysicalDeviceQueueFamilyProperties(device->device, &count, properties);

  device->queue_families.count = count;
  device->queue_families.properties = properties;
}

Devices GetDevices(VkInstance instance) {
  /* Populates an array of Devices with their features and properties */
  uint32_t count = 0;
  vkEnumeratePhysicalDevices(instance, &count, NULL);

  if (!count) Panic("Unable to find a VkPhysicalDevice.");

  VkPhysicalDevice *vulkan_devices = calloc(count, sizeof(VkPhysicalDevice));
  if (!vulkan_devices)
    Panic("devices/GetDevices: Unable to allocate VkPhysicalDevice array");

  vkEnumeratePhysicalDevices(instance, &count, vulkan_devices);

  Device *soda_devices  = calloc(count, sizeof(Device));
  if (!soda_devices)
    Panic("devices/GetDevices: Unable to allocate Device array");

  uint32_t i;
  for (i = 0; i < count; i++) {
    Device *device = &soda_devices[i];

    device->device = vulkan_devices[i],

    vkGetPhysicalDeviceProperties(device->device, &device->properties);
    vkGetPhysicalDeviceFeatures(device->device, &device->features);
    GetQueueFamiliesProperties(device);
  }

  free(vulkan_devices);

  return (Devices) {
    .count = count,
    .devices = soda_devices,
  };
}

Devices DestroyDevices(Devices devices) {
  /* Deallocates devices and returns an empty Devices container */
  if (devices.devices) free(devices.devices);

  return (Devices) {
    .count = 0,
    .devices = NULL,
  };
}
