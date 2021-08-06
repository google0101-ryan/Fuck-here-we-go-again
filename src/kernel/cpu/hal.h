#pragma once

#include <stddef.h>
#include <stdint.h>

static __inline void io_wait()
{
    __asm__ __volatile__("outb %%al, $0x80" :: "a"(0));
}

static __inline unsigned char inportb(unsigned short _port)
{
	unsigned char rv;
	asm volatile("inb %1, %0"
				 : "=a"(rv)
				 : "dN"(_port));
	return rv;
}

static __inline void outportb(unsigned short _port, unsigned char _data)
{
	asm volatile("outb %1, %0"
				 :
				 : "dN"(_port), "a"(_data));
}