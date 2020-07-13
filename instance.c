#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#include "instance.h"
#include "devices.h"

/* types */

typedef bool (*ValidatePropertyNameMethod)(Instance, const char *);
/* function pointer type for the Properties name validation methods */

/* function prototypes */

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessengerCallback();

/* macros */

#define ARRAY_SIZE(array, type) (sizeof(array) / sizeof(type))

/* data */

VkDebugUtilsMessengerCreateInfoEXT DEBUG_UTILS_CREATE_INFO = {
    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    .pNext = NULL,
    .flags =  0,
    .messageSeverity =
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    .messageType =
      VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    .pfnUserCallback = DebugMessengerCallback,
    .pUserData = NULL,
};

const char *RENDERER_DEBUG_EXTENSIONS[] = {
  /* RENDERER_DEBUG_EXTENSIONS are the RequiredProperties to enable in debug
  mode */
  "VK_EXT_debug_utils"
};

const char *RENDERER_DEBUG_LAYERS[] = {
  /* These are the required layers when debug mode is enabled */
  "VK_LAYER_KHRONOS_validation"
};

/* code */

static void Panic(const char *format, ...) {
  /* Prints an error message and then closes the app */
  va_list args;
  va_start(args, format);

  vfprintf(stderr, format, args);

  va_end(args);
  abort();
}

static PhysicalDevices CreatePhysicalDevices(VkInstance instance) {
  uint32_t count;
  vkEnumeratePhysicalDevices(instance, &count, NULL);

  if (!count) Panic("Unabled to find PhysicalDevices with Vulkan support\n");

  VkPhysicalDevice *devices = calloc(count, sizeof(VkPhysicalDevice));
  if (!devices) Panic("Unable to allocate VkPhysicalDevice array\n");

  vkEnumeratePhysicalDevices(instance, &count, devices);

  return (PhysicalDevices) {
    .count = count,
    .devices = devices,
  };
}

static RequiredProperties GetGlfwRequiredProperties() {
  /* Returns the Extensions required by GLFW */
  RequiredProperties required;
  required.data = glfwGetRequiredInstanceExtensions(&required.count);

  return required;
}

RequiredProperties EmptyRequiredProperties = {
  /* An empty RequiredProperties struct. Use this if we want to toggle
  the RequiredProperties off */
    .count = 0,
    .data = NULL,
};


static RequiredProperties GetRendererRequiredLayers(bool debug) {
  /* Return the required extensions for the Renderer (i.e. this app) */
  if (!debug) return EmptyRequiredProperties;

  return (RequiredProperties) {
    .count = ARRAY_SIZE(RENDERER_DEBUG_LAYERS, const char *),
    .data = RENDERER_DEBUG_LAYERS
  };
}

static RequiredProperties GetRendererRequiredProperties(bool debug) {
    /* Return the required extensions for the Renderer (i.e. this app) */
    if (!debug) return EmptyRequiredProperties;

    return (RequiredProperties) {
      .count = ARRAY_SIZE(RENDERER_DEBUG_EXTENSIONS, const char *),
      .data = RENDERER_DEBUG_EXTENSIONS
    };
}

static uint32_t GetTotalRequiredProperties(RequiredProperties sources[], size_t elems) {
  /* Returns the sum of the count members of each of the RequiredProperties */
  uint32_t total = 0;

  size_t i;
  for (i = 0; i < elems; i++)
    total += sources[i].count;

  return total;
}

static RequiredProperties CreateRequiredProperties(RequiredProperties sources[], size_t elems, uint32_t total) {
  /* Allocates and initialises of strings for the RequiredProperties */
  if (total == 0) return EmptyRequiredProperties;

  const char **required = calloc(total, sizeof(const char *));

  if (!required) Panic("renderer/vulkan: failed to allocate data for RequiredProperties");

  size_t i;
  const char **dst = required;
  for (i = 0; i < elems; i++) {
      RequiredProperties source = sources[i];

      size_t size = source.count * sizeof(const char *);
      memcpy(dst, source.data, size);

      dst += source.count;
  }

  return (RequiredProperties) {
    .data = required,
    .count = total
  };
}

static void DestroyRequiredProperties(RequiredProperties required)  {
  /* Deallocates the RequiredProperties struct and sets the values to 0 */
  if (required.data) {
    free(required.data);
    required.data = NULL;
  }

  required.count = 0;
}

static RequiredProperties GetRequiredExtensions() {
  /* Collates the RequiredProperties for the Renderer/other libs */
  RequiredProperties sources[] = {
    GetRendererRequiredProperties(true),
    GetGlfwRequiredProperties(),
  };

  size_t elems = ARRAY_SIZE(sources, RequiredProperties);
  uint32_t total = GetTotalRequiredProperties(sources, elems);

  return CreateRequiredProperties(sources, elems, total);
}

static RequiredProperties GetRequiredLayers() {
  RequiredProperties sources[] = {
    GetRendererRequiredLayers(true)
  };

  size_t elems = ARRAY_SIZE(sources, RequiredProperties);
  uint32_t total = GetTotalRequiredProperties(sources, elems);

  return CreateRequiredProperties(sources, elems, total);
}

