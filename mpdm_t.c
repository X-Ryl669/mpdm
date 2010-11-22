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

static mpdm_t t_new_val(int flags, void *ptr, int size)
{
	mpdm_t r = NULL;

	if (ptr != NULL) {
		char *ptr2 = malloc(size);
		memcpy(ptr2, ptr, size);

		r = mpdm_new(MPDM_FREE | flags, ptr2, size);
	}

	return r;
}


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
	mpdm_t r = NULL;
	char *ptr = NULL;
	int size = 0;

#ifdef CONFOPT_WIN32
	HANDLE h;

	h = CreateMutex(NULL, FALSE, NULL);

	if (h != NULL) {
		size = sizeof(h);
		ptr = (char *)&h;
	}
#endif

#ifdef CONFOPT_PTHREADS
    pthread_mutex_t m;

    if (pthread_mutex_init(&m, NULL) == 0) {
        size = sizeof(m);
        ptr = (char *)&m;
    }

#endif

	r = t_new_val(0, ptr, size);

    return r;
}


void mpdm_mutex_lock(mpdm_t mutex)
{
#ifdef CONFOPT_WIN32
	HANDLE *h = (HANDLE *)mutex->data;

	WaitForSingleObject(*h, INFINITE);
#endif

#ifdef CONFOPT_PTHREADS
    pthread_mutex_t *m = (pthread_mutex_t *)mutex->data;

    pthread_mutex_lock(m);
#endif
}


void mpdm_mutex_unlock(mpdm_t mutex)
{
#ifdef CONFOPT_WIN32
	HANDLE *h = (HANDLE *)mutex->data;

	ReleaseMutex(*h);
#endif

#ifdef CONFOPT_PTHREADS
    pthread_mutex_t *m = (pthread_mutex_t *)mutex->data;

    pthread_mutex_unlock(m);
#endif
}


/** semaphores **/

mpdm_t mpdm_new_sem(int init_value)
{
    char *ptr = NULL;
    int size = 0;

#ifdef CONFOPT_WIN32
    HANDLE h;

    if ((h = CreateSemaphore(NULL, init_value, 0x7fffffff, NULL)) != NULL) {
        size = sizeof(h);
        ptr = (char *)&h;
    }

#endif

    return t_new_val(0, ptr, size);
}


void mpdm_sem_wait(mpdm_t sem)
{
#ifdef CONFOPT_WIN32
	HANDLE *h = (HANDLE *)sem->data;

	WaitForSingleObject(*h, INFINITE);
#endif
}


void mpdm_sem_post(mpdm_t sem)
{
#ifdef CONFOPT_WIN32
	HANDLE *h = (HANDLE *)sem->data;

    ReleaseSemaphore(*h, 1, NULL);
#endif
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
	int size = 0;

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
		size = sizeof(t);
		ptr = (char *)&t;
	}

#endif

#ifdef CONFOPT_PTHREADS
    pthread_t pt;

    if (pthread_create(&pt, NULL, pthreads_thread, a) == 0) {
        size = sizeof(pthread_t);
        ptr = (char *)&pt;
    }

#endif

	r = t_new_val(0, ptr, size);

	return r;
}
