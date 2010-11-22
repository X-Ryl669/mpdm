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

#ifdef CONFOPT_PTHREADS
#include <pthread.h>
#endif

#ifdef CONFOPT_POSIXSEMS
#include <semaphore.h>
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


/** mutexes **/

mpdm_t mpdm_new_mutex(void)
{
    return NULL;
}


void mpdm_mutex_lock(mpdm_t mutex)
{
}


void mpdm_mutex_unlock(mpdm_t mutex)
{
}


/** semaphores **/

mpdm_t mpdm_new_sem(void)
{
    return NULL;
}


void mpdm_sem_wait(mpdm_t sem)
{
}


void mpdm_sem_post(mpdm_t sem)
{
}


/** threads **/

static void thread_caller(mpdm_t a)
{
	/* ignore return value */
	mpdm_unref(
        mpdm_ref(
            mpdm_exec(mpdm_aget(a, 0), mpdm_aget(a, 1), mpdm_aget(a, 2))
        )
    );

	/* was referenced in mpdm_exec_thread() */
	mpdm_unref(a);
}


#ifdef CONFOPT_WIN32
DWORD WINAPI win32_thread(LPVOID param)
{
    thread_caller((mpdm_t) param);

	return 0;
}
#endif

#ifdef CONFOPT_PTHREADS
void *pthreads_thread(void *args)
{
    thread_caller((mpdm_t) args);

    return NULL;
}
#endif

mpdm_t mpdm_exec_thread(mpdm_t c, mpdm_t args, mpdm_t ctxt)
{
	mpdm_t a;
	mpdm_t r = NULL;
	char *ptr = NULL;
	int size;

	if (ctxt == NULL)
		ctxt = MPDM_A(0);

	/* to be unreferenced at thread stop */
	a = mpdm_ref(MPDM_A(3));

	mpdm_aset(a, c, 0);
	mpdm_aset(a, args, 1);
	mpdm_aset(a, ctxt, 2);

#ifdef CONFOPT_WIN32
	HANDLE t;

	t = CreateThread(NULL, 0, win32_thread, a, 0, NULL);

	if (t != NULL) {
		size = sizeof(HANDLE);
		ptr = malloc(size);
		memcpy(ptr, &t, size);
	}

#endif

#ifdef CONFOPT_PTHREADS
    pthread_t pt;

    if (pthread_create(&pt, NULL, pthreads_thread, a) == 0) {
        size = sizeof(pthread_t);
        ptr = malloc(size);
        memcpy(ptr, &pt, size);
    }

#endif

	if (ptr != NULL)
		r = mpdm_new(MPDM_FREE, ptr, size);

	return r;
}
