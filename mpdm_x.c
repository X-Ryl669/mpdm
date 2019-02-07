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
#include <wchar.h>

#include "mpdm.h"

/**
 * mpdm_is_true - Returns 1 if a value is true.
 * @v: the value
 *
 * Returns 1 if @v is true. False values are: NULL, those with
 * cached integer values of 0, those with cached real values
 * of 0.0, empty strings and the special string "0".
 * The reference count is touched.
 */
int mpdm_is_true(mpdm_t v)
{
    int r = 0;

    if (v) {
        mpdm_ref(v);

        if (mpdm_type(v) == MPDM_TYPE_SCALAR) {
            wchar_t *ptr = mpdm_string(v);

            /* ... and it's "" or "0", it's false */
            r = !(*ptr == L'\0' || wcscmp(ptr, L"0") == 0);
        }
        else
            r = 1;

        mpdm_unref(v);
    }

    return r;
}


/**
 * mpdm_bool - Returns a boolean value.
 * @b: true or false
 *
 * Returns the stored values for TRUE or FALSE.
 */
mpdm_t mpdm_bool(int b)
{
    return mpdm_hget_s(mpdm_root(), b ? L"TRUE" : L"FALSE");
}


mpdm_t mpdm_new_x(mpdm_type_t type, void *f, mpdm_t a)
{
    mpdm_t r = NULL;

    switch (type) {
    case MPDM_TYPE_FUNCTION:
        r = mpdm_new(type | MPDM_EXEC, (const void *)f, 0);
        break;

    case MPDM_TYPE_PROGRAM:
        r = mpdm_new(type | MPDM_EXEC | MPDM_MULTIPLE | MPDM_FREE, NULL, 0);

        mpdm_push(r, MPDM_X(f));
        mpdm_push(r, a);

        break;

    default:
        r = NULL;
    }

    return r;
}



mpdm_t mpdm_get(mpdm_t set, mpdm_t i)
{
    mpdm_t r;

    switch (mpdm_type(set)) {
    case MPDM_TYPE_FUNCTION:
    case MPDM_TYPE_PROGRAM:
        r = mpdm_exec_1(set, i, NULL);
        break;

    case MPDM_TYPE_ARRAY:
        r = mpdm_aget(set, mpdm_ival(i));
        break;

    case MPDM_TYPE_OBJECT:
        r = mpdm_hget(set, i);
        break;

    default:
        r = NULL;
        break;
    }

    return r;
}


