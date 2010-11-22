/*

    MPDM - Minimum Profit Data Manager
    Copyright (C) 2003/2010 Angel Ortega <angel@triptico.com>

    mpdm_t.c - Threading primitives

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

    http://www.triptico.com

*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <time.h>

#ifdef CONFOPT_WIN32
#include <windows.h>
#endif

#include "mpdm.h"


/** code **/

/**
 * mpdm_sleep - Sleeps a number of milliseconds.
 * @msecs: the milliseconds to sleep
 *
 * Sleeps a number of milliseconds.
 * [Threading]
 */
void mpdm_sleep(int msecs)
{
#ifdef CONFOPT_WIN32

    Sleep(msecs);

#endif

#ifdef CONFOPT_NANOSLEEP
    struct timespec ts;

    ts.tv_sec   = msecs / 1000;
    ts.tv_nsec  = (msecs % 1000) * 1000000;

    nanosleep(&ts, NULL);
#endif
}
