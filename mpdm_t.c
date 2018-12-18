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

    ts.tv_sec = msecs / 1000;
    ts.tv_nsec = (msecs % 1000) * 1000000;

    nanosleep(&ts, NULL);
#endif
}


/** mutexes **/

static void mutex_destroy(mpdm_ex_t ev)
{
#ifdef CONFOPT_WIN32
    HANDLE *h = (HANDLE *) ev->data;

    CloseHandle(*h);
#endif

#ifdef CONFOPT_PTHREADS
    pthread_mutex_t *m = (pthread_mutex_t *) ev->data;

    pthread_mutex_destroy(m);
#endif
}


/**
 * mpdm_new_mutex - Creates a new mutex.
 *
 * Creates a new mutex.
 * [Threading]
 */
mpdm_t mpdm_new_mutex(void)
{
    char *ptr = NULL;
    int size = 0;
    mpdm_ex_t ev;

#ifdef CONFOPT_WIN32
    HANDLE h;

    h = CreateMutex(NULL, FALSE, NULL);

    if (h != NULL) {
        size = sizeof(h);
        ptr = (char *) &h;
    }
#endif

#ifdef CONFOPT_PTHREADS
    pthread_mutex_t m;

    if (pthread_mutex_init(&m, NULL) == 0) {
        size = sizeof(m);
        ptr = (char *) &m;
    }

#endif

    ev = (mpdm_ex_t) MPDM_C(MPDM_MUTEX | MPDM_EXTENDED, ptr, size);
    ev->destroy = mutex_destroy;

    return (mpdm_t) ev;
}


/**
 * mpdm_mutex_lock - Locks a mutex.
 * @mutex: the mutex to be locked
 *
 * Locks a mutex. If the mutex is not already locked,
 * it waits until it is.
 * [Threading]
 */
void mpdm_mutex_lock(mpdm_t mutex)
{
#ifdef CONFOPT_WIN32
    HANDLE *h = (HANDLE *) mutex->data;

    WaitForSingleObject(*h, INFINITE);
#endif

#ifdef CONFOPT_PTHREADS
    pthread_mutex_t *m = (pthread_mutex_t *) mutex->data;

    pthread_mutex_lock(m);
#endif
}


/**
 * mpdm_mutex_unlock - Unlocks a mutex.
 * @mutex: the mutex to be unlocked
 *
 * Unlocks a previously locked mutex. The thread
 * unlocking the mutex must be the one who locked it.
 * [Threading]
 */
void mpdm_mutex_unlock(mpdm_t mutex)
{
#ifdef CONFOPT_WIN32
    HANDLE *h = (HANDLE *) mutex->data;

    ReleaseMutex(*h);
#endif

#ifdef CONFOPT_PTHREADS
    pthread_mutex_t *m = (pthread_mutex_t *) mutex->data;

    pthread_mutex_unlock(m);
#endif
}


/** semaphores **/

static void semaphore_destroy(mpdm_ex_t ev)
{
#ifdef CONFOPT_WIN32
    HANDLE *h = (HANDLE *) ev->data;

    CloseHandle(*h);
#endif

#ifdef CONFOPT_POSIXSEMS
    sem_t *s = (sem_t *) ev->data;

    sem_destroy(s);
#endif
}


/**
 * mpdm_new_semaphore - Creates a new semaphore.
 * @init_value: the initial value of the semaphore.
 *
 * Creates a new semaphore with an @init_value.
 * [Threading]
 */
mpdm_t mpdm_new_semaphore(int init_value)
{
    char *ptr = NULL;
    int size = 0;
    mpdm_ex_t ev;

#ifdef CONFOPT_WIN32
    HANDLE h;

    if ((h = CreateSemaphore(NULL, init_value, 0x7fffffff, NULL)) != NULL) {
        size = sizeof(h);
        ptr = (char *) &h;
    }

#endif

#ifdef CONFOPT_POSIXSEMS
    sem_t s;

    if (sem_init(&s, 0, init_value) == 0) {
        size = sizeof(s);
        ptr = (char *) &s;
    }

#endif

    ev = (mpdm_ex_t) MPDM_C(MPDM_SEMAPHORE | MPDM_EXTENDED, ptr, size);
    ev->destroy = semaphore_destroy;

    return (mpdm_t) ev;
}


/**
 * mpdm_semaphore_wait - Waits for a semaphore to be ready.
 * @sem: the semaphore to wait onto
 *
 * Waits for the value of a semaphore to be > 0. If it's
 * not, the thread waits until it is.
 * [Threading]
 */
