/*

    MPDM - Minimum Profit Data Manager
    mpdm_a.c - Array management

    Angel Ortega <angel@triptico.com> et al.

    This software is released into the public domain.
    NO WARRANTY. See file LICENSE for details.

*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "mpdm.h"


/** data **/

/* sorting callback code */
static mpdm_t sort_cb = NULL;


/** code **/

mpdm_t mpdm_array__destroy(mpdm_t a)
{
    int n;

    /* unref all contained values */
    for (n = 0; n < mpdm_size(a); n++)
        mpdm_unref(mpdm_get_i(a, n));

    return a;
}


mpdm_t mpdm_new_a(int size)
/* creates a new array value */
{
    mpdm_t v;

    /* creates and expands */
    v = mpdm_new(MPDM_TYPE_ARRAY, NULL, 0);

    mpdm_expand(v, 0, size);

    return v;
}


/* interface */

/**
 * mpdm_expand - Expands an array.
 * @a: the array
 * @index: insertion index
 * @num: number of elements to insert
 *
 * Expands an array value, inserting @num elements (initialized
 * to NULL) at the specified @index.
 * [Arrays]
 */
mpdm_t mpdm_expand(mpdm_t a, int index, int num)
{
    int n;
    mpdm_t *p;

    /* sanity checks */
    if (num > 0) {
        /* add size */
        a->size += num;

        /* expand */
        p = (mpdm_t *) realloc((mpdm_t *) a->data, a->size * sizeof(mpdm_t));

        /* moves up from top of the array */
        for (n = a->size - 1; n >= index + num; n--)
            p[n] = p[n - num];

        /* fills the new space with blanks */
        for (; n >= index; n--)
            p[n] = NULL;

        a->data = p;
    }

    return a;
}


/**
 * mpdm_collapse - Collapses an array.
 * @a: the array
 * @index: deletion index
 * @num: number of elements to collapse
 *
 * Collapses an array value, deleting @num elements at
 * the specified @index.
 * [Arrays]
 */
mpdm_t mpdm_collapse(mpdm_t a, int index, int num)
{
    int n;
    mpdm_t *p;

    if (a->size && num > 0) {
        /* don't try to delete beyond the limit */
        if (index + num > a->size)
            num = a->size - index;

        p = (mpdm_t *) a->data;

        /* unrefs the about-to-be-deleted elements */
        for (n = index; n < index + num; n++)
            mpdm_unref(p[n]);

        /* array is now shorter */
        a->size -= num;

        /* moves down the elements */
        for (n = index; n < a->size; n++)
            p[n] = p[n + num];

        /* finally shrinks the memory block */
        a->data = realloc(p, a->size * sizeof(mpdm_t));
    }

    return a;
}


/**
 * mpdm_get_i - Gets an element of an array.
 * @a: the array
 * @index: the subscript of the element
 *
 * Returns the element at @index of the array @a.
 * [Arrays]
 */
mpdm_t mpdm_get_i(const mpdm_t a, int index)
{
    mpdm_t r = NULL;

    index = mpdm_wrap_pointers(a, index, NULL);

    /* boundary checks */
    if (index >= 0 && index < mpdm_size(a)) {
        mpdm_t *p = (mpdm_t *) a->data;
        r = p[index];
    }

    return r;
}


/**
 * mpdm_set_i - Sets the value of an array's element.
 * @a: the array
 * @e: the element to be assigned
 * @index: the subscript of the element
 *
 * Sets the element of the array @a at @index to be the @e value.
 * Returns the new element (versions prior to 1.0.10 returned the
 * old element).
 * [Arrays]
 */
mpdm_t mpdm_set_i(mpdm_t a, mpdm_t e, int index)
{
    index = mpdm_wrap_pointers(a, index, NULL);

    /* if the array is shorter than offset, expand to make room for it */
    if (index >= mpdm_size(a))
        mpdm_expand(a, mpdm_size(a), index - mpdm_size(a) + 1);

    mpdm_t *p = (mpdm_t *) a->data;

    mpdm_store(&p[index], e);

    return e;
}


/**
 * mpdm_ins - Insert an element in an array.
 * @a: the array
 * @e: the element to be inserted
 * @index: subscript where the element is going to be inserted
 *
 * Inserts the @e value in the @a array at @index.
 * Further elements are pushed up, so the array increases its size
 * by one. Returns the inserted element.
 * [Arrays]
 */
mpdm_t mpdm_ins(mpdm_t a, mpdm_t e, int index)
{
    index = mpdm_wrap_pointers(a, index, NULL);

    /* open room and set value */
    mpdm_expand(a, index, 1);
    mpdm_set_i(a, e, index);

    return e;
}


