#include <stdbool.h>

#include <SDL.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include "panic.h"

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

/* types */

typedef struct {
	/* Container for required Vulkan Instance Extensions */
	const char **names;
	uint32_t count;
} InstanceExtensions;

/* Container for required Vulkan Validation Layers */
typedef InstanceExtensions ValidationLayers;

typedef struct {
	/* Type that describes the settings required for an environment e.g. dev,
	prod,	etc. */
	InstanceExtensions instance_extensions;
	ValidationLayers validation_layers;
	struct {
		struct messenger {
			VkDebugUtilsMessengerCreateInfoEXT *create_info;
		} messenger;
	} debug_utils;
} Environment;

typedef struct {
  /* Type that keeps the device related attributes together */
	struct {
		VkDeviceCreateInfo info;
	} create;

  struct {
    /* Container for VkPhysicalDevice attributes */
    VkPhysicalDevice device;
		VkPhysicalDeviceProperties properties;
		VkPhysicalDeviceFeatures features;
  } physical;

	struct {
		/* Container for queue related attributes */
		float priorities;

		struct {
			/* Container for VkDeviceQueueCreateInfo attributes */
			uint32_t count;
			VkDeviceQueueCreateInfo *info;
		} create;

		struct {
			/* Container for VkQueueFamilyProperties attributes */
			int graphics, present;
			uint32_t count;
			VkQueueFamilyProperties *properties;
		} family;

	} queue;

} Device;

/* namespaces */

static struct sdl {
	/* sdl is a namespace containing SDL related variables */
	SDL_Window *window;
	InstanceExtensions instance_extensions;
} sdl;

static struct vk {
	/* vk is a namespace containing Vulkan related variables */
	VkInstance instance;
	VkSurfaceKHR surface;
	struct extension_properties {
		/* extension_properties is a namespace containing the VkExtensionProperties data */
		VkExtensionProperties *properties;
		uint32_t count;
	} extension_properties;

	/* instance_extensions is a namespace for the required Vulkan Instance
	Extensions */
	InstanceExtensions instance_extensions;

	struct {
		/* debug_utils is a namespace containing VkDebugUtils */
		VkDebugUtilsMessengerEXT *messenger;
	} debug_utils;

	struct {
		VkDevice device;
	} logical;

	struct {
		Device *devices;
		uint32_t count;
	} physical;

} vk;

/* constants */

static const char *DEV_INSTANCE_EXTENSIONS[] = {
	/* These are the required extensions when debug mode is enabled */
	"VK_EXT_debug_utils",
};

static const char *DEV_VALIDATION_LAYERS[] = {
	/* These are the required layers when debug mode is enabled */
	"VK_LAYER_KHRONOS_validation"
};

static VKAPI_ATTR VkBool32 VKAPI_CALL devDebugUtilsMessenger();

VkDebugUtilsMessengerCreateInfoEXT DEBUG_UTILS_MESSENGER_CREATE_INFO = {
	.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
	.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
	.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
	.pfnUserCallback = devDebugUtilsMessenger,
};

static Environment dev = {
	/* The dev Environment includes layers and extensions required for debugging
	Vulkan */
	.instance_extensions = {
		.names = DEV_INSTANCE_EXTENSIONS,
		.count = ARRAY_SIZE(DEV_INSTANCE_EXTENSIONS),
	},
	.validation_layers = {
		.names = DEV_VALIDATION_LAYERS,
		.count = ARRAY_SIZE(DEV_VALIDATION_LAYERS),
	},

	.debug_utils.messenger.create_info = &DEBUG_UTILS_MESSENGER_CREATE_INFO,
};

/* The prod Environment excludes layers and extensions for debugging Vulkan */
static Environment prod;

/* methods */

static VKAPI_ATTR VkBool32 VKAPI_CALL devDebugUtilsMessenger(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
	void *pUserData
) {
	/* Prints the pCallbackData->pMessage */
	fprintf(stderr, "%s\n", pCallbackData->pMessage);

	return VK_FALSE;
}

