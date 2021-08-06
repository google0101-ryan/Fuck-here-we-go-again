#include "debug.h"
#include "../devices/char/tty.h"
#include <stdbool.h>

#define SERIAL_PORT_A 0x3f8

static char *DEBUG_NAME[] = {
	[DEBUG_TRACE] = "TRACE",
	[DEBUG_INFO] = "INFO",
	[DEBUG_WARNING] = "WARNING",
	[DEBUG_ERROR] = "ERROR",
	[DEBUG_FATAL] = "FATAL",
};

void debug_init()
{
    serial_enable(SERIAL_PORT_A);
}