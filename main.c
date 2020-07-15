#include "instance.h"

PhysicalDevice *PickPhysicalDevice(PhysicalDevices);

int main(int argc, char *argv[]) {
    Instance instance = CreateSDLInstance();
    PickPhysicalDevice(instance.physical_devices);
    while (1) {
        SDL_Event e;

    	while (SDL_PollEvent(&e)) {
		    switch(e.type) {
        		case SDL_QUIT:
				return 0;
				break;
		    }
	    }
    }

  DestroySDLInstance(&instance);

	return 0;
}