static struct sdl initSDL() {
	/* Setup the sdl namespace */
	if(SDL_Init(SDL_INIT_VIDEO))
		Panic("initSDL: failed to SDL_Init");

	SDL_Window *window = SDL_CreateWindow("soda: SDL/Vulkan", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);

	uint32_t count = 0;
	SDL_Vulkan_GetInstanceExtensions(window, &count, NULL);

	const char **names = calloc(count, sizeof(const char *));
	if (!names)
		Panic("instance/InitSDL: failed to allocate data for SDL_Vulkan_GetInstanceExtensions");

	SDL_Vulkan_GetInstanceExtensions(window, &count, names);

	return (struct sdl) {
		.instance_extensions = {
				.count = count,
				.names = names,
		},
		.window = window,
	};
}

static void destroySDL() {
	/* Free and unset the SDL Namespace */
	free(sdl.instance_extensions.names);

	SDL_DestroyWindow(sdl.window);

	sdl = (struct sdl) {};
}

static VkExtensionProperties *getVkExtensionProperties(uint32_t count) {
	/* Set the supported VkExtensionProperties in vk.extension namespace */
	VkExtensionProperties *properties = calloc(count, sizeof(VkExtensionProperties));
	if (!properties) Panic("getExtensionsProperties: unable to allocate VkExtensionProperties");

	vkEnumerateInstanceExtensionProperties(NULL, &count, properties);

	return properties;
}

static uint32_t countVkExtensionProperties() {
	/* Count the supported VkExtensionProperties in vk.extension namespace */
	uint32_t count = 0;
	vkEnumerateInstanceExtensionProperties(NULL, &count, NULL);

	return count;
}

static void destroyVkExtensionProperties () {
	/* Free and unset the VkExtensionProperties in vk.extension namespace */
	if (vk.extension_properties.properties) free(vk.extension_properties.properties);
	vk.extension_properties.count = 0;
	vk.extension_properties.properties = NULL;
}

static inline uint32_t totalSources(InstanceExtensions *sources, int target) {
	/* Iterate over the InstanceExtensions sources and return the total of their
	counts */
	uint32_t count = 0;
	int i;

	for (i = 0; i < target; i++)
		count += sources[i].count;

	return count;
}

static bool validInstanceExtension(const char *name) {
	/* Checks the name of an Instance Extension, returns true if it's valid and
	false if it's not */
	uint32_t i;
	for (i = 0; i < vk.extension_properties.count; i++) {
		VkExtensionProperties properties = vk.extension_properties.properties[i];
		if (strcmp(name, properties.extensionName) == 0) return true;
	}

	return false;
}

static InstanceExtensions setVkInstanceExtensions(Environment *environment) {
	/* Validates and sets vk.instance_extensions namespace */
	InstanceExtensions sources[] = {
		environment->instance_extensions,
		sdl.instance_extensions,
	};

	uint32_t count = totalSources(sources, ARRAY_SIZE(sources));

	const char **names = calloc(count, sizeof(const char *));
	if(!names) Panic("setVkInstanceExtensions: unable to allocate names\n");

	const char **dst = names;
	uint32_t i;
	for (i = 0; i < ARRAY_SIZE(sources); i++) {
		InstanceExtensions *source = &(sources[i]);

		size_t size = source->count * sizeof(const char *);
		memcpy(dst, source->names, size);

		dst += source->count;
	}

	for (i = 0; i < count; i++) {
		const char *name = names[i];
		if (validInstanceExtension(names[i])) continue;

		Panic("setVkInstanceExtensions: Invalid Instance Extension requested: %s\n", name);
	}

	return (InstanceExtensions) {
		.count = count,
		.names = names,
	};
}

