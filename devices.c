#include <vulkan/vulkan.h>

#include "instance.h"
#include "devices.h"
#include "panic.h"

static void GetQueueFamiliesProperties(PhysicalDevice *physical_device) {
  /* Populates VkQueueFamilyProperties in PhysicalDevice */
  uint32_t count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device->physical_device, &count, NULL);

  VkQueueFamilyProperties *properties = calloc(count, sizeof(VkQueueFamilyProperties));

  if (!properties)
    Panic("physical_devices/GetQueueFamiliesProperties: Unable to allocate VkPhysicalDevice array");

  vkGetPhysicalDeviceQueueFamilyProperties(physical_device->physical_device, &count, properties);

  physical_device->queue_families.count = count;
  physical_device->queue_families.properties = properties;
}

PhysicalDevices GetPhysicalDevices(VkInstance instance) {
  /* Populates an array of PhysicalDevices with their features and properties */
  uint32_t count = 0;
  vkEnumeratePhysicalDevices(instance, &count, NULL);

  if (!count) Panic("Unable to find a VkPhysicalDevice.");

  VkPhysicalDevice *vulkan_physical_devices = calloc(count, sizeof(VkPhysicalDevice));
  if (!vulkan_physical_devices)
    Panic("physical_devices/GetPhysicalDevices: Unable to allocate VkPhysicalDevice array");

  vkEnumeratePhysicalDevices(instance, &count, vulkan_physical_devices);

  PhysicalDevice *soda_physical_devices  = calloc(count, sizeof(PhysicalDevice));
  if (!soda_physical_devices)
    Panic("physical_devices/GetPhysicalDevices: Unable to allocate PhysicalDevice array");

  uint32_t i;
  for (i = 0; i < count; i++) {
    PhysicalDevice *physical_device = &soda_physical_devices[i];

    physical_device->physical_device = vulkan_physical_devices[i];

    vkGetPhysicalDeviceProperties(physical_device->physical_device, &physical_device->properties);
    vkGetPhysicalDeviceFeatures(physical_device->physical_device, &physical_device->features);
    GetQueueFamiliesProperties(physical_device);
  }

  free(vulkan_physical_devices);

  return (PhysicalDevices) {
    .count = count,
    .physical_devices = soda_physical_devices,
  };
}

int GetValidQueueFamily(PhysicalDevice *physical_device) {
  /* Returns and sets the selected VkQueueFamily */
  int i;
  for (i = 0; i < physical_device->queue_families.count; i++) {
    if (physical_device->queue_families.properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
      return physical_device->queue_families.selected = i;
  }

  return physical_device->queue_families.selected = QUEUE_FAMILY_NOT_FOUND;
}

PhysicalDevices DestroyPhysicalDevices(PhysicalDevices physical_devices) {
  /* Deallocates physical_devices and returns an empty PhysicalDevices container */
  if (physical_devices.physical_devices) free(physical_devices.physical_devices);

  return (PhysicalDevices) {
    .count = 0,
    .physical_devices = NULL,
  };
}

PhysicalDevice *PickPhysicalDevice(PhysicalDevices physical_devices) {
  /* Selects a PhysicalDevice to use */
  uint32_t i;
  for (i = 0; i < physical_devices.count; i++) {
    PhysicalDevice *physical_device = &(physical_devices.physical_devices[i]);
    uint32_t family = GetValidQueueFamily(physical_device);

    if (family == QUEUE_FAMILY_NOT_FOUND) continue;

    return physical_device;
  }

  return NULL;
}

static VkDeviceQueueCreateInfo devicequeueCreateInfos[]

VkDevice CreateLogicalDevice(PhysicalDevice *physical_device) {
  float priorities[] = {1.0f};

  VkDeviceQueueCreateInfo queue_create_infos[] = {
    {
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .queueFamilyIndex = device.queue_families.graphics,
      .queueCount = ARRAY_SIZE(priorities),
      .pQueuePriorities = priorities,
    },
  };

  VkPhysicalDeviceFeatures enabled_features = 0;

  VkDeviceCreateInfo device_create_info = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .queueCreateInfoCount = ARRAY_SIZE(queue_create_infos),
    .pQueueCreateInfos = queue_create_infos,
    .enabledLayerCount = 0, /* deprecated and ignored */
    .ppEnabledLayerNames = NULL, //deprecated and ignored
    .enabledExtensionCount = 0,
    .ppEnabledExtensionNames = NULL,
    .pEnabledFeatures &enabled_features,
  };

  VkDevice logical_device;
  VkResult created = vkCreateDevice(
    physical_device.physical_device,
    &device_create_info,
    NULL,
    &logical_device
  );

  if (created != VK_SUCCESS)
    Panic("devices/CreateQueue: Failed to vkCreateDevice");

  return logical_device;

}