/**
 * mpdm_del_i - Deletes an element of an array.
 * @a: the array
 * @index: subscript of the element to be deleted
 *
 * Deletes the element at @index of the @a array. The array
 * is shrinked by one. If @index is negative, is counted from
 * the end of the array (so a value of -1 means delete the
 * last element of the array).
 *
 * Always returns NULL (version prior to 1.0.10 used to return
 * the deleted element).
 * [Arrays]
 */
mpdm_t mpdm_del_i(mpdm_t a, int index)
{
    mpdm_collapse(a, mpdm_wrap_pointers(a, index, NULL), 1);

    return NULL;
}


/**
 * mpdm_shift - Extracts the first element of an array.
 * @a: the array
 *
 * Extracts the first element of the array. The array
 * is shrinked by one.
 *
 * Returns the element.
 * [Arrays]
 */
mpdm_t mpdm_shift(mpdm_t a)
{
    mpdm_t r;

    r = mpdm_ref(mpdm_get_i(a, 0));
    mpdm_del_i(a, 0);
    mpdm_unrefnd(r);

    return r;
}


/**
 * mpdm_push - Pushes a value into an array.
 * @a: the array
 * @e: the value
 *
 * Pushes a value into an array (i.e. inserts at the end).
 * [Arrays]
 */
mpdm_t mpdm_push(mpdm_t a, mpdm_t e)
{
    /* inserts at the end */
    return mpdm_ins(a, e, mpdm_size(a));
}


/**
 * mpdm_pop - Pops a value from an array.
 * @a: the array
 *
 * Pops a value from the array (i.e. deletes from the end
 * and returns it).
 * [Arrays]
 */
mpdm_t mpdm_pop(mpdm_t a)
{
    mpdm_t r;

    r = mpdm_ref(mpdm_get_i(a, -1));
    mpdm_del_i(a, -1);
    r = mpdm_unrefnd(r);

    return r;
}


/**
 * mpdm_queue - Implements a queue in an array.
 * @a: the array
 * @e: the element to be pushed
 * @size: maximum size of array
 *
 * Pushes the @e element into the @a array. If the array already has
 * @size elements, the first (oldest) element is deleted from the
 * queue and returned.
 *
 * Returns the deleted element, or NULL if the array doesn't have
 * @size elements yet.
 * [Arrays]
 */
mpdm_t mpdm_queue(mpdm_t a, mpdm_t e, int size)
{
    mpdm_t v = NULL;

    /* zero size is nonsense */
    if (size) {
        /* loop until a has the desired size */
        while (mpdm_size(a) > size)
            mpdm_del_i(a, 0);

        if (mpdm_size(a) == size)
            v = mpdm_shift(a);

        mpdm_push(a, e);
    }

    return v;
}


/**
 * mpdm_clone - Creates a clone of a value.
 * @v: the value
 *
 * Creates a clone of a value. If the value is multiple, a new value will
 * be created containing clones of all its elements; otherwise,
 * the same unchanged value is returned.
 * [Value Management]
 */
mpdm_t mpdm_clone(const mpdm_t v)
{
    int n;
    mpdm_t w = NULL;

    switch (mpdm_type(v)) {
    case MPDM_TYPE_ARRAY:
    case MPDM_TYPE_OBJECT:
        mpdm_ref(v);

        /* creates a similar value */
        w = mpdm_type(v) == MPDM_TYPE_OBJECT ? MPDM_O() : MPDM_A(0);

        /* fills each element with duplicates of the original */
        for (n = 0; n < v->size; n++)
            mpdm_set_i(w, mpdm_clone(mpdm_get_i(v, n)), n);

        mpdm_unref(v);
        break;

    default:
        w = v;
        break;
    }

    return w;
}


/**
 * mpdm_seek_wcs - Seeks a value in an array (sequential, string version).
 * @a: the array
 * @v: the value
 * @step: number of elements to step
 *
 * Seeks sequentially the value @v in the @a array in
 * increments of @step. A complete search should use a step of 1.
 * Returns the offset of the element if found, or -1 otherwise.
 * [Arrays]
 */
int mpdm_seek_wcs(const mpdm_t a, const wchar_t *v, int step)
{
    int n, o;

    /* avoid stupid steps */
    if (step <= 0)
        step = 1;

    o = -1;

    for (n = 0; o == -1 && n < mpdm_size(a); n += step) {
        int r;

        r = mpdm_cmp_wcs(mpdm_get_i(a, n), v);

        if (r == 0)
            o = n;
    }

    return o;
}


/**
 * mpdm_seek - Seeks a value in an array (sequential).
 * @a: the array
 * @v: the value
 * @step: number of elements to step
 *
 * Seeks sequentially the value @v in the @a array in
 * increments of @step. A complete search should use a step of 1.
 * Returns the offset of the element if found, or -1 otherwise.
 * [Arrays]
 */