static VkDebugUtilsMessengerEXT *createDebugUtilsMessenger(VkDebugUtilsMessengerCreateInfoEXT *create_info) {
	/* Initialise and return VkDebugUtilsMessengerEXT if create_info is set */
	if (!create_info) return NULL;

	PFN_vkCreateDebugUtilsMessengerEXT create =
		(PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vk.instance, "vkCreateDebugUtilsMessengerEXT");

	if (!create)
		Panic("initDebugUtilsMessenger: unable to get vkCreateDebugUtilsMessengerEXT\n");

	VkDebugUtilsMessengerEXT *messenger = calloc(1, sizeof(VkDebugUtilsMessengerEXT));
	if (!messenger)
		Panic("initDebugUtilsMessenger: unable to allocate 'messenger'\n");

	create(vk.instance, create_info, NULL, messenger);

	return messenger;
}

static VkDebugUtilsMessengerEXT *destroyDebugUtilsMessenger(VkDebugUtilsMessengerEXT *messenger) {
	/* Free and unset messenger */
	PFN_vkDestroyDebugUtilsMessengerEXT destroy =
		(PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vk.instance, "vkDestroyDebugUtilsMessengerEXT");

	if (!destroy)
		Panic("initDebugUtilsMessenger: unable to get vkDestroyDebugUtilsMessengerEXT\n");

	destroy(vk.instance, *messenger, NULL);
	free(messenger);

	return NULL;
}

static uint32_t countVkPhysicalDevices() {
  /* Counts the VkPhysicalDevice */
  uint32_t count = 0;
  vkEnumeratePhysicalDevices(vk.instance, &count, NULL);

  return count;
}

static VkPhysicalDevice *getVkPhysicalDevices(uint32_t count) {
  /* Allocate and return VkPhysicalDevice array */
  VkPhysicalDevice *devices = calloc(count, sizeof(VkPhysicalDevice));

  if (!devices)
    Panic("getVkPhysicalDevices: unable to allocate VkPhysicalDevice array\n");

  vkEnumeratePhysicalDevices(vk.instance, &count, devices);

  return devices;
}

static uint32_t countQueueFamilyProperties(VkPhysicalDevice physical_device) {
  /* Counts the VkPhysicalDeviceQueueFamilyProperties */
  uint32_t count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, NULL);

  return count;
}

static VkQueueFamilyProperties *getQueueFamilyProperties(VkPhysicalDevice physical_device, uint32_t count) {
  /* Allocate and return VkQueueFamilyProperties array */
  VkQueueFamilyProperties *properties = calloc(count, sizeof(VkQueueFamilyProperties));

  if (!properties)
    Panic("getQueueFamiliesProperties: unable to allocate VkQueueFamilyProperties array\n");

  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, properties);

  return properties;
}

/* 0 is a valid VkQueueFamilyProperties index so NO_QUEUE_FAMILY is -1 */
static const int NO_QUEUE_FAMILY = -1;

/* SET_QUEUE_FAMILY is a ternary expression that sets a queue_family only if it
hasn't been set yet */
#define SET_QUEUE_FAMILY(target, value) target = (target == NO_QUEUE_FAMILY) ? value : target

static void setQueueFamilies(Device *device) {
	/* setQueueFamilies will set the queue_family for device to the first index it
	finds. */
	int i;

	for (i = 0; i < device->queue.family.count; i++) {
		VkQueueFamilyProperties *properties = &(device->queue.family.properties[i]);

		if (properties->queueFlags & VK_QUEUE_GRAPHICS_BIT)
			SET_QUEUE_FAMILY(device->queue.family.graphics, i);

		VkBool32 can_present = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(device->physical.device, i, vk.surface, &can_present);

		if (can_present)
			SET_QUEUE_FAMILY(device->queue.family.present, i);
	}
}

