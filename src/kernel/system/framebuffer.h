#ifndef SYSTEM_FRAMEBUFFER_H
#define SYSTEM_FRAMEBUFFER_H

#include <multiboot2.h>
#include <stdarg.h>
#include <stdint.h>

struct framebuffer
{
	uint32_t addr;
	uint32_t pitch;
	uint32_t width;
	uint32_t height;
	uint8_t bpp;
};

void framebuffer_init(struct multiboot_tag_framebuffer *);
struct framebuffer *get_framebuffer();

#endif