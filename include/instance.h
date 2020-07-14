#ifndef _SODA_INSTANCE_H
#define _SODA_INSTANCE_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <SDL.h>
#include <SDL_vulkan.h>

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
	/* Instance is wrapper for the VkInstance and its related state */
	VkInstance instance;
	VkDebugUtilsMessengerEXT debug_messenger;
	VkSurfaceKHR surface;

	struct {
		/* Namespace for the layers and extensions required by Vulkan */
	 RequiredProperties extensions, layers;
 	} requires;

	struct {
		/* Namespace for the layers and extensions supported by Vulkan */
		ExtensionProperties extensions;
		LayerProperties layers;
	} supports;

	Devices devices;

	struct {
		/* Namespace for SDL related data and attributes */
		SDL_Window *window;
	} sdl;

} Instance;

/* methods */

Instance CreateInstance();
void DestroyInstance(Instance *);

Instance CreateSDLInstance();
void DestroySDLInstance(Instance *);

#endif
