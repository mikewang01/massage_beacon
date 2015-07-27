/*
 * (C) Copyright 2001
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * Generic RTC interface.
 */
#ifndef _RTC_H_
#define _RTC_H_
#include "oop_hal.h"
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"

/*
 * The struct used to pass data from the generic interface code to
 * the hardware dependend low-level code ande vice versa. Identical
 * to struct rtc_time used by the Linux kernel.
 *
 * Note that there are small but significant differences to the
 * common "struct time":
 *
 * 		struct time:		struct rtc_time:
 * tm_mon	0 ... 11		1 ... 12
 * tm_year	years since 1900	years since 0
 */

struct rtc_time {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
};


DEF_CLASS(rtc_date)
	bool (*init)	(CLASS(rtc_date) *);
	bool (*de_init) (CLASS(rtc_date) *);
	/*convert from normal date to timestamp*/
	unsigned long (*make_beijing_time) (unsigned int year, unsigned int mon,
								unsigned int day, unsigned int hour,
								unsigned int min, unsigned int sec);
		/*convert from time stamp to normal time*/
	bool (*to_beijing_tm) (int, struct rtc_time *);
	/*convert from time stamp to normal time*/
	bool (*to_tm) (int, struct rtc_time *);
END_DEF_CLASS(rtc_date)


void rtc_get (struct rtc_time *);
void rtc_set (struct rtc_time *);
void rtc_reset (void);

void GregorianDay (struct rtc_time *);

#endif	/* _RTC_H_ */