mpdm_t mpdm_set(mpdm_t set, mpdm_t v, mpdm_t i)
{
    mpdm_t r = NULL;

    if (MPDM_IS_ARRAY(set))
        r = mpdm_aset(set, v, mpdm_ival(i));
    else
    if (MPDM_IS_HASH(set))
        r = mpdm_hset(set, v, i);

    return r;
}


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
    mpdm_t(*func2) (mpdm_t, mpdm_t);
    mpdm_t(*func3) (mpdm_t, mpdm_t, mpdm_t);

    mpdm_ref(c);
    mpdm_ref(args);
    mpdm_ref(ctxt);

    switch (mpdm_type(c)) {
    case MPDM_TYPE_FUNCTION:
        if ((func2 = (mpdm_t(*)(mpdm_t, mpdm_t)) (c->data)) != NULL)
            r = func2(args, ctxt);

        break;

    case MPDM_TYPE_PROGRAM:
        /* the executable is internally an array;
           1st element is the 3 argument version of the function (i.e. the cpu),
           2nd its optional additional information (i.e. the bytecode) */
        r = mpdm_aget(c, 0);

        if ((func3 = (mpdm_t(*)(mpdm_t, mpdm_t, mpdm_t)) (r->data)) != NULL)
            r = func3(mpdm_aget(c, 1), args, ctxt);

        break;

    default:
        r = NULL;
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
 * mpdm_iterator - Iterates through the content of a set.
 * @set: the set (hash, array, file or scalar)
 * @context: A pointer to an opaque context
 * @v: a pointer to a value to store the key
 * @i: a pointer to a value to store the index
 *
 * Iterates through the @set. If it's a hash, every value/index pair
 * is returned on each call. If it's an array, @v contains the
 * element and @i the index number on each call. If it's a file,
 * @v contains the line read and @i the index number. Otherwise, it's
 * assumed to be a string containing a numeral and @v and @i are filled
 * with values from 0 to @set - 1 on each call.
 *
 * Any of @v and @i pointers can be NULL if the value is not of interest.
 *
 * The @context pointer to integer is opaque and should be
 * initialized to zero on the first call.
 *
 * Returns 0 if no more data is left in @set.
 * [Hashes]
 * [Arrays]
 */
int mpdm_iterator(mpdm_t set, int *context, mpdm_t *v, mpdm_t *i)
{
    mpdm_t w;
    int ret = 0;

    mpdm_ref(set);

    switch (mpdm_type(set)) {
    case MPDM_TYPE_OBJECT:
        ret = mpdm_oiterator(set, context, v, i);
        break;

    case MPDM_TYPE_ARRAY:
        if (*context < mpdm_size(set)) {
            if (v) *v = mpdm_aget(set, (*context));
            if (i) *i = MPDM_I(*context);

            (*context)++;
            ret = 1;
        }

        break;

    case MPDM_TYPE_FILE:
        w = mpdm_read(set);

        if (w != NULL) {
            if (v)
                *v = w;
            else
                mpdm_void(w);

            if (i) *i = MPDM_I(*context);

            (*context)++;
            ret = 1;
        }

        break;

    case MPDM_TYPE_SCALAR:
        /* assume it's a number */
        if (*context < mpdm_ival(set)) {
            if (v) *v = MPDM_I(*context);
            if (i) *i = MPDM_I(*context);

            (*context)++;
            ret = 1;
        }

        break;

    default:
        ret = 0;
    }

    mpdm_unrefnd(set);

    return ret;
}


mpdm_t mpdm_map(mpdm_t set, mpdm_t filter, mpdm_t ctxt)
{
    mpdm_t v, i;
    mpdm_t out = NULL;
    int n;

    mpdm_ref(set);
    mpdm_ref(filter);
    mpdm_ref(ctxt);

    switch (mpdm_type(set)) {
    case MPDM_TYPE_NULL:
        break;

    case MPDM_TYPE_SCALAR:
        if (mpdm_type(filter) == MPDM_TYPE_SCALAR) {
            out = MPDM_A(0);

            while ((v = mpdm_regex(set, filter, n))) {
                mpdm_push(out, v);
                n = mpdm_regex_offset + mpdm_regex_size;
            }
        }

        break;

    default:
        out = MPDM_A(0);

        while (mpdm_iterator(set, &n, &v, &i)) {
            mpdm_t w = NULL;
            mpdm_ref(v);
            mpdm_ref(i);

            if (MPDM_IS_EXEC(filter))
                w = mpdm_exec_2(filter, v, i, ctxt);
            else
            if (MPDM_IS_HASH(filter))
                w = mpdm_hget(filter, v);
            else
            if (mpdm_type(filter) == MPDM_TYPE_SCALAR)
                w = mpdm_regex(v, filter, 0);
            else
                w = v;

            mpdm_push(out, w);

            mpdm_unref(i);
            mpdm_unref(v);
        }

        break;
    }

    mpdm_unref(ctxt);
    mpdm_unref(filter);
    mpdm_unref(set);

    return out;
}


mpdm_t mpdm_hmap(mpdm_t set, mpdm_t filter, mpdm_t ctxt)
{
    mpdm_t out = NULL;

    mpdm_ref(set);
    mpdm_ref(filter);
    mpdm_ref(ctxt);

    if (set != NULL) {
        mpdm_t v, i;
        int n = 0;

        out = MPDM_H(0);

        while (mpdm_iterator(set, &n, &v, &i)) {
            mpdm_t w = NULL;
            mpdm_ref(i);
            mpdm_ref(v);

            if (MPDM_IS_EXEC(filter))
                w = mpdm_exec_2(filter, v, i, ctxt);
            else
            if (filter == NULL) {
                /* invert hash */
                w = MPDM_A(2);
                mpdm_aset(w, v, 0);
                mpdm_aset(w, i, 1);
            }

            mpdm_ref(w);

            if (MPDM_IS_ARRAY(w))
                mpdm_hset(out, mpdm_aget(w, 0), mpdm_aget(w, 1));

            mpdm_unref(w);
            mpdm_unref(v);
            mpdm_unref(i);
        }
    }

    mpdm_unref(ctxt);
    mpdm_unref(filter);
    mpdm_unref(set);

    return out;
}


mpdm_t mpdm_grep(mpdm_t set, mpdm_t filter, mpdm_t ctxt)
{
    mpdm_t out = NULL;

    mpdm_ref(set);
    mpdm_ref(filter);
    mpdm_ref(ctxt);

    if (set != NULL) {
        mpdm_t v, i;
        int n = 0;

        out = MPDM_IS_HASH(set) ? MPDM_H(0) : MPDM_A(0);

        while (mpdm_iterator(set, &n, &v, &i)) {
            mpdm_t w = NULL;
            mpdm_ref(i);
            mpdm_ref(v);

            if (MPDM_IS_EXEC(filter))
                w = mpdm_exec_2(filter, v, i, ctxt);
            else
            if (mpdm_type(filter) == MPDM_TYPE_SCALAR)
                w = mpdm_regex(v, filter, 0);
            else
            if (MPDM_IS_HASH(filter))
                w = mpdm_bool(mpdm_exists(filter, v));

            if (mpdm_is_true(w)) {
                if (MPDM_IS_HASH(out))
                    mpdm_hset(out, i, v);
                else
                    mpdm_push(out, v);
            }

            mpdm_unref(v);
            mpdm_unref(i);
        }
    }

    mpdm_unref(ctxt);
    mpdm_unref(filter);
    mpdm_unref(set);

    return out;
}
