#pragma once

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

enum debug_level
{
    DEBUG_TRACE,
    DEBUG_INFO,
    DEBUG_WARNING,
    DEBUG_ERROR,
    DEBUG_FATAL
};

void debug_init();