void mpdm_semaphore_wait(mpdm_t sem)
{
#ifdef CONFOPT_WIN32
    HANDLE *h = (HANDLE *) sem->data;

    WaitForSingleObject(*h, INFINITE);
#endif

#ifdef CONFOPT_POSIXSEMS
    sem_t *s = (sem_t *) sem->data;

    sem_wait(s);
#endif
}


/**
 * mpdm_semaphore_post - Increments the value of a semaphore.
 * @sem: the semaphore to increment
 *
 * Increments by 1 the value of a semaphore.
 * [Threading]
 */
void mpdm_semaphore_post(mpdm_t sem)
{
#ifdef CONFOPT_WIN32
    HANDLE *h = (HANDLE *) sem->data;

    ReleaseSemaphore(*h, 1, NULL);
#endif

#ifdef CONFOPT_POSIXSEMS
    sem_t *s = (sem_t *) sem->data;

    sem_post(s);
#endif
}


/** threads **/

static void thread_caller(mpdm_t a)
{
    /* ignore return value */
    mpdm_void(mpdm_exec
              (mpdm_aget(a, 0), mpdm_aget(a, 1), mpdm_aget(a, 2)));

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

/**
 * mpdm_exec_thread - Runs an executable value in a new thread.
 * @c: the executable value
 * @args: executable arguments
 * @ctxt: the context
 *
 * Runs the @c executable value in a new thread. The code
 * starts executing immediately. The @args and @ctxt arguments
 * are sent to the executable value as arguments.
 *
 * Returns a handle for the thread.
 * [Threading]
 */
mpdm_t mpdm_exec_thread(mpdm_t c, mpdm_t args, mpdm_t ctxt)
{
    mpdm_t a;
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
        ptr = (char *) &t;
    }

#endif

#ifdef CONFOPT_PTHREADS
    pthread_t pt;

    if (pthread_create(&pt, NULL, pthreads_thread, a) == 0) {
        size = sizeof(pthread_t);
        ptr = (char *) &pt;
    }

#endif

    return MPDM_C(MPDM_THREAD, ptr, size);
}


void mpdm_new_channel(mpdm_t *parent, mpdm_t *child)
{
    mpdm_t p = mpdm_ref(MPDM_A(6));
    mpdm_t c = mpdm_ref(MPDM_A(6));
    mpdm_t v;

    v = mpdm_new_semaphore(0);
    mpdm_aset(p, v, 0);
    mpdm_aset(c, v, 3);

    v = mpdm_new_mutex();
    mpdm_aset(p, v, 1);
    mpdm_aset(c, v, 4);

    v = MPDM_A(0);
    mpdm_aset(p, v, 2);
    mpdm_aset(c, v, 5);

    v = mpdm_new_semaphore(0);
    mpdm_aset(c, v, 0);
    mpdm_aset(p, v, 3);

    v = mpdm_new_mutex();
    mpdm_aset(c, v, 1);
    mpdm_aset(p, v, 4);

    v = MPDM_A(0);
    mpdm_aset(c, v, 2);
    mpdm_aset(p, v, 5);

    p->flags |= (MPDM_FILE | MPDM_CHANNEL);
    c->flags |= (MPDM_FILE | MPDM_CHANNEL);

    mpdm_unrefnd(p);
    mpdm_unrefnd(c);

    *parent = p;
    *child  = c;
}


mpdm_t mpdm_channel_read(mpdm_t channel)
{
    mpdm_t s, m, l, r = NULL;

    mpdm_ref(channel);

    s = mpdm_aget(channel, 0);
    m = mpdm_aget(channel, 1);
    l = mpdm_aget(channel, 2);

    if (s->ref > 1)
        mpdm_semaphore_wait(s);

    mpdm_mutex_lock(m);
    r = mpdm_shift(l);
    mpdm_mutex_unlock(m);

    mpdm_unref(channel);

    return r;
}


void mpdm_channel_write(mpdm_t channel, mpdm_t v)
{
    mpdm_t s, m, l;

    mpdm_ref(channel);

    s = mpdm_aget(channel, 3);
    m = mpdm_aget(channel, 4);
    l = mpdm_aget(channel, 5);

    mpdm_mutex_lock(m);
    mpdm_push(l, v);
    mpdm_mutex_unlock(m);
    mpdm_semaphore_post(s);

    mpdm_unref(channel);
}
