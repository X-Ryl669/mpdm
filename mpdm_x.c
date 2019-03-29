/*

    MPDM - Minimum Profit Data Manager
    mpdm_x.c - Extended functions

    Angel Ortega <angel@triptico.com> et al.

    This software is released into the public domain.
    NO WARRANTY. See file LICENSE for details.

*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <wchar.h>

#include "mpdm.h"

void mpdm_function__destroy(mpdm_t v)
{
    v->data = NULL;
}


void mpdm_program__destroy(mpdm_t v)
{
    mpdm_array__destroy(v);
}


/**
 * mpdm_is_true - Returns 1 if a value is true.
 * @v: the value
 *
 * Returns 1 if @v is true. False values are: NULL, integers
 * with value 0, reals with value 0.0, empty strings and the
 * special string "0".
 * The reference count is touched.
 */
int mpdm_is_true(mpdm_t v)
{
    int r;

    mpdm_ref(v);

    switch (mpdm_type(v)) {
    case MPDM_TYPE_NULL:
        r = 0;
        break;

    case MPDM_TYPE_INTEGER:
        r = mpdm_ival(v);
        break;

    case MPDM_TYPE_REAL:
        r = (mpdm_rval(v) != 0.0);
        break;

    case MPDM_TYPE_STRING:
        {
            wchar_t *ptr = mpdm_string(v);

            /* ... and it's "" or "0", it's false */
            r = !(*ptr == L'\0' || wcscmp(ptr, L"0") == 0);
        }

        break;

    default:
        r = 1;
        break;
    }

    mpdm_unref(v);

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
    return mpdm_get_wcs(mpdm_root(), b ? L"TRUE" : L"FALSE");
}


mpdm_t mpdm_new_x(mpdm_type_t type, void *f, mpdm_t a)
{
    mpdm_t r = NULL;

    switch (type) {
    case MPDM_TYPE_FUNCTION:
        r = mpdm_new(type, (const void *)f, 0);
        break;

    case MPDM_TYPE_PROGRAM:
        r = mpdm_new(type, NULL, 0);

        mpdm_push(r, MPDM_X(f));
        mpdm_push(r, a);

        break;

    default:
        r = NULL;
    }

    return r;
}



int mpdm_count_o(const mpdm_t o);

int mpdm_count(mpdm_t v)
{
    int r;

    switch (mpdm_type(v)) {
    case MPDM_TYPE_NULL:
    case MPDM_TYPE_ARRAY:
    case MPDM_TYPE_PROGRAM:
        r = mpdm_size(v);
        break;

    case MPDM_TYPE_OBJECT:
        r = mpdm_count_o(v);
        break;

    default:
        r = wcslen(mpdm_string(v));
        break;
    }

    return r;
}


mpdm_t mpdm_get_o(const mpdm_t o, const mpdm_t i);

mpdm_t mpdm_get(mpdm_t set, mpdm_t i)
{
    mpdm_t r;

    switch (mpdm_type(set)) {
    case MPDM_TYPE_FUNCTION:
    case MPDM_TYPE_PROGRAM:
        r = mpdm_exec_1(set, i, NULL);
        break;

    case MPDM_TYPE_ARRAY:
        r = mpdm_get_i(set, mpdm_ival(i));
        break;

    case MPDM_TYPE_OBJECT:
        r = mpdm_get_o(set, i);
        break;

    case MPDM_TYPE_STRING:
        switch (mpdm_type(i)) {
        case MPDM_TYPE_INTEGER:
        case MPDM_TYPE_REAL:
            mpdm_splice(set, NULL, mpdm_ival(i), 1, NULL, &r);
            break;

        default:
            r = NULL;
            break;
        }

        break;

    default:
        r = NULL;
        break;
    }

    return r;
}


mpdm_t mpdm_del_o(mpdm_t o, const mpdm_t i);

mpdm_t mpdm_del(mpdm_t set, mpdm_t i)
{
    mpdm_t r;

    switch (mpdm_type(set)) {
    case MPDM_TYPE_ARRAY:
        r = mpdm_del_i(set, mpdm_ival(i));
        break;

    case MPDM_TYPE_OBJECT:
        r = mpdm_del_o(set, i);
        break;

    default:
        r = NULL;
        break;
    }

    return r;
}