int mpdm_seek(const mpdm_t a, const mpdm_t v, int step)
{
    int r;

    mpdm_ref(v);
    r = mpdm_seek_wcs(a, mpdm_string(v), step);
    mpdm_unref(v);

    return r;
}


/**
 * mpdm_bseek_wcs - Seeks a value in an array (binary, string version).
 * @a: the ordered array
 * @v: the value
 * @step: number of elements to step
 * @pos: the position where the element should be, if it's not found
 *
 * Seeks the value @v in the @a array in increments of @step.
 * The array should be sorted to work correctly. A complete search
 * should use a step of 1.
 *
 * If the element is found, returns the offset of the element
 * as a positive number; otherwise, -1 is returned and the position
 * where the element should be is stored in @pos. You can set @pos
 * to NULL if you don't mind.
 * [Arrays]
 */
int mpdm_bseek_wcs(const mpdm_t a, const wchar_t *v, int step, int *pos)
{
    int b, t, n, c, o;

    /* avoid stupid steps */
    if (step <= 0)
        step = 1;

    b = 0;
    t = (mpdm_size(a) - 1) / step;

    o = -1;

    while (o == -1 && t >= b) {
        mpdm_t w;

        n = (b + t) / 2;
        if ((w = mpdm_get_i(a, n * step)) == NULL)
            break;

        c = mpdm_cmp_wcs(w, v);

        if (c == 0)
            o = n * step;
        else
        if (c > 0)
            t = n - 1;
        else
            b = n + 1;
    }

    if (pos != NULL)
        *pos = b * step;

    return o;
}


/**
 * mpdm_bseek - Seeks a value in an array (binary).
 * @a: the ordered array
 * @v: the value
 * @step: number of elements to step
 * @pos: the position where the element should be, if it's not found
 *
 * Seeks the value @v in the @a array in increments of @step.
 * The array should be sorted to work correctly. A complete search
 * should use a step of 1.
 *
 * If the element is found, returns the offset of the element
 * as a positive number; otherwise, -1 is returned and the position
 * where the element should be is stored in @pos. You can set @pos
 * to NULL if you don't mind.
 * [Arrays]
 */
int mpdm_bseek(const mpdm_t a, const mpdm_t v, int step, int *pos)
{
    int r;

    mpdm_ref(v);
    r = mpdm_bseek_wcs(a, mpdm_string(v), step, pos);
    mpdm_unref(v);

    return r;
}


static int sort_cmp(const void *s1, const void *s2)
/* qsort help function */
{
    int ret = 0;

    /* if callback is NULL, use basic value comparisons */
    if (sort_cb == NULL)
        ret = mpdm_cmp(*(mpdm_t *) s1, *(mpdm_t *) s2);
    else {
        /* executes the callback and converts to integer */
        ret = mpdm_ival(mpdm_exec_2(sort_cb,
                                    (mpdm_t) *((mpdm_t *) s1),
                                    (mpdm_t) *((mpdm_t *) s2), NULL));
    }

    return ret;
}


/**
 * mpdm_sort - Sorts an array.
 * @a: the array
 * @step: increment step
 *
 * Sorts the array. @step is the number of elements to group together.
 *
 * Returns the same array, sorted (versions prior to 1.0.10 returned
 * a new array).
 * [Arrays]
 */
mpdm_t mpdm_sort(const mpdm_t a, int step)
{
    return mpdm_sort_cb(a, step, NULL);
}


/**
 * mpdm_sort_cb - Sorts an array with a special sorting function.
 * @a: the array
 * @step: increment step
 * @asort_cb: sorting function
 *
 * Sorts the array. @step is the number of elements to group together.
 * For each pair of elements being sorted, the executable mpdm_t value
 * @sort_cb is called with an array containing the two elements as
 * argument. It must return a signed numerical mpdm_t value indicating
 * the sorting order.
 *
 * Returns the same array, sorted (versions prior to 1.0.10 returned
 * a new array).
 * [Arrays]
 */
mpdm_t mpdm_sort_cb(mpdm_t a, int step, mpdm_t cb)
{
    if (a != NULL) {
        mpdm_store(&sort_cb, cb);

        qsort((mpdm_t *) a->data, mpdm_size(a) / step,
            sizeof(mpdm_t) * step, sort_cmp);

        mpdm_store(&sort_cb, NULL);
    }

    return a;
}


/**
 * mpdm_split_wcs - Separates a string into an array of pieces (string version).
 * @v: the value to be separated
 * @s: the separator
 *
 * Separates the @v string value into an array of pieces, using @s
 * as a separator.
 *
 * If the separator is NULL, the string is splitted by characters.
 *
 * If the string does not contain the separator, an array holding 
 * the complete string is returned.
 * [Arrays]
 * [Strings]
 */
