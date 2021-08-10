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
#define assert(expression, ...) ((expression)  \
									 ? (void)0 \
									 : (void)({ serial_write(##__VA_ARGS__); __asm__ __volatile__("int $0x01"); }))
#define assert_not_reached(...) ( {serial_write(##__VA_ARGS__); __asm__ __volatile__("int $0x01");} )