static ExtensionProperties GetSupportedExtensions() {
  /* Gets the supported ExtensionProperties */
  uint32_t count = 0;
  vkEnumerateInstanceExtensionProperties(NULL, &count, NULL);

  VkExtensionProperties *data = calloc(count, sizeof(VkExtensionProperties));
  if (!data) Panic("renderer/vulkan: unable to allocate ExtensionProperties data");

  vkEnumerateInstanceExtensionProperties(NULL, &count, data);

  return (ExtensionProperties) {
    .count = count,
    .data = data
  };
}

static LayerProperties GetSupportedLayers() {
  /* Gets the supported LayerProperties */
  uint32_t count = 0;
  vkEnumerateInstanceLayerProperties(&count, NULL);

  VkLayerProperties *data = calloc(count, sizeof(VkLayerProperties));
  if (!data) Panic("renderer/vulkan: unable to allocate LayerProperties data");

  vkEnumerateInstanceLayerProperties(&count, data);

  return (LayerProperties) {
    .data = data,
    .count = count
  };
}

static bool ValidExtensionName(Instance instance, const char *extension_name) {
  /* Returns true if extension_name can be found in the ExtensionProperties */
  VkExtensionProperties *ep = instance.supports.extensions.data;
  uint32_t count = instance.supports.extensions.count;

  uint32_t i;
  for (i = 0; i < count; i++)
    if (strcmp(extension_name, ep[i].extensionName) == 0) return true;

  return false;
}

static bool ValidLayerName(Instance instance, const char *layer_name) {
  /* Returns true if the layer_name can be found in the supported
  LayerProperties */
  VkLayerProperties *lp  = instance.supports.layers.data;
  uint32_t count = instance.supports.layers.count;

  uint32_t i;
  for (i = 0; i < count; i++)
    if (strcmp(layer_name, lp[i].layerName) == 0) return true;

  return false;
}


static bool ValidateRequiredProperties(Instance instance, RequiredProperties required, ValidatePropertyNameMethod method) {
  /* Checks to see if the RequiredProperties exist within Vulkan */

  int i;
  for (i = 0; i < required.count; i++) {
    const char *name = required.data[i];
    if (method(instance, name)) continue;

    Panic("renderer/vulkan: %s unavailable\n", name);
  }

  return true;
}

static bool ValidateRequiredExtensions(Instance instance) {
  /* Validates the RequiredExtensions */
  return ValidateRequiredProperties(instance, instance.requires.extensions, ValidExtensionName);
}

static bool ValidateRequiredLayers(Instance instance) {
  /* Validates the RequiredLayers */
  return ValidateRequiredProperties(instance, instance.requires.layers, ValidLayerName);
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessengerCallback(
  VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
  VkDebugUtilsMessageTypeFlagsEXT messageType,
  const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
  void *pUserData) {
  fprintf(stderr, "%s\n", pCallbackData->pMessage);

  return VK_FALSE;
}

static VkDebugUtilsMessengerEXT CreateDebugMessenger(VkInstance instance) {
  /* Create the VkDebugUtilsMessengerEXT */
  VkDebugUtilsMessengerEXT debug_messenger;

  PFN_vkCreateDebugUtilsMessengerEXT create = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
  if (!create) Panic("Unable to get vkCreateDebugUtilsMessengerEXT");

  create(instance, &DEBUG_UTILS_CREATE_INFO, NULL, &debug_messenger);

  return debug_messenger;
}

static void DestroyDebugMessenger(Instance instance) {
  /* Destroy the VkDebugUtilsMessengerEXT */
  PFN_vkDestroyDebugUtilsMessengerEXT destroy = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance.instance, "vkDestroyDebugUtilsMessengerEXT");
  if (!destroy) return;

  destroy(instance.instance, instance.debug_messenger, NULL);
}

Instance CreateInstance() {
  /* Creates the VkInstance and get the rest of the Vulkan's state */
  Instance instance;

  instance.supports.extensions = GetSupportedExtensions();
  instance.requires.extensions = GetRequiredExtensions();
  ValidateRequiredExtensions(instance);

  instance.supports.layers = GetSupportedLayers();
  instance.requires.layers = GetRequiredLayers();
  ValidateRequiredLayers(instance);


  VkApplicationInfo appInfo = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pNext = NULL,
    .pApplicationName = "renderer/vulkan",
    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
    .pEngineName = "soda",
    .engineVersion = VK_MAKE_VERSION(1, 0, 0),
    .apiVersion = VK_API_VERSION_1_0,
  };

  bool debug = true;
  VkInstanceCreateInfo createInfo = {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pNext = (debug) ? &DEBUG_UTILS_CREATE_INFO : NULL,
    .flags = 0,
    .pApplicationInfo = &appInfo,
    .enabledLayerCount = instance.requires.layers.count,
    .ppEnabledLayerNames = instance.requires.layers.data,
    .enabledExtensionCount = instance.requires.extensions.count,
    .ppEnabledExtensionNames = instance.requires.extensions.data
  };

  if (vkCreateInstance(&createInfo, NULL, &instance.instance) != VK_SUCCESS) Panic("renderer/vulkan: failed to create VkInstance\n");

  instance.debug_messenger = CreateDebugMessenger(instance.instance);

  return instance;
}

void DestroyInstance(Instance instance) {
  /* Deallocates an Instance */
  DestroyRequiredProperties(instance.requires.extensions);
  DestroyRequiredProperties(instance.requires.layers);
  DestroyDebugMessenger(instance);
  vkDestroyInstance(instance.instance, NULL);
}