mpdm_t mpdm_split_wcs(const mpdm_t v, const wchar_t *s)
{
    mpdm_t w = NULL;

    if (v != NULL) {
        const wchar_t *ptr;

        mpdm_ref(v);

        w = MPDM_A(0);

        /* NULL separator? special case: split string in characters */
        if (s == NULL) {
            for (ptr = mpdm_string(v); ptr && *ptr != '\0'; ptr++)
                mpdm_push(w, MPDM_NS(ptr, 1));
        }
        else {
            wchar_t *sptr;
            int ss;

            ss = wcslen(s);

            /* travels the string finding separators and creating new values */
            for (ptr = v->data;
                 *ptr != L'\0' && (sptr = wcsstr(ptr, s)) != NULL;
                 ptr = sptr + ss)
                mpdm_push(w, MPDM_NS(ptr, sptr - ptr));

            /* add last part */
            mpdm_push(w, MPDM_S(ptr));
        }

        mpdm_unref(v);
    }

    return w;
}


/**
 * mpdm_split - Separates a string into an array of pieces.
 * @v: the value to be separated
 * @s: the separator
 *
 * Separates the @v string value into an array of pieces, using @s
 * as a separator.
 *
 * If the separator is NULL, the string is splitted by characters.
 *
 * If the string does not contain the separator, an array holding 
 * the complete string is returned.
 * [Arrays]
 * [Strings]
 */
mpdm_t mpdm_split(const mpdm_t v, const mpdm_t s)
{
    mpdm_t r;

    mpdm_ref(s);
    r = mpdm_split_wcs(v, s ? mpdm_string(s) : NULL);
    mpdm_unref(s);

    return r;
}


/**
 * mpdm_join_wcs - Joins all elements of an array into a string (string version).
 * @a: array to be joined
 * @s: joiner string
 *
 * Joins all elements from @a into one string, using @s as a glue.
 * [Arrays]
 * [Strings]
 */
mpdm_t mpdm_join_wcs(const mpdm_t a, const wchar_t *s)
{
    int n, c;
    wchar_t *ptr = NULL;
    int l = 0;
    int ss;
    mpdm_t v, r = NULL;

    mpdm_ref(a);

    switch (mpdm_type(a)) {
    case MPDM_TYPE_ARRAY:
    case MPDM_TYPE_FILE:

        n = c = 0;
        ss = s ? wcslen(s) : 0;

        while (mpdm_iterator(a, &n, &v, NULL)) {
            /* add separator */
            if (c && ss)
                ptr = mpdm_pokewsn(ptr, &l, s, ss);

            /* add element */
            ptr = mpdm_pokev(ptr, &l, v);
            c++;
        }

        r = ptr == NULL ? MPDM_S(L"") : MPDM_ENS(ptr, l);

        break;

    default:
        break;
    }

    mpdm_unref(a);

    return r;
}


mpdm_t mpdm_reverse(const mpdm_t a)
{
    int n, m = mpdm_size(a);
    mpdm_t r = MPDM_A(m);

    mpdm_ref(a);

    for (n = 0; n < m; n++)
        mpdm_set_i(r, mpdm_get_i(a, m - n - 1), n);

    mpdm_unref(a);

    return r;
}


mpdm_t mpdm_splice_a(const mpdm_t v, const mpdm_t i,
                     int offset, int del, mpdm_t *n, mpdm_t *d)
/* do not use this; use mpdm_splice() */
{
    int c;

    mpdm_ref(v);
    mpdm_ref(i);

    if (n) *n = NULL;
    if (d) *d = NULL;

    offset = mpdm_wrap_pointers(v, offset, &del);

    if (offset > mpdm_size(v))
        offset = mpdm_size(v);

    if (n) {
        *n = MPDM_A(0);

        /* copy the first part */
        for (c = 0; c < offset; c++)
            mpdm_push(*n, mpdm_get_i(v, c));

        /* copy all elements in i */
        for (c = 0; c < mpdm_size(i); c++)
            mpdm_push(*n, mpdm_get_i(i, c));

        /* copy the remainder */
        for (c = offset + del; c < mpdm_size(v); c++)
            mpdm_push(*n, mpdm_get_i(v, c));
    }

    if (d) {
        *d = MPDM_A(0);

        for (c = offset; c < offset + del; c++)
            mpdm_push(*d, mpdm_get_i(v, c));
    }

    mpdm_unref(i);
    mpdm_unref(v);

    /* returns the new value or the deleted value */
    return n ? *n : (d ? *d : NULL);
}

/** old compatibility layer **/

mpdm_t mpdm_aget(const mpdm_t a, int index)
{
    return mpdm_get_i(a, index);
}


mpdm_t mpdm_aset(mpdm_t a, mpdm_t e, int index)
{
    return mpdm_set_i(a, e, index);
}


mpdm_t mpdm_adel(mpdm_t a, int index)
{
    return mpdm_del_i(a, index);
}
