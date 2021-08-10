#include "rtc.h"

#include <cpu/hal.h>
#include <cpu/idt.h>
#include <cpu/pic.h>
#include <utils/debug.h>
#include <utils/math.h>

#define CMOS_ADDRESS 0x70
#define CMOS_DATA 0x71
#define RTC_TICKS_PER_SECOND 32

static volatile uint32_t current_ticks = 0;

static uint8_t rtc_get_update_flag()
{
	outportb(CMOS_ADDRESS, 0x0A);
	return inportb(CMOS_DATA) & 0x80;
}

static uint8_t rtc_get_register(uint32_t reg)
{
	outportb(CMOS_ADDRESS, reg);
	return inportb(CMOS_DATA);
}

void rtc_get_datetime(uint16_t *year, uint8_t *month, uint8_t *day,
					  uint8_t *hour, uint8_t *minute, uint8_t *second)
{
	uint8_t century;
	uint8_t last_second, last_minute, last_hour, last_day, last_month, last_year, last_century;

	while (rtc_get_update_flag())
		;
	*second = rtc_get_register(0x00);
	*minute = rtc_get_register(0x02);
	*hour = rtc_get_register(0x04);
	*day = rtc_get_register(0x07);
	*month = rtc_get_register(0x08);
	*year = rtc_get_register(0x09);
	century = rtc_get_register(0x32);

	do
	{
		last_second = *second;
		last_minute = *minute;
		last_hour = *hour;
		last_day = *day;
		last_month = *month;
		last_year = *year;
		last_century = century;

		while (rtc_get_update_flag())
			;
		*second = rtc_get_register(0x00);
		*minute = rtc_get_register(0x02);
		*hour = rtc_get_register(0x04);
		*day = rtc_get_register(0x07);
		*month = rtc_get_register(0x08);
		*year = rtc_get_register(0x09);
		century = rtc_get_register(0x32);

	} while ((last_second != *second) || (last_minute != *minute) || (last_hour != *hour) ||
			 (last_day != *day) || (last_month != *month) || (last_year != *year) ||
			 (last_century != century));

	uint8_t registerB = rtc_get_register(0x0B);
	// Convert BCD to binary values if necessary
	if (!(registerB & 0x04))
	{
		*second = (*second & 0x0F) + ((*second / 16) * 10);
		*minute = (*minute & 0x0F) + ((*minute / 16) * 10);
		*hour = ((*hour & 0x0F) + (((*hour & 0x70) / 16) * 10)) | (*hour & 0x80);
		*day = (*day & 0x0F) + ((*day / 16) * 10);
		*month = (*month & 0x0F) + ((*month / 16) * 10);
		*year = (*year & 0x0F) + ((*year / 16) * 10);
		century = (century & 0x0F) + ((century / 16) * 10);
	}
	*year = century * 100 + *year;

	// Convert 12 hour clock to 24 hour clock if necessary
	if (!(registerB & 0x02) && (*hour & 0x80))
	{
		*hour = ((*hour & 0x7F) + 12) % 24;
	}
}

static int32_t rtc_irq_handler(struct interrupt_registers *regs)
{
	current_ticks++;

	if (current_ticks % (RTC_TICKS_PER_SECOND / 4) == 0)
	{
		uint16_t year;
		uint8_t second, minute, hour, day, month;

		rtc_get_datetime(&year, &month, &day, &hour, &minute, &second);
		set_current_time(year, month, day, hour, minute, second);
	}

	outportb(0x70, 0x0C);  // select register C
	inportb(0x71);		   // just throw away contents

	irq_ack(regs->int_no);
	return IRQ_HANDLER_CONTINUE;
}

void rtc_init()
{
	serial_write("RTC: Initializing\n");

	uint8_t rate = log2(32768 / RTC_TICKS_PER_SECOND) + 1;
	disable_interrupts();
	outportb(0x70, 0x8A);		  // select Status Register A, and disable NMI (by setting the 0x80 bit)
	outportb(0x71, 0x20 | rate);  // write to CMOS/RTC RAM and interrupt rate

	// turn on irq8
	outportb(0x70, 0x8B);		  // select register B, and disable NMI
	char prev = inportb(0x71);	  // read the current value of register B
	outportb(0x70, 0x8B);		  // set the index again (a read will reset the index to register D)
	outportb(0x71, prev | 0x40);  // write the previous value ORed with 0x40. This turns on bit 6 of register B
	register_interrupt_handler(IRQ8, rtc_irq_handler);
	pic_clear_mask(IRQ8);

	serial_write("RTC: Done\n");
}

extern volatile uint32_t jiffies;

volatile uint32_t boot_seconds, current_seconds;
struct time epoch_time = {
	.year = 1970,
	.month = 1,
	.day = 1,
	.hour = 0,
	.minute = 0,
	.second = 0,
};
struct time current_time;

void set_boot_seconds(uint64_t bs)
{
	boot_seconds = bs;
}

void set_current_time(uint16_t year, uint8_t month, uint8_t day,
					  uint8_t hour, uint8_t minute, uint8_t second)
{
	current_time.year = year;
	current_time.month = month;
	current_time.day = day;
	current_time.hour = hour;
	current_time.minute = minute;
	current_time.second = second;
	current_seconds = get_seconds(NULL);
}

// NOTE: MQ 2019-07-25 According to this paper http://howardhinnant.github.io/date_algorithms.html#civil_from_days
static struct time *get_time_from_seconds(int32_t seconds)
{
	struct time *t = kcalloc(1, sizeof(struct time));
	int32_t days = seconds / (24 * 3600);

	days += 719468;
	uint32_t era = (days >= 0 ? days : days - 146096) / 146097;
	uint32_t doe = days - era * 146097;									   // [0, 146096]
	uint32_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;  // [0, 399]
	uint32_t y = yoe + era * 400;
	uint32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);	 // [0, 365]
	uint32_t mp = (5 * doy + 2) / 153;						 // [0, 11]
	uint32_t d = doy - (153 * mp + 2) / 5 + 1;				 // [1, 31]
	uint32_t m = mp + (mp < 10 ? 3 : -9);					 // [1, 12]

	t->year = y + (m <= 2);
	t->month = m;
	t->day = d;
	t->hour = (seconds % (24 * 3600)) / 3600;
	t->minute = (seconds % (60 * 60)) / 60;
	t->second = seconds % 60;

	return t;
}

// NOTE: MQ 2019-07-25 According to this paper http://howardhinnant.github.io/date_algorithms.html#days_from_civil
static uint32_t get_days(struct time *t)
{
	int32_t year = t->year;
	uint32_t month = t->month;
	uint32_t day = t->day;

	year -= month <= 2;

	uint32_t era = (year >= 0 ? year : year - 399) / 400;
	uint32_t yoe = (year - era * 400);										  // [0, 399]
	uint32_t doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;  // [0, 365]
	uint32_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;					  // [0, 146096]
	return era * 146097 + (doe)-719468;
}

uint32_t get_seconds(struct time *t)
{
	if (t == NULL)
		t = &current_time;

	return get_days(t) * 24 * 3600 + t->hour * 3600 + t->minute * 60 + t->second;
}

uint64_t get_milliseconds(struct time *t)
{
	if (t == NULL)
		return boot_seconds * 1000 + jiffies;
	else
		return get_seconds(t) * 1000 + jiffies % 1000;
}

struct time *get_time(int32_t seconds)
{
	return seconds == 0 ? &current_time : get_time_from_seconds(seconds);
}

uint64_t get_milliseconds_since_epoch()
{
	return get_milliseconds(NULL) - get_milliseconds(&epoch_time);
}