mpdm_t mpdm_set_o(mpdm_t o, mpdm_t v, mpdm_t i);

mpdm_t mpdm_set(mpdm_t set, mpdm_t v, mpdm_t i)
{
    mpdm_t r;

    switch (mpdm_type(set)) {
    case MPDM_TYPE_FUNCTION:
    case MPDM_TYPE_PROGRAM:
        r = mpdm_exec_2(set, v, i, NULL);
        break;

    case MPDM_TYPE_ARRAY:
        r = mpdm_set_i(set, v, mpdm_ival(i));
        break;

    case MPDM_TYPE_OBJECT:
        r = mpdm_set_o(set, v, i);
        break;

    default:
        r = NULL;
        break;
    }

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
        r = mpdm_get_i(c, 0);

        if ((func3 = (mpdm_t(*)(mpdm_t, mpdm_t, mpdm_t)) (r->data)) != NULL)
            r = func3(mpdm_get_i(c, 1), args, ctxt);

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

    mpdm_set_i(a, a1, 0);

    return mpdm_exec(c, a, ctxt);
}


mpdm_t mpdm_exec_2(mpdm_t c, mpdm_t a1, mpdm_t a2, mpdm_t ctxt)
{
    mpdm_t a = MPDM_A(2);

    mpdm_set_i(a, a1, 0);
    mpdm_set_i(a, a2, 1);

    return mpdm_exec(c, a, ctxt);
}


mpdm_t mpdm_exec_3(mpdm_t c, mpdm_t a1, mpdm_t a2, mpdm_t a3, mpdm_t ctxt)
{
    mpdm_t a = MPDM_A(3);

    mpdm_set_i(a, a1, 0);
    mpdm_set_i(a, a2, 1);
    mpdm_set_i(a, a3, 2);

    return mpdm_exec(c, a, ctxt);
}