static void setQueueCreateInfo(Device *device) {

	device->queue.create.info = calloc(device->queue.family.count, sizeof(VkDeviceQueueCreateInfo));
	if (!device->queue.create.info)
		Panic("setQueueCreateInfo: unable to allocate VkDeviceQueueCreateInfo array for '%s'\n", device->physical.properties.deviceName);

	bool *created = calloc(device->queue.family.count, sizeof(bool));
	if (!created)
		Panic("setQueueCreateInfo: unable to allocate created array\n");

	int queue_family[] = {
		device->queue.family.graphics,
		device->queue.family.present,
	};

	int i, count = 0;
	for (i = 0; i < ARRAY_SIZE(queue_family); i++) {
		int family_index = queue_family[i];

		if (created[family_index]) continue;

		device->queue.create.info[count] = (VkDeviceQueueCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = family_index,
			.queueCount = 1,
			.pQueuePriorities = &(device->queue.priorities),
		};

		count += 1;
		created[family_index] = true;
	}

	device->queue.create.count = count;

	printf("device->queue.create.count: %d\n", device->queue.create.count);
	free(created);
}

static Device *createDevices(uint32_t count) {
	/* Allocate and return an array of Devices. */

	Device *devices = calloc(count, sizeof(Device));
	if (!devices)
		Panic("createDevices: unable to allocate Device array\n");

	VkPhysicalDevice *physical_devices = getVkPhysicalDevices(count);

	int i;
	for (i = 0; i < count; i++) {
		Device *device = &(devices[i]);
		VkPhysicalDevice physical_device = physical_devices[i];

		device->physical.device = physical_device;
		vkGetPhysicalDeviceProperties(physical_device, &(device->physical.properties));
		vkGetPhysicalDeviceFeatures(physical_device, &(device->physical.features));

		device->queue.priorities = 1.0f;
		device->queue.family.count = countQueueFamilyProperties(physical_device);
		device->queue.family.properties = getQueueFamilyProperties(physical_device, device->queue.family.count);

		device->queue.family.graphics =
		device->queue.family.present =
			NO_QUEUE_FAMILY;

		setQueueFamilies(device);
		setQueueCreateInfo(device);

		device->create.info = (VkDeviceCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.queueCreateInfoCount = device->queue.create.count,
			.pQueueCreateInfos = device->queue.create.info,
		};
	}

	free(physical_devices);

	return devices;
}

static VkDevice createLogicalDevice(Device *device) {
	VkPhysicalDeviceFeatures features = {0};
	device->create.info.pEnabledFeatures = &features;


	VkDevice logical_device;
	if (vkCreateDevice(device->physical.device, &device->create.info, NULL, &logical_device) == VK_SUCCESS)
		return logical_device;

	Panic("createLogicalDevice: unable to create logical device\n");

	return 0;
}

void CreateRenderer() {
	/* Creates the VkInstance and get the rest of the Vulkan's state */

	Environment *environment = &dev;

	sdl = initSDL();

	vk.extension_properties.count = countVkExtensionProperties();
	vk.extension_properties.properties = getVkExtensionProperties(vk.extension_properties.count);

	vk.instance_extensions = setVkInstanceExtensions(environment);

	VkApplicationInfo application_info = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "soda/vulkan",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "soda",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_2,
	};

	VkInstanceCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = environment->debug_utils.messenger.create_info,
		.pApplicationInfo = &application_info,
		.enabledLayerCount = environment->validation_layers.count,
		.ppEnabledLayerNames = environment->validation_layers.names,
		.enabledExtensionCount = vk.instance_extensions.count,
		.ppEnabledExtensionNames = vk.instance_extensions.names,
	};

	VkResult result = vkCreateInstance(&create_info, NULL, &vk.instance);

	if (result != VK_SUCCESS)
		Panic("CreateInstance: failed to create VkInstance\n");

	vk.debug_utils.messenger = createDebugUtilsMessenger(environment->debug_utils.messenger.create_info);

	SDL_Vulkan_CreateSurface(sdl.window, vk.instance, &vk.surface);

	vk.physical.count = countVkPhysicalDevices();
	vk.physical.devices = createDevices(vk.physical.count);

	vk.logical.device = createLogicalDevice(&vk.physical.devices[0]);

	//puts(vk.physical[0].physical.properties.deviceName);
}
