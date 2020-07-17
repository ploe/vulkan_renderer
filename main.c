#include "instance.h"

int main(int argc, char *argv[]) {
    Instance instance = CreateSDLInstance();
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
