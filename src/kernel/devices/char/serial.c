#include "tty.h"
#include "../../cpu/hal.h"

int cur_port;

void serial_output(int port, char a)
{
	while (serial_transmit_empty(port) == 0)
		;

	outportb(port, a);
}

int serial_write(const char *buf)
{
    for (int i = 0; i < strlen(buf); i++)
    {
        serial_output(cur_port, buf[i]);
    }
}

void serial_enable(int port)
{
    outportb(port + 1, 0x00);
    outportb(port + 3, 0x80);
    outportb(port + 0, 0x03);
    outportb(port + 1, 0x00);
    outportb(port + 3, 0x03);
    outportb(port + 2, 0xC7);
    outportb(port + 4, 0x0B);

    cur_port = port;
}

int serial_transmit_empty(int port)
{
	return inportb(port + 5) & 0x20;
}