int mpdm_iterator_o(mpdm_t set, int *context, mpdm_t *v, mpdm_t *i);

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
        ret = mpdm_iterator_o(set, context, v, i);
        break;

    case MPDM_TYPE_ARRAY:
    case MPDM_TYPE_PROGRAM:
        if (*context < mpdm_size(set)) {
            if (v) *v = mpdm_get_i(set, (*context));
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

    case MPDM_TYPE_INTEGER:
    case MPDM_TYPE_REAL:
        if (*context < mpdm_ival(set)) {
            if (v) *v = MPDM_I(*context);
            if (i) *i = MPDM_I(*context);

            (*context)++;
            ret = 1;
        }

        break;

    case MPDM_TYPE_STRING:
        /* FIXME */

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
    int n = 0;

    mpdm_ref(set);
    mpdm_ref(filter);
    mpdm_ref(ctxt);

    switch (mpdm_type(set)) {
    case MPDM_TYPE_NULL:
        break;

    case MPDM_TYPE_STRING:
        out = MPDM_A(0);

        while ((v = mpdm_regex(set, filter, n))) {
            mpdm_push(out, v);
            n = mpdm_regex_offset + mpdm_regex_size;
        }

        break;

    default:
        out = MPDM_A(0);

        while (mpdm_iterator(set, &n, &v, &i)) {
            mpdm_t w = NULL;
            mpdm_ref(v);
            mpdm_ref(i);

            switch (mpdm_type(filter)) {
            case MPDM_TYPE_FUNCTION:
            case MPDM_TYPE_PROGRAM:
                w = mpdm_exec_2(filter, v, i, ctxt);
                break;

            case MPDM_TYPE_ARRAY:
            case MPDM_TYPE_OBJECT:
                w = mpdm_get(filter, v);
                break;

            case MPDM_TYPE_REGEX:
                w = mpdm_regex(v, filter, 0);
                break;

            case MPDM_TYPE_STRING:
                w = filter;
                break;

            default:
                w = v;
                break;
            }

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
    mpdm_t v, i;
    mpdm_t out = NULL;
    int n = 0;

    mpdm_ref(set);
    mpdm_ref(filter);
    mpdm_ref(ctxt);

    switch (mpdm_type(set)) {
    case MPDM_TYPE_NULL:
        break;

    default:
        out = MPDM_O();

        while (mpdm_iterator(set, &n, &v, &i)) {
            mpdm_t w = NULL;
            mpdm_ref(i);
            mpdm_ref(v);

            switch (mpdm_type(filter)) {
            case MPDM_TYPE_NULL:
                /* invert hash */
                w = MPDM_A(2);
                mpdm_set_i(w, v, 0);
                mpdm_set_i(w, i, 1);

                break;

            case MPDM_TYPE_FUNCTION:
            case MPDM_TYPE_PROGRAM:
                w = mpdm_exec_2(filter, v, i, ctxt);
                break;

            default:
                break;
            }

            mpdm_ref(w);

            if (mpdm_type(w) == MPDM_TYPE_ARRAY)
                mpdm_set(out, mpdm_get_i(w, 1), mpdm_get_i(w, 0));

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

        out = mpdm_type(set) == MPDM_TYPE_OBJECT ? MPDM_O() : MPDM_A(0);

        while (mpdm_iterator(set, &n, &v, &i)) {
            mpdm_t w = NULL;
            mpdm_ref(i);
            mpdm_ref(v);

            switch (mpdm_type(filter)) {
            case MPDM_TYPE_FUNCTION:
            case MPDM_TYPE_PROGRAM:
                w = mpdm_exec_2(filter, v, i, ctxt);
                break;

            case MPDM_TYPE_REGEX:
            case MPDM_TYPE_STRING:
                w = mpdm_regex(v, filter, 0);
                break;

            case MPDM_TYPE_OBJECT:
                w = mpdm_bool(mpdm_exists(filter, v));
                break;

            default:
                break;
            }

            if (mpdm_is_true(w)) {
                if (mpdm_type(out) == MPDM_TYPE_OBJECT)
                    mpdm_set(out, v, i);
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


/**
 * mpdm_join - Joins two values.
 * @a: first value
 * @b: second value
 *
 * Joins two values. If both are hashes, a new hash containing the
 * pairs in @a overwritten with the keys in @b is returned; if both
 * are arrays, a new array is returned with all elements in @a followed
 * by all elements in b; if @a is an array and @b is a string,
 * a new string is returned with all elements in @a joined using @b
 * as a separator; and if @a is a hash and @b is a string, a new array
 * is returned containing all pairs in @a joined using @b as a separator.
 * [Arrays]
 * [Hashes]
 * [Strings]
 */
mpdm_t mpdm_join(const mpdm_t a, const mpdm_t b)
{
    int n, c = 0;
    mpdm_t r, v, i;

    mpdm_ref(a);
    mpdm_ref(b);

    switch (mpdm_type(a)) {
    case MPDM_TYPE_OBJECT:

        switch (mpdm_type(b)) {
        case MPDM_TYPE_OBJECT:
            /* hash~hash -> hash */
            r = MPDM_O();

            n = 0;
            while (mpdm_iterator(a, &n, &v, &i))
                mpdm_set(r, v, i);
            n = 0;
            while (mpdm_iterator(b, &n, &v, &i))
                mpdm_set(r, v, i);

            break;

        case MPDM_TYPE_ARRAY:
            /* hash~array -> hash */
            r = MPDM_O();

            /* the array is a list of pairs */
            for (n = 0; n < mpdm_size(b); n += 2)
                mpdm_set(r, mpdm_get_i(b, n + 1), mpdm_get_i(b, n));

            break;

        case MPDM_TYPE_STRING:
            /* hash~string -> array */
            r = MPDM_A(mpdm_count(a));

            n = 0;
            while (mpdm_iterator(a, &n, &v, &i))
                mpdm_set_i(r, mpdm_strcat(i, mpdm_strcat(b, v)), c++);

            break;

        default:
            r = NULL;
            break;
        }

        break;

    case MPDM_TYPE_ARRAY:
        switch (mpdm_type(b)) {
        case MPDM_TYPE_ARRAY:
            /* array~array -> array */
            r = MPDM_A(0);

            n = 0;
            while (mpdm_iterator(a, &n, &v, NULL))
                mpdm_push(r, v);
            n = 0;
            while (mpdm_iterator(b, &n, &v, NULL))
                mpdm_push(r, v);

            break;

        case MPDM_TYPE_STRING:
        case MPDM_TYPE_NULL:
            /* array~string -> string */
            r = mpdm_join_wcs(a, b ? mpdm_string(b) : NULL);

            break;

        default:
            r = NULL;
            break;
        }

        break;

    case MPDM_TYPE_STRING:
        /* string~string -> string */
        r = mpdm_strcat(a, b);
        break;

    case MPDM_TYPE_INTEGER:
        /* integer~integer -> sum! */
        r = MPDM_I(mpdm_ival(a) + mpdm_ival(b));
        break;

    case MPDM_TYPE_REAL:
        /* real~real -> sum! */
        r = MPDM_R(mpdm_rval(a) + mpdm_rval(b));
        break;

    default:
        r = NULL;
        break;
    }

    mpdm_unref(b);
    mpdm_unref(a);

    return r;
}


mpdm_t mpdm_splice_a(const mpdm_t v, const mpdm_t i,
                     int offset, int del, mpdm_t *n, mpdm_t *d);
mpdm_t mpdm_splice_s(const mpdm_t v, const mpdm_t i,
                     int offset, int del, mpdm_t *n, mpdm_t *d);

mpdm_t mpdm_splice(const mpdm_t v, const mpdm_t i,
                   int offset, int del, mpdm_t *n, mpdm_t *d)
{
    mpdm_t r;

    switch (mpdm_type(v)) {
    case MPDM_TYPE_NULL:
    case MPDM_TYPE_STRING:
        r = mpdm_splice_s(v, i, offset, del, n, d);
        break;

    case MPDM_TYPE_ARRAY:
        r = mpdm_splice_a(v, i, offset, del, n, d);
        break;

    default:
        r = NULL;
        break;
    }

    return r;
}


/**
 * mpdm_cmp - Compares two values.
 * @v1: the first value
 * @v2: the second value
 *
 * Compares two values. If both has the MPDM_STRING flag set,
 * a comparison using wcscoll() is returned; if both are arrays,
 * the size is compared first and, if they have the same number
 * elements, each one is compared; otherwise, a simple visual
 * representation comparison is done.
 * [Strings]
 */
int mpdm_cmp(const mpdm_t v1, const mpdm_t v2)
{
    int r;

    mpdm_ref(v1);
    mpdm_ref(v2);

    /* same values? */
    if (v1 == v2)
        r = 0;
    else
    if (v1 == NULL)
        r = -1;
    else
    if (v2 == NULL)
        r = 1;
    else {
        switch (mpdm_type(v1)) {
        case MPDM_TYPE_NULL:
            r = -1;
            break;

        case MPDM_TYPE_INTEGER:
            r = mpdm_ival(v1) - mpdm_ival(v2);
            break;

        case MPDM_TYPE_REAL:
            {
                double d = mpdm_rval(v1) - mpdm_rval(v2);
                r = d < 0.0 ? -1 : d > 0.0 ? 1 : 0;
            }
            break;

        case MPDM_TYPE_ARRAY:
        case MPDM_TYPE_OBJECT:
        case MPDM_TYPE_PROGRAM:

            if (mpdm_type(v2) == mpdm_type(v1)) {
                /* if they are the same size, compare elements one by one */
                if ((r = mpdm_size(v1) - mpdm_size(v2)) == 0) {
                    int n = 0;
                    mpdm_t v, i;

                    while (mpdm_iterator(v1, &n, &v, &i)) {
                        if ((r = mpdm_cmp(v, mpdm_get(v2, i))) != 0)
                            break;
                    }
                }

                break;
            }

            /* fallthrough */

        default:
            r = mpdm_cmp_wcs(v1, v2 ? mpdm_string(v2) : NULL);
            break;
        }
    }

    mpdm_unref(v2);
    mpdm_unref(v1);

    return r;
}
