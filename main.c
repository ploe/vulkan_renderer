#include "instance.h"

Device *PickDevice(Devices);

int main(int argc, char *argv[]) {
    Instance instance = CreateSDLInstance();
    PickDevice(instance.devices);
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
