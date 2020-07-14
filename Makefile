.PHONY: all

all: soda

clean:
	rm -v soda

soda: devices.c main.c instance.c panic.c
	cc -o soda $^ -I./include `pkg-config --cflags --static --libs sdl2` -I${VULKAN_SDK}/include  -L${VULKAN_SDK}/lib -lvulkan
