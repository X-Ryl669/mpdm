/*

    MPDM - Minimum Profit Data Manager
    Copyright (C) 2003/2019 Angel Ortega <angel@triptico.com>

    mpdm_x.c - Extended functions

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

    http://triptico.com

*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#include "mpdm.h"

/**
 * mpdm_exec - Executes an executable value.
 * @c: the code value
 * @args: the arguments
 * @ctxt: the context
 *
 * Executes an executable value. If @c is a scalar value, its data
 * should be a pointer to a directly executable C function with a
 * prototype of mpdm_t func(mpdm_t args, mpdm_t ctxt); if it's a multiple
 * one, the first value's data should be a pointer to a directly executable
 * C function with a prototype of
 * mpdm_t func(mpdm_t b, mpdm_t args, mpdm_t ctxt) and
 * the second value will be passed as the @b argument. This value is used
 * to store bytecode or so when implementing virtual machines or compilers.
 * The @ctxt is meant to be used as a special context to implement local
 * symbol tables and such. Its meaning is free and can be NULL.
 *
 * Returns the return value of the code. If @c is NULL or not executable,
 * returns NULL.
 * [Value Management]
 */
mpdm_t mpdm_exec(mpdm_t c, mpdm_t args, mpdm_t ctxt)
{
    mpdm_t r = NULL;

    mpdm_ref(c);
    mpdm_ref(args);
    mpdm_ref(ctxt);

    if (c != NULL && (c->flags & MPDM_EXEC)) {
        if (c->flags & MPDM_MULTIPLE) {
            mpdm_t x;
            mpdm_t(*func) (mpdm_t, mpdm_t, mpdm_t);

            /* value is multiple; first element is the
               3 argument version of the executable function,
               next its optional additional information,
               the arguments and the context */
            x = mpdm_aget(c, 0);

            if ((func = (mpdm_t(*)(mpdm_t, mpdm_t, mpdm_t)) (x->data)) != NULL)
                r = func(mpdm_aget(c, 1), args, ctxt);
        }
        else {
            mpdm_t(*func) (mpdm_t, mpdm_t);

            /* value is scalar; c is the 2 argument
               version of the executable function */
            if ((func = (mpdm_t(*)(mpdm_t, mpdm_t)) (c->data)) != NULL)
                r = func(args, ctxt);
        }
    }

    mpdm_ref(r);

    mpdm_unref(ctxt);
    mpdm_unref(args);
    mpdm_unref(c);

    return mpdm_unrefnd(r);
}


mpdm_t mpdm_exec_1(mpdm_t c, mpdm_t a1, mpdm_t ctxt)
{
    mpdm_t a = MPDM_A(1);

    mpdm_aset(a, a1, 0);

    return mpdm_exec(c, a, ctxt);
}


mpdm_t mpdm_exec_2(mpdm_t c, mpdm_t a1, mpdm_t a2, mpdm_t ctxt)
{
    mpdm_t a = MPDM_A(2);

    mpdm_aset(a, a1, 0);
    mpdm_aset(a, a2, 1);

    return mpdm_exec(c, a, ctxt);
}


mpdm_t mpdm_exec_3(mpdm_t c, mpdm_t a1, mpdm_t a2, mpdm_t a3, mpdm_t ctxt)
{
    mpdm_t a = MPDM_A(3);

    mpdm_aset(a, a1, 0);
    mpdm_aset(a, a2, 1);
    mpdm_aset(a, a3, 2);

    return mpdm_exec(c, a, ctxt);
}


/**
 * mpdm_iterator - Iterates through the content of an object.
 * @o: the object
 * @context: A pointer to an opaque context
 * @k: a pointer to a value to store the key
 * @v: a pointer to a value to store the value
 *
 * Iterates through the @o value. If it's a hash, every key/value pair
 * is returned on each call. If it's an array, @v contains the
 * element and @k the index number on each call. Otherwise, it's assumed
 * to be a string containing a numeral and @k and @v are filled with
 * values from 0 to @o - 1 on each call.
 *
 * Any of @k and @v pointers can be NULL if the value is not of interest.
 *
 * The @context pointer to integer is opaque and should be
 * initialized to zero on the first call.
 *
 * Returns 0 if no more data is left in @h.
 * [Hashes]
 * [Arrays]
 */
int mpdm_iterator(mpdm_t o, int *context, mpdm_t *k, mpdm_t *v)
{
    int ret = 0;

    if (MPDM_IS_HASH(o)) {
        int bi, ei;

        if (mpdm_size(o)) {
            /* get bucket and element index */
            bi = (*context) % mpdm_size(o);
            ei = (*context) / mpdm_size(o);

            while (ret == 0 && bi < mpdm_size(o)) {
                mpdm_t b;

                /* if bucket is empty or there are no more
                   elements in it, pick the next one */
                if (!(b = mpdm_aget(o, bi)) || ei >= mpdm_size(b)) {
                    ei = 0;
                    bi++;
                }
                else {
                    /* get pair */
                    if (k) *k = mpdm_aget(b, ei);
                    if (v) *v = mpdm_aget(b, ei + 1);

                    ei += 2;

                    /* update context */
                    *context = (ei * mpdm_size(o)) + bi;
                    ret = 1;
                }
            }
        }
    }
    else
    if (MPDM_IS_ARRAY(o)) {
        if (*context < mpdm_size(o)) {
            if (k) *k = MPDM_I(*context);
            if (v) *v = mpdm_aget(o, (*context));

            (*context)++;
            ret = 1;
        }
    }
    else
    if (MPDM_IS_FILE(o)) {
        mpdm_t w = mpdm_read(o);

        if (w != NULL) {
            if (k) *k = MPDM_I(*context);
            if (v) *v = w;

            if (!v) mpdm_void(w);

            (*context)++;
            ret = 1;
        }
    }
    else {
        /* assume it's a number */
        if (*context < mpdm_ival(o)) {
            if (k) *k = MPDM_I(*context);
            if (v) *v = MPDM_I(*context);

            (*context)++;
            ret = 1;
        }
    }

    return ret;
}


mpdm_t mpdm_map(mpdm_t set, mpdm_t filter, mpdm_t ctxt)
{
    mpdm_t out = NULL;
    mpdm_t k, v;

    if (set != NULL) {
        int n = 0;
        out = MPDM_A(0);

        while (mpdm_iterator(set, &n, &k, &v)) {
            mpdm_t w = NULL;
            mpdm_ref(k);
            mpdm_ref(v);

            if (MPDM_IS_EXEC(filter)) {
                if (MPDM_IS_HASH(set))
                    w = mpdm_exec_2(filter, k, v, ctxt);
                else
                    w = mpdm_exec_2(filter, v, k, ctxt);
            }
            else
            if (MPDM_IS_HASH(filter))
                w = mpdm_hget(filter, v);
            else
            if (MPDM_IS_STRING(filter))
                w = mpdm_regex(v, filter, 0);
            else
                w = v;

            mpdm_push(out, w);

            mpdm_unref(v);
            mpdm_unref(k);
        }
    }

    return out;
}
