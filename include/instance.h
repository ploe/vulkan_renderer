#ifndef _SODA_INSTANCE_H
#define _SODA_INSTANCE_H

#include <vulkan/vulkan.h>

#include "devices.h"

/* types */

typedef struct {
  /* ExtensionProperties is a container for the VkExtensionProperties that are
  supported by Vulkan */
  uint32_t count;
  VkExtensionProperties *data;
} ExtensionProperties;

typedef struct {
  /* LayerProperties is a container for the VkLayerProperties that are supported
  by Vulkan */
  uint32_t count;
  VkLayerProperties *data;
} LayerProperties;

typedef struct {
  /* RequiredProperties is a container for holding required Vulkan Properties */
  const char **data;
  uint32_t count;
} RequiredProperties;

typedef struct {
  /* Instance namespace for the layers and extensions supported by Vulkan */
  ExtensionProperties extensions;
  LayerProperties layers;
} InstanceSupports;

typedef struct {
  /* Instance namespace for the layers and extensions required by Vulkan */
  RequiredProperties extensions, layers;
} InstanceRequires;

typedef struct {
  /* Instance is wrapper for the VkInstance and its related state */
  VkInstance instance;
  VkDebugUtilsMessengerEXT debug_messenger;

  InstanceRequires requires;
  InstanceSupports supports;

  PhysicalDevices physical;
} Instance;

Instance CreateInstance();
void DestroyInstance(Instance);

#endif
