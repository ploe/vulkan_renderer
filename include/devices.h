#ifndef _SODA_DEVICES_H
#define _SODA_DEVICES_H

/* constants */

#define QUEUE_FAMILY_NOT_FOUND -1

/* macros */

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

/* types */

typedef struct {
  /* PhysicalDevice is a struct that collates the attributes pertaining to a
  VkPhysicalDevice */
  VkPhysicalDevice physical_device;
  VkPhysicalDeviceProperties properties;
  VkPhysicalDeviceFeatures features;

  struct {
    /* Namespace for VkQueueFamilyProperties */
    uint32_t count, selected;
    VkQueueFamilyProperties *properties;
  } queue_families;

} PhysicalDevice;

typedef struct {
  struct {
    VkDevice device;
  } vk;
} LogicalDevice;

typedef struct {
  /* PhysicalDevices is a container for an array of PhysicalDevice structs */
  uint32_t count;
  PhysicalDevice *physical_devices;
} PhysicalDevices;

/* methods */

PhysicalDevices GetPhysicalDevices(VkInstance);
PhysicalDevices DestroyPhysicalDevices(PhysicalDevices);
VkDevice CreateLogicalDevice(PhysicalDevice *);
PhysicalDevice *PickPhysicalDevice(PhysicalDevices);

#endif
