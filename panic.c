#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

void Panic(const char *format, ...) {
	/* Prints an error message and then closes the app */
	va_list args;
	va_start(args, format);

	vfprintf(stderr, format, args);

	va_end(args);
	abort